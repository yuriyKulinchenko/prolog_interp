//
// Created by Yuriy Kulinchenko on 31/03/2026.
//

#include "unification_environment.h"
#include <iostream>

env_index unification_environment::add_node(node& node) {
    switch (node.type) {
        using enum node_type;
        case TERM: return {prolog_term_type::TERM, add_term(node)};
        case VARIABLE: return {prolog_term_type::VARIABLE, add_variable(node.name)};
        default: throw std::logic_error("Passed node is not a term or variable");
    }
}

int unification_environment::add_term(node &node_instance) {
    // - Resolve identifier map

    int index = add_identifier(node_instance.name);
    prolog_term term {prolog_term_type::TERM, index};

    // - Recursively map children

    for (node& child: node_instance.children) {
        term.children.push_back(add_node(child));
    }

    // - Form constructed term

    term_vector.push_back(term);
    return static_cast<int>(term_vector.size()) - 1;
}

int unification_environment::add_variable(std::string& variable_name) {
    if (name_variable_map.contains(variable_name)) {
        return name_variable_map[variable_name];
    }
    // Otherwise, this name is not yet mapped:
    variable_vector.emplace_back();
    int variable_index = static_cast<int>(variable_vector.size()) - 1;
    name_variable_map[variable_name] = variable_index;
    variable_name_map[variable_index] = variable_name;
    return variable_index;
}

int unification_environment::add_identifier(const std::string &name) {
    int index;
    auto it = identifier_map.find(name);
    if (it != identifier_map.end()) {
        index = it->second;
    } else {
        identifier_vector.push_back(name);
        index = static_cast<int>(identifier_vector.size()) - 1;
        identifier_map[name] = index;
    }
    return index;
}

env_index unification_environment::resolve_bound_variable(int variable_index) {
    using enum prolog_term_type;
    using enum prolog_variable_type;
    prolog_variable& variable = variable_vector[variable_index];

    if (variable.type == BOUND_TERM)
        return {TERM, variable.index};

    if (variable.type == BOUND_VARIABLE)
        return resolve_bound_variable(variable.index);

    return {VARIABLE, variable_index};
}


bool unification_environment::unify(env_index i, env_index j) {
    using enum prolog_term_type;
    using enum prolog_variable_type;

    int trail_index = trail.size();

    // If a variable is bound, you can keep going deeper:
    if (i.type == VARIABLE) {
        i = resolve_bound_variable(i.index);
    }

    if (j.type == VARIABLE) {
        j = resolve_bound_variable(j.index);
    }

    // Term unification:
    if (i.type == TERM && j.type == TERM) {
        bool success = unify_terms(i.index, j.index);
        if (success) return true;
        unwind_trail(trail_index);
        return false;
    }

    // Variable unification:
    if (i.type == VARIABLE && j.type == VARIABLE) {
        unify_unbound_variables(i.index, j.index);
        return true;
    }

    // Variable + term unification:
    if (i.type == VARIABLE) {
        unify_unbound_variable_term(i.index, j.index);
        return true;
    }

    unify_unbound_variable_term(j.index, i.index);
    return true;
}

bool unification_environment::unify_terms(int i, int j) {
    prolog_term& t1 = term_vector[i];
    prolog_term& t2 = term_vector[j];
    if (t1.index != t2.index) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Identifiers: " << identifier_vector[t1.index]
        << ", " << identifier_vector[t2.index] << " do not match.\n";
#endif
        return false;
    }

    if (t1.children.size() != t2.children.size()) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Arity of " << t1.children.size()
        << " and " << t2.children.size() << " do not match.\n";
#endif
        return false;
    }

    for (int n = 0; n < t1.children.size(); n++) {
        if (!unify(t1.children[n], t2.children[n])) return false;
    }

    return true;
}

// unify_unbound_variables(i, j) binds i, leaves j free
void unification_environment::unify_unbound_variables(int i, int j) {
    // If the variables are the same, do nothing:
    if (i == j) return;
    variable_vector[i] = {prolog_variable_type::BOUND_VARIABLE, j};
    trail.push_back(i);
}

// unify_unbound_variable_term(i, j) binds i to the term j
void unification_environment::unify_unbound_variable_term(int i, int j) {
    variable_vector[i] = {prolog_variable_type::BOUND_TERM, j};
    trail.push_back(i);
}

void unification_environment::unwind_trail(int i) {
    if (trail.empty()) return;
    for (int j = static_cast<int>(trail.size()) - 1; j >= i; j--) {
        int variable_index = trail[j];
        variable_vector[variable_index].type = prolog_variable_type::UNBOUND;
    }

    trail.resize(i);
}

void unification_environment::unwind_variable_vector(int i) {
    if (variable_vector.empty()) return;
    for (int j = static_cast<int>(variable_vector.size()) - 1; j >= i; j--) {
        auto it = variable_name_map.find(j);
        if (it != variable_name_map.end()) {
            std::string name = it->second;
            variable_name_map.erase(it);
            name_variable_map.erase(name);
        }
    }

    variable_vector.resize(i);
}

void unification_environment::unwind_term_vector(int i) {
    term_vector.resize(i);
}

void unification_environment::test_unification() {
    std::string s1, s2;
    std::cout << "Enter first term: ";
    std::cin >> s1;
    std::cout << "Enter second term: ";
    std::cin >> s2;

    lexer lexer {std::move(s1)};
    parser parser{lexer.run()};
    node n1 = parser.term();

    lexer.reset(std::move(s2));
    parser.reset(lexer.run());
    node n2 = parser.term();

    env_index i1 = add_node(n1);
    env_index i2 = add_node(n2);

    std::cout << (unify(i1, i2) ?
    "Unification succeeded" : "Unification failed") << '\n';

    log_variables(std::cout);
    unwind_variable_vector(0);
    unwind_term_vector(0);
    unwind_trail(0);
}

std::ostream &unification_environment::log_term(std::ostream& stream, env_index i, int depth) {

    if (depth == 0) return stream << "...";

    // Identify what the variable is bound to:

    if (i.type == prolog_term_type::VARIABLE)
        i = resolve_bound_variable(i.index);

    // If term is still a variable:

    if (i.type == prolog_term_type::VARIABLE) {
        if (variable_name_map.contains(i.index)) {
            stream << variable_name_map[i.index];
            return stream;
        }

        stream << "V" << i.index;
        return stream;
    }

    // If raw term:

    prolog_term& term = term_vector[i.index];
    std::string& identifier = identifier_vector[term.index];

    // Dedicated list handling:

    if (identifier == ".") {
        stream << '[';
        // Progress through the list:
        log_term(stream, term.children[0], depth - 1);
        prolog_term* current_term = &term;
        while (depth > 0) {
            depth--;
            env_index next_index = current_term->children[1];
            if (next_index.type == prolog_term_type::VARIABLE) {
                next_index = resolve_bound_variable(next_index.index);
            }

            if (next_index.type == prolog_term_type::VARIABLE) {
                stream << "| ";
                log_term(stream, next_index, depth);
                return stream << ']';
            }

            // Raw term:
            current_term = &term_vector[next_index.index];
            std::string& id = identifier_vector[current_term->index];

            if (id == "[]") {
                return stream << ']';
            }

            if (id == ".") {
                stream << ", ";
                log_term(stream, current_term->children[0], depth);
            } else {
                stream << "| ";
                log_term(stream, next_index, depth);
                return stream << ']';
            }
        }
        return stream << ']';
    }

    stream << identifier_vector[term.index];

    if (!term.children.empty()) {
        stream << '(';
        log_term(stream, term.children[0], depth - 1);
        for (int j = 1; j < term.children.size(); j++) {
            stream << ", ";
            log_term(stream ,term.children[j], depth - 1);
        }
        stream << ')';
    }

    return stream;
}


std::ostream& unification_environment::log_variables(std::ostream &stream) {
    for (auto& [name, index]: name_variable_map) {
        stream << name << " = ";
        log_term(stream, {prolog_term_type::VARIABLE, index});
        stream << '\n';
    }
    return stream;
}

