//
// Created by Yuriy Kulinchenko on 31/03/2026.
//

#include "unification_environment.h"
#include <iostream>

int unification_environment::add_node(node& node) {
    switch (node.type) {
        using enum node_type;
        case TERM: return add_structure(node);
        case VARIABLE: return add_variable(node.name);
        default: throw std::logic_error("ERROR: Passed node is not a term or variable");
    }
}

int unification_environment::add_structure(node &node_instance) {
    int structure_index = static_cast<int>(term_vector.size());
    term_vector.emplace_back(prolog_term_type::STRUCTURE);

    int index = add_identifier(node_instance.name);
    term_vector[structure_index].as.structure.identifier_index = index;

    for (node& child: node_instance.children) {
        int child_index = add_node(child);
        term_vector[structure_index].as.structure.children.push_back(child_index);
    }

    return structure_index;
}

int unification_environment::add_variable(std::string& variable_name) {
    if (name_variable_map.contains(variable_name)) {
        return name_variable_map[variable_name];
    }
    // Otherwise, this name is not yet mapped:
    term_vector.emplace_back(prolog_term_type::VARIABLE);
    int variable_index = static_cast<int>(term_vector.size()) - 1;
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

int unification_environment::resolve_bound_variable(int variable_index) {
    prolog_term& term = term_vector[variable_index];

    // Variable index may be a structure:
    if (term.type == prolog_term_type::STRUCTURE) return variable_index;

    // Variable index may be unbound:
    prolog_variable& variable = term.as.variable;
    if (variable.type == prolog_variable_type::UNBOUND) {
        return variable_index;
    }

    // If variable is bound, follow it:
    return resolve_bound_variable(variable.index);
}


bool unification_environment::unify(int i, int j) {
    int trail_index = trail.size();

    // If a variable is bound, you can keep going deeper:
    i = resolve_bound_variable(i);
    j = resolve_bound_variable(j);

    prolog_term& i_term = term_vector[i];
    prolog_term& j_term = term_vector[j];

    // Structure unification:
    if (i_term.is_structure() && j_term.is_structure()) {
        bool success = unify_structures(i, j);
        if (success) return true;
        unwind_trail(trail_index);
        return false;
    }

    // Variable unification:
    if (i_term.is_variable() && j_term.is_variable()) {
        unify_unbound_variables(i, j);
        return true;
    }

    // Variable + term unification:
    if (i_term.is_variable()) {
        unify_unbound_variable_term(i, j);
        return true;
    }

    unify_unbound_variable_term(j, i);
    return true;
}

bool unification_environment::unify_structures(int i, int j) {
    prolog_structure& i_structure = term_vector[i].as.structure;
    prolog_structure& j_structure = term_vector[j].as.structure;
    if (i_structure.identifier_index != j_structure.identifier_index) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Identifiers: " << identifier_vector[i_structure.identifier_index]
        << ", " << identifier_vector[j_structure.identifier_index] << " do not match.\n";
#endif
        return false;
    }

    if (i_structure.children.size() != j_structure.children.size()) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Arity of " << i_structure.children.size()
        << " and " << j_structure.children.size() << " do not match.\n";
#endif
        return false;
    }

    for (int n = 0; n < i_structure.children.size(); n++) {
        if (!unify(i_structure.children[n], j_structure.children[n])) return false;
    }

    return true;
}

// unify_unbound_variables(i, j) binds i, leaves j free
void unification_environment::unify_unbound_variables(int i, int j) {
    // If the variables are the same, do nothing:
    if (i == j) return;
    term_vector[i].as.variable = {prolog_variable_type::BOUND, j};
    trail.push_back(i);
}

// unify_unbound_variable_term(i, j) binds i to the term j
void unification_environment::unify_unbound_variable_term(int i, int j) {
    term_vector[i].as.variable = {prolog_variable_type::BOUND, j};
    trail.push_back(i);
}

void unification_environment::unwind_trail(int i) {
    if (trail.empty()) return;
    for (int j = static_cast<int>(trail.size()) - 1; j >= i; j--) {
        int variable_index = trail[j];
        term_vector[variable_index].as.variable.type = prolog_variable_type::UNBOUND;
    }

    trail.resize(i);
}

void unification_environment::unwind_term_vector(int i) {
    if (term_vector.empty()) return;
    for (int j = static_cast<int>(term_vector.size()) - 1; j >= i; j--) {
        if (term_vector[j].is_variable()) {
            auto it = variable_name_map.find(j);
            if (it != variable_name_map.end()) {
                std::string name = it->second;
                variable_name_map.erase(it);
                name_variable_map.erase(name);
            }
        }
    }
    term_vector.erase(term_vector.begin() + i, term_vector.end());
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

    int i1 = add_node(n1);
    int i2 = add_node(n2);

    bool success = unify(i1, i2);

    std::cout << (success ? "Unification succeeded" : "Unification failed") << '\n';
    if (success) log_variables(std::cout);

    unwind_trail(0);
    unwind_term_vector(0);
}

std::ostream &unification_environment::log_term(std::ostream& stream, int i, int depth) {
    if (depth == 0) return stream << "...";
    i = resolve_bound_variable(i);

    if (term_vector[i].is_variable())
        return log_variable(stream, i, depth);

    return log_structure(stream, i, depth);
}

std::ostream &unification_environment::log_structure(std::ostream &stream, int structure_index, int depth) {
    prolog_structure& structure = term_vector[structure_index].as.structure;
    std::string& identifier = identifier_vector[structure.identifier_index];

    // Dedicated list handling:

    if (identifier == ".") {
        stream << '[';

        // Progress through the list:
        log_term(stream, structure.children[0], depth - 1);

        prolog_structure* current_structure = &structure;

        while (depth > 0) {
            depth--;

            int next_index = current_structure->children[1];
            next_index = resolve_bound_variable(next_index);

            if (term_vector[next_index].is_structure()) {
                current_structure = &term_vector[next_index].as.structure;
                std::string& id = identifier_vector[current_structure->identifier_index];

                if (id == "[]") {
                    return stream << ']';
                }

                if (id == ".") {
                    stream << ", ";
                    log_term(stream, current_structure->children[0], depth);
                    continue;
                }
            }

            // Non-cons structure OR variable → treat as tail
            stream << "| ";
            log_term(stream, next_index, depth);
            return stream << ']';
        }

        return stream << ']';
    }

    stream << identifier;

    if (!structure.children.empty()) {
        stream << '(';
        log_term(stream, structure.children[0], depth - 1);
        for (int j = 1; j < structure.children.size(); j++) {
            stream << ", ";
            log_term(stream, structure.children[j], depth - 1);
        }
        stream << ')';
    }

    return stream;
}

std::ostream &unification_environment::log_variable(std::ostream &stream, int variable_index, int depth) {
    if (variable_name_map.contains(variable_index)) {
        stream << variable_name_map[variable_index];
        return stream;
    }
    stream << "V" << variable_index;
    return stream;
}




std::ostream& unification_environment::log_variables(std::ostream &stream) {
    for (auto& [name, index]: name_variable_map) {
        stream << name << " = ";
        log_term(stream, index);
        stream << '\n';
    }
    return stream;
}

