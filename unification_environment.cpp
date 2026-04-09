//
// Created by Yuriy Kulinchenko on 31/03/2026.
//

#include "unification_environment.h"
#include "solver.h"
#include "helper.h"
#include <iostream>
#include <print>
#include <algorithm>

unification_environment::unification_environment() {
    // These remain fixed
    for (auto& identifier: reserved_identifiers) {
        std::string identifier_string {identifier};
        add_identifier(identifier_string);
    }
}

bool is_goal_node(node_type type) {
    using enum node_type;
    return type == GOAL
    || type == CONJUNCTION
    || type == DISJUNCTION
    || type == CUT;
}

bool is_goal_string(std::string& name) {
    return name == "," || name == ";" || name == "!";
}

template<prolog_term_type type>
void unification_environment::validate_type(int index) {
    prolog_term& term = term_vector[index];
    if (!(term.type == type)) {
        std::string expected_type = prolog_term_type_to_string(type);
        std::string received_type = prolog_term_type_to_string(term.type);
        throw formatted_error("ERROR: Expected {}, got {}", expected_type, received_type);
    }
}


prolog_structure &unification_environment::get_structure(int index) {
    validate_type<prolog_term_type::STRUCTURE>(index);
    return term_vector[index].as.structure;
}

prolog_variable &unification_environment::get_variable(int index) {
    validate_type<prolog_term_type::VARIABLE>(index);
    return term_vector[index].as.variable;
}

int unification_environment::get_integer(int index) {
    validate_type<prolog_term_type::INTEGER>(index);
    return term_vector[index].as.integer;
}

prolog_clause &unification_environment::get_clause(int index) {
    return clause_vector[index];
}

int unification_environment::add_node(node& node) {
    switch (node.type) {
        using enum node_type;
        case TERM: return add_structure(node);
        case INTEGER_TERM: return add_integer(node.integer);
        case VARIABLE: return add_variable(node.name);
        default: {
            if (is_goal_node(node.type)) return add_structure(node);
        }
    }
    throw std::logic_error("ERROR: Passed node is not a term, variable or goal");
}

int unification_environment::add_clause(node &node) {
    if (node.type != node_type::CLAUSE) {
        std::string type_string = node_type_to_short_string(node.type);
        throw formatted_error("ERROR: Expected clause, received {}", type_string);
    }

    // Clause may or may not have a body:
    clause_vector.emplace_back(add_node(node.children[0]));
    int clause_index = static_cast<int>(clause_vector.size()) - 1;
    if (node.children.size() == 2) {
        // The body is also present:
        clause_vector[clause_index].body = add_node(node.children[1]);
    }
    // Remove all variable bindings:
    variable_name_map.clear();
    name_variable_map.clear();
    return clause_index;
}

int unification_environment::duplicate_clause(int clause_index) {
    // Duplicates a clause, returns its index:
    int duplicated_clause_index = static_cast<int>(clause_vector.size());
    prolog_clause& original_clause = clause_vector[clause_index];
    std::unordered_map<int, int> variable_map{};
    int duplicated_head = duplicate_term(original_clause.head, variable_map);
    int duplicated_body = original_clause.body == -1 ? -1 : duplicate_term(original_clause.body, variable_map);
    clause_vector.emplace_back(duplicated_head, duplicated_body);
    return duplicated_clause_index;
}

int unification_environment::duplicate_term(int index, std::unordered_map<int, int> &variable_map) {
    prolog_term& term = term_vector[index];

    switch (term.type) {
        using enum prolog_term_type;
        case VARIABLE: {
#ifdef UNIFICATION_DEBUG
            if (term.as.variable.type == prolog_variable_type::BOUND) {
                log_term(std::cerr, index) << '\n';
                throw std::logic_error("ERROR: Cannot duplicate bound variable");
            }
#endif
            return duplicate_variable(index, variable_map);
        }

        case STRUCTURE: {
            return duplicate_structure(index, variable_map);
        }

        case INTEGER: {
            return index;
        }
    }
    return -1;
}

int unification_environment::duplicate_structure(int index, std::unordered_map<int, int>& variable_map) {
    const prolog_structure& original = get_structure(index);

    int original_identifier_index = original.identifier_index;
    std::vector<int> original_children = original.children;

    if (original_children.empty()) {
        // Atomic structure, index can be re-used:
        return index;
    }

    std::vector<int> duplicated_children;
    duplicated_children.reserve(original_children.size());

    for (int child : original_children) {
        duplicated_children.push_back(duplicate_term(child, variable_map));
    }

    int duplicated_structure_index = static_cast<int>(term_vector.size());
    term_vector.emplace_back(prolog_term_type::STRUCTURE);

    prolog_structure& duplicated = term_vector[duplicated_structure_index].as.structure;
    duplicated.identifier_index = original_identifier_index;
    duplicated.children = std::move(duplicated_children);

    return duplicated_structure_index;
}

int unification_environment::duplicate_variable(int index, std::unordered_map<int, int>& variable_map) {
    // If mapping exists:
    if (variable_map.contains(index)) return variable_map[index];

    // If mapping does not exist:
    int duplicated_variable_index = static_cast<int>(term_vector.size());
    term_vector.emplace_back(prolog_term_type::VARIABLE);
    variable_map[index] = duplicated_variable_index;
    return duplicated_variable_index;
}

void unification_environment::add_clauses(std::vector<node>& nodes) {
    for (auto& node: nodes) {
        add_clause(node);
    }

    auto projection = [&](const prolog_clause& clause) {
        return get_structure(clause.head).identifier_index;
    };

    // Clauses will be sorted in the clause_vector based on the ordering of the identifier, for O(1) access time
    std::ranges::sort(clause_vector, std::less{}, projection);

    int current_id = -1;
    int start_index = 0;

    for (int n = 0; n < clause_vector.size(); n++) {
        int id = projection(clause_vector[n]);

        if (id != current_id) {
            // close previous range
            if (current_id != -1) {
                identifier_clause_map[current_id] = {start_index, n};
            }

            // start new range
            current_id = id;
            start_index = n;
        }
    }

    // close final range
    if (current_id != -1) {
        identifier_clause_map[current_id] = {start_index, clause_vector.size()};
    }
}


int unification_environment::add_structure(node &node_instance) {
    if (node_instance.type == node_type::GOAL) return add_node(node_instance.children[0]);

    int index {};

    switch (node_instance.type) {
        using enum node_type;
        case CONJUNCTION: {
            index = get_reserved_identifier_index(",");
            break;
        }
        case DISJUNCTION: {
            index = get_reserved_identifier_index(";");
            break;
        }
        case CUT: {
            index = get_reserved_identifier_index("!");
            break;
        }
        case TERM: {
            index = add_identifier(node_instance.name);
            break;
        }

        default: {
            throw formatted_error("ERROR: Cannot add structure of type {}",
                node_type_to_short_string(node_instance.type));
        }
    }

    int structure_index = static_cast<int>(term_vector.size());
    term_vector.emplace_back(prolog_term_type::STRUCTURE);
    term_vector[structure_index].as.structure.identifier_index = index;

    for (node& child: node_instance.children) {
        int child_index = add_node(child);
        term_vector[structure_index].as.structure.children.push_back(child_index);
    }

    return structure_index;
}

int unification_environment::add_variable(std::string& variable_name) {

    if (name_variable_map.contains(variable_name) && variable_name != "_") {
        return name_variable_map[variable_name];
    }
    // Otherwise, this name is not yet mapped, or is anonymous:
    int variable_index = static_cast<int>(term_vector.size());
    term_vector.emplace_back(prolog_term_type::VARIABLE);


    if (variable_name != "_") {
        name_variable_map[variable_name] = variable_index;
        variable_name_map[variable_index] = variable_name;
    }

    return variable_index;
}

int unification_environment::add_integer(int integer) {
    term_vector.emplace_back(integer);
    return static_cast<int>(term_vector.size()) - 1;
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

    switch (term.type) {
        using enum prolog_term_type;
        case STRUCTURE:
        case INTEGER: return variable_index;
        case VARIABLE: {
            prolog_variable& variable = term.as.variable;

            // Variable index may be unbound:
            if (variable.type == prolog_variable_type::UNBOUND) {
                return variable_index;
            }

            // If variable is bound, follow it:
            return resolve_bound_variable(variable.index);
        }
    }
    throw formatted_error("Unrecognized type: {}", prolog_term_type_to_string(term.type));
}


bool unification_environment::unify(int i, int j) {
    int trail_index = static_cast<int>(trail.size());

    // If a variable is bound, you can keep going deeper:
    i = resolve_bound_variable(i);
    j = resolve_bound_variable(j);

    prolog_term& i_term = term_vector[i];
    prolog_term& j_term = term_vector[j];

    // Structure unification:
    if (i_term.is_ground() && j_term.is_ground()) {
        bool success = unify_ground(i, j);
        if (success) return true;
        unwind_trail(trail_index);
        return false;
    }

    // Variable unification:
    if (i_term.is_variable() && j_term.is_variable()) {
        unify_unbound_variables(i, j);
        return true;
    }

    // Variable + ground unification:
    if (i_term.is_variable()) {
        unify_unbound_variable_ground(i, j);
        return true;
    }


    // Ground + variable unification:
    unify_unbound_variable_ground(j, i);
    return true;
}

bool unification_environment::unify_ground(int i, int j) {
    prolog_term& i_term = term_vector[i];
    prolog_term& j_term = term_vector[j];

    if (i_term.type != j_term.type) return false;
    if (i_term.is_integer() && j_term.is_integer()) return i_term.as.integer == j_term.as.integer;

    prolog_structure& i_structure = get_structure(i);
    prolog_structure& j_structure = get_structure(j);
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
void unification_environment::unify_unbound_variable_ground(int i, int j) {
    term_vector[i].as.variable = {prolog_variable_type::BOUND, j};
    trail.push_back(i);
}

void unification_environment::unwind_trail(int i) {
    if (trail.empty()) return;
    for (int j = static_cast<int>(trail.size()) - 1; j >= i; j--) {
        int variable_index = trail[j];
        term_vector[variable_index].as.variable.type = prolog_variable_type::UNBOUND;
    }
    trail.erase(trail.begin() + i, trail.end());
}

void unification_environment::unwind_term_vector(int i) {
    if (term_vector.empty()) return;
    term_vector.erase(term_vector.begin() + i, term_vector.end());
}

void unification_environment::unwind_clause_vector(int i) {
    if (clause_vector.empty()) return;
    clause_vector.erase(clause_vector.begin() + i, clause_vector.end());
}

void unification_environment::test_unification() {
    std::string s1, s2;
    std::cout << "Enter first term: ";
    std::cin >> s1;
    std::cout << "Enter second term: ";
    std::cin >> s2;

    lexer lexer {std::move(s1)};
    parser parser{lexer.run(), lexer};
    node n1 = parser.term();

    lexer.reset(std::move(s2));
    parser.reset(lexer.run());
    node n2 = parser.term();

    int i1 = add_node(n1);
    int i2 = add_node(n2);

    bool success = unify(i1, i2);

    std::cout << (success ? "Unification succeeded" : "Unification failed") << '\n';
    if (success) log_variables(std::cout);

    variable_name_map.clear();
    name_variable_map.clear();
    unwind_trail(0);
    unwind_term_vector(0);
}

void unification_environment::test_clauses(const std::string& path) {
    std::string s = read_file(path);
    lexer lexer{std::move(s)};
    parser parser{lexer.run(), lexer};

    std::vector<node> clauses = parser.program();
    add_clauses(clauses);

    log_clauses(std::cout);

    std::cout << "Identifier vector: " << identifier_vector << '\n';
}

void unification_environment::test_duplication(const std::string &path) {
    std::string s = read_file(path);
    lexer lexer{std::move(s)};
    parser parser{lexer.run(), lexer};

    std::vector<node> clauses = parser.program();
    add_clauses(clauses);

    // Create one copy of each clause, and add it to the set of clauses:

    int n = static_cast<int>(clause_vector.size());

    for (int i = 0; i < n; i++) {
        duplicate_clause(i);
    }

    log_clauses(std::cout);
}



std::ostream &unification_environment::log_term(std::ostream& stream, int i, int depth) {
    if (depth == 0) return stream << "...";
    i = resolve_bound_variable(i);

    switch (term_vector[i].type) {
        using enum prolog_term_type;
        case VARIABLE: return log_variable(stream, i);
        case STRUCTURE: return log_structure(stream, i, depth);
        case INTEGER: return stream << get_integer(i);
    }

    return stream;
}

std::ostream &unification_environment::log_bracketed_term(std::ostream &stream, int term_index, int depth) {
    stream << '(';
    return log_term(stream, term_index, depth) << ')';
}


bool unification_environment::is_compound_term(int term_index) {
    if (!term_vector[term_index].is_structure()) return false;
    switch (get_structure(term_index).identifier_index) {
        case get_reserved_identifier_index(","):
        case get_reserved_identifier_index(";"):
            return true;
        case -1:
        default:
            return false;
    }
}

bool unification_environment::is_infix_term(int term_index) {
    if (!term_vector[term_index].is_structure()) return false;
    switch (get_structure(term_index).identifier_index) {
        case get_reserved_identifier_index("+"):
        case get_reserved_identifier_index("-"):
        case get_reserved_identifier_index("*"):
        case get_reserved_identifier_index("/"):
        case get_reserved_identifier_index("is"):
        case get_reserved_identifier_index("="):
        case get_reserved_identifier_index("<"):
        case get_reserved_identifier_index(">"):
        case get_reserved_identifier_index("\\="):
        case get_reserved_identifier_index("\\+"):
            return true;
        case -1:
        default:
            return false;
    }

}


#define COMPOUND_CASE_STATEMENT(s, c)\
case get_reserved_identifier_index(s): return log_compound_term(stream, structure_index, depth, c)

#define INFIX_CASE_STATEMENT(s)\
case get_reserved_identifier_index(s): return log_infix_term(stream, structure_index, depth, s)

std::ostream &unification_environment::log_structure(std::ostream &stream, int structure_index, int depth) {
    prolog_structure& structure = get_structure(structure_index);
    int identifier_index = structure.identifier_index;

    switch (identifier_index) {
        case get_reserved_identifier_index("."):
            return log_list(stream, structure_index, depth);
        COMPOUND_CASE_STATEMENT(",", ',');
        COMPOUND_CASE_STATEMENT(";", ';');
        INFIX_CASE_STATEMENT("+");
        INFIX_CASE_STATEMENT("-");
        INFIX_CASE_STATEMENT("*");
        INFIX_CASE_STATEMENT("/");
        INFIX_CASE_STATEMENT("=");
        INFIX_CASE_STATEMENT("<");
        INFIX_CASE_STATEMENT(">");
        INFIX_CASE_STATEMENT("is");
        default:
    }

    stream << identifier_vector[identifier_index];

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

std::ostream &unification_environment::log_list(std::ostream &stream, int list_index, int depth) {
    prolog_structure& structure = get_structure(list_index);
    stream << '[';

    // Progress through the list:

    if (is_compound_term(structure.children[0])) {
        log_bracketed_term(stream, structure.children[0], depth - 1);
    } else {
        log_term(stream, structure.children[0], depth - 1);
    }

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
                if (is_compound_term(current_structure->children[0])) {
                    log_bracketed_term(stream, current_structure->children[0], depth);
                } else {
                    log_term(stream, current_structure->children[0], depth);
                }
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


std::ostream &unification_environment::log_variable(std::ostream &stream, int variable_index) {
    if (variable_name_map.contains(variable_index)) return stream << variable_name_map[variable_index];
    return stream << "V" << variable_index;
}

std::ostream& unification_environment::log_variables(std::ostream &stream) {
    for (auto& [name, index]: name_variable_map) {
        stream << name << " = ";
        log_term(stream, index);
        stream << '\n';
    }
    return stream;
}

std::ostream &unification_environment::log_clauses(std::ostream &stream) {
    for (int i = 0; i < clause_vector.size(); i++) {
        log_clause(stream, i, max_logging_depth) << '\n';
    }
    return stream;
}


std::ostream &unification_environment::log_clause(std::ostream &stream, int clause_index, int depth) {
    prolog_clause& clause = clause_vector[clause_index];
    log_term(stream, clause.head, depth);
    if (clause.body != -1) {
        stream << " :- ";
        log_term(stream, clause.body, depth);
    }
    return stream << '.';
}

std::ostream &unification_environment::log_compound_term(std::ostream &stream, int term_index, int depth, char seperator) {
    std::vector<int>& term_children = get_structure(term_index).children;
    if (is_compound_term(term_children[0])) {
        log_bracketed_term(stream, term_children[0], depth - 1);
    } else {
        log_term(stream, term_children[0], depth - 1);
    }

    log_term(stream,term_children[0], depth - 1);
    for (int i = 1; i < term_children.size(); i++) {
        stream << seperator << ' ';
        if (is_compound_term(term_children[i])) {
            log_bracketed_term(stream, term_children[i], depth - 1);
        } else {
            log_term(stream, term_children[i], depth - 1);
        }
    }
    return stream;
}

std::ostream &unification_environment::log_infix_term(std::ostream &stream, int term_index, int depth, const std::string &infix_operator) {
    std::vector<int>& term_children = get_structure(term_index).children;
    int left_index = term_children[0];
    int right_index = term_children[1];

    if (is_infix_term(left_index) || is_compound_term(left_index)) {
        log_bracketed_term(stream, left_index, depth - 1);
    } else {
        log_term(stream, left_index, depth - 1);
    }

    stream << ' ' << infix_operator << ' ';

    if (is_infix_term(right_index) || is_compound_term(right_index)) {
        log_bracketed_term(stream, right_index, depth - 1);
    } else {
        log_term(stream, right_index, depth - 1);
    }
    return stream;
}

int unification_environment::evaluate_and_create_arithmetic_term(int term_index) {
    return add_integer(evaluate_arithmetic_term(term_index));
}

#define CASE_STATEMENT(op)\
case get_reserved_identifier_index(#op): {                          \
    int left = evaluate_arithmetic_term(structure.children[0]);     \
    int right = evaluate_arithmetic_term(structure.children[1]);    \
    return left op right;                                           \
}                                                                   \

int unification_environment::evaluate_arithmetic_term(int term_index) {
    term_index = resolve_bound_variable(term_index);
    prolog_term& term = term_vector[term_index];
    if (term.is_integer()) return term.as.integer;
    if (term.is_structure()) {
        prolog_structure& structure = get_structure(term_index);
        switch (structure.identifier_index) {
            CASE_STATEMENT(+);
            CASE_STATEMENT(*);
            CASE_STATEMENT(-);
            default: {
                throw formatted_error("ERROR: '{}' is not a valid arithmetic function",
                    identifier_vector[structure.identifier_index]);
            }

        }
    }
    throw formatted_error("ERROR: Unable to evaluate variable in arithmetic expression");
}

#undef CASE_STATEMENT



void unification_environment::run_interpreter() {
    int term_index = static_cast<int>(term_vector.size());
    for (;;) {
        unwind_term_vector(term_index);
        name_variable_map.clear();
        variable_name_map.clear();
        std::cout << "?- ";
        std::string s;
        std::getline(std::cin, s);
        if (s.empty()) continue;

        node n;

        try {
            lexer lexer(std::move(s));
            parser parser(lexer.run(), lexer);
            n = parser.query();
        } catch (std::logic_error& e) {
            std::println("{}{}{}", RED, e.what(), RESET);
            continue;
        }

        int goal_index = add_node(n);

        solver solver{*this};
        solver.solve(goal_index);
        ++solver;

        while (!solver.at_end()) {
            std::println("{}true{}", GREEN, RESET);
            log_variables(std::cout);
            std::string command;
            std::getline(std::cin, command);
            ++solver;
        }

        std::println("{}false{}", RED, RESET);
    }
}