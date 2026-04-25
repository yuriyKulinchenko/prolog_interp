//
// Created by Yuriy Kulinchenko on 31/03/2026.
//

#include "unification_environment.h"
#include "solver.h"
#include "helper.h"
#include <iostream>
#include <print>
#include <algorithm>

#include "frame_solver.h"

unification_environment::unification_environment() {
    // These remain fixed
    for (auto& identifier : reserved_identifiers) {
        std::string identifier_string{identifier};
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

template <prolog_term_type type>
void unification_environment::validate_type(term_index index) {
    prolog_term& term = term_vector[index.raw()];
    if (!(term.type == type)) {
        std::string expected_type = prolog_term_type_to_string(type);
        std::string received_type = prolog_term_type_to_string(term.type);
        throw formatted_error("ERROR: Expected {}, got {}", expected_type,
                              received_type);
    }
}

prolog_term& unification_environment::get_term(term_index index) {
    return term_vector[index.raw()];
}

prolog_struct& unification_environment::get_struct(term_index index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::STRUCTURE>(index);
#endif
    return term_vector[index.raw()].structure();
}

prolog_var& unification_environment::get_var(term_index index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::VARIABLE>(index);
#endif
    return term_vector[index.raw()].variable();
}

int unification_environment::get_integer(term_index index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::INTEGER>(index);
#endif
    return term_vector[index.raw()].integer();
}

prolog_clause& unification_environment::get_clause(clause_index index) {
    return clause_vector[index.raw()];
}

term_index unification_environment::add_node(node& node) {
    switch (node.type) {
        using enum node_type;
    case TERM:
        return add_structure(node);
    case INTEGER_TERM:
        return add_integer(node.integer());
    case VARIABLE:
        return add_variable(node.name());
    default: {
        if (is_goal_node(node.type))
            return add_structure(node);
    }
    }
    throw std::logic_error(
        "ERROR: Passed node is not a term, variable or goal");
}

clause_index unification_environment::add_clause(node& node_instance) {
    if (node_instance.type != node_type::CLAUSE) {
        std::string type_string = node_type_to_string(node_instance.type);
        throw formatted_error("ERROR: Expected clause, received {}",
                              type_string);
    }

    // Clause head has constraints:

    node& head_node{node_instance.children[0]};

    if (head_node.type != node_type::TERM) {
        std::string type_string = node_type_to_string(node_instance.type);
        throw formatted_error(
            "ERROR: Clause head expected to be Term, received {}", type_string);
    }

    clause_vector.emplace_back(add_node(node_instance.children[0]));
    clause_index idx{clause_vector.size() - 1};
    if (node_instance.children.size() == 2) {
        // The body is also present:
        clause_vector[idx.raw()].body = add_node(node_instance.children[1]);
    }
    // Remove all variable bindings:
    variable_name_map.clear();
    name_variable_map.clear();
    return idx;
}

clause_index unification_environment::duplicate_clause(clause_index index) {
    // Duplicates a clause, returns its index:
    clause_index duplicated_clause_index{clause_vector.size()};
    prolog_clause& original_clause = clause_vector[index.raw()];
    std::unordered_map<term_index, term_index> variable_map{};
    term_index duplicated_head = duplicate_term(original_clause.head,
                                                variable_map);
    term_index duplicated_body = original_clause.body == term_index::invalid()
                                     ? term_index::invalid()
                                     : duplicate_term(
                                         original_clause.body, variable_map);
    clause_vector.emplace_back(duplicated_head, duplicated_body);
    return duplicated_clause_index;
}

term_index unification_environment::duplicate_term(
    term_index index,
    std::unordered_map<term_index, term_index>& variable_map) {
    prolog_term& term = term_vector[index.raw()];

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
    return term_index::invalid();
}

term_index unification_environment::duplicate_structure(
    term_index index,
    std::unordered_map<term_index, term_index>& variable_map) {
    size_t n = get_struct(index).num_children;
    if (n == 0)
        return index;

    identifier_index orig_id = get_struct(index).index;
    prolog_struct dup(n, orig_id);

    for (size_t i = 0; i < n; i++) {
        dup[i] = duplicate_term(get_struct(index)[i], variable_map);
    }

    term_index duplicate_index{term_vector.size()};
    term_vector.emplace_back(prolog_term_type::STRUCTURE);
    term_vector[duplicate_index.raw()].structure() = std::move(dup);
    return duplicate_index;
}

term_index unification_environment::duplicate_variable(
    term_index index,
    std::unordered_map<term_index, term_index>& variable_map) {
    // If mapping exists:
    if (variable_map.contains(index))
        return variable_map[index];

    // If mapping does not exist:

    term_index duplicated_variable_index{term_vector.size()};
    prolog_var& original_variable = term_vector[index.raw()].variable();

    prolog_var duplicated_variable{original_variable.identifier};
    term_vector.emplace_back(duplicated_variable);

    variable_map[index] = duplicated_variable_index;
    return duplicated_variable_index;
}

void unification_environment::add_clauses(std::vector<node>& nodes) {
    for (auto& node : nodes) {
        add_clause(node);
    }

    auto projection = [&](const prolog_clause& clause) {
        return get_struct(clause.head).index;
    };

    // Clauses will be sorted in the clause_vector based on the ordering of the identifier, for O(1) access time
    std::ranges::sort(clause_vector, std::less{}, projection);

    // Create flat table instead of hash-map:
    identifier_clause_map.assign(identifier_vector.size(),
                                 {clause_index{0}, clause_index{0}});

    identifier_index current_id = identifier_index::invalid();
    clause_index start_index{0};

    for (clause_index n{0}; n < clause_index{clause_vector.size()}; ++n) {
        identifier_index id = projection(clause_vector[n.raw()]);

        if (id != current_id) {
            // close previous range
            if (current_id != identifier_index::invalid()) {
                identifier_clause_map[current_id.raw()] = {start_index, n};
            }

            // start new range
            current_id = id;
            start_index = n;
        }
    }

    // close final range
    if (current_id != identifier_index::invalid()) {
        identifier_clause_map[current_id.raw()] =
            {start_index, clause_index{clause_vector.size()}};
    }
}


term_index unification_environment::add_structure(node& node_instance) {
    if (node_instance.type == node_type::GOAL)
        return add_node(node_instance.children[0]);

    identifier_index id{};

    switch (node_instance.type) {
        using enum node_type;
    case CONJUNCTION: {
        id = get_reserved_identifier_index(",");
        break;
    }
    case DISJUNCTION: {
        id = get_reserved_identifier_index(";");
        break;
    }
    case CUT: {
        id = get_reserved_identifier_index("!");
        break;
    }
    case TERM: {
        id = add_identifier(node_instance.name());
        break;
    }

    default: {
        throw formatted_error("ERROR: Cannot add structure of type {}",
                              node_type_to_short_string(node_instance.type));
    }
    }

    size_t n = node_instance.children.size();
    prolog_struct s(n, id);
    for (size_t i = 0; i < n; i++) {
        s[i] = add_node(node_instance.children[i]);
    }

    term_index structure_index{term_vector.size()};
    term_vector.emplace_back(prolog_term_type::STRUCTURE);
    term_vector[structure_index.raw()].structure() = std::move(s);
    return structure_index;
}

term_index unification_environment::add_variable(std::string& variable_name) {

    // Variable names are also added to the identifier index:
    identifier_index variable_name_index = add_identifier(variable_name);

    if (name_variable_map.contains(variable_name) && variable_name != "_") {
        return name_variable_map[variable_name];
    }
    // Otherwise, this name is not yet mapped, or is anonymous:
    term_index variable_index{term_vector.size()};

    prolog_var variable{variable_name_index};
    term_vector.emplace_back(variable);

    if (variable_name != "_") {
        name_variable_map[variable_name] = variable_index;
        variable_name_map[variable_index] = variable_name;
    }

    return variable_index;
}

term_index unification_environment::add_integer(int integer) {
    term_vector.emplace_back(integer);
    return term_index{term_vector.size() - 1};
}

identifier_index unification_environment::add_identifier(
    const std::string& name) {
    identifier_index index{};
    auto it = identifier_map.find(name);
    if (it != identifier_map.end()) {
        index = it->second;
    } else {
        identifier_vector.push_back(name);
        index = identifier_index{identifier_vector.size() - 1};
        identifier_map[name] = index;
    }
    return index;
}

term_index unification_environment::resolve_bound_variable(
    term_index variable_index) {

    while (get_term(variable_index).is_bound_variable()) {
        variable_index = get_var(variable_index).index;
    }

    return variable_index;
}


bool unification_environment::unify(term_index i, term_index j) {
    trail_index saved_trail{trail.size()};

    // If a variable is bound, you can keep going deeper:
    i = resolve_bound_variable(i);
    j = resolve_bound_variable(j);

    prolog_term& i_term = term_vector[i.raw()];
    prolog_term& j_term = term_vector[j.raw()];

    // Structure unification:
    if (i_term.is_ground() && j_term.is_ground()) {
        bool success = unify_ground(i, j);
        if (success)
            return true;
        unwind_trail(saved_trail);
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

bool unification_environment::unify_ground(term_index i, term_index j) {
    prolog_term& i_term = term_vector[i.raw()];
    prolog_term& j_term = term_vector[j.raw()];

    if (i_term.type != j_term.type)
        return false;
    if (i_term.is_integer() && j_term.is_integer())
        return i_term.integer() == j_term.integer();

    prolog_struct& i_structure = get_struct(i);
    prolog_struct& j_structure = get_struct(j);
    if (i_structure.index != j_structure.index) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Identifiers: " << identifier_vector[i_structure.index.raw()]
        << ", " << identifier_vector[j_structure.index.raw()] << " do not match.\n";
#endif
        return false;
    }

    if (i_structure.num_children != j_structure.num_children) {
#ifdef UNIFICATION_DEBUG
        std::cout << "Arity of " << i_structure.num_children
        << " and " << j_structure.num_children << " do not match.\n";
#endif
        return false;
    }

    for (size_t n = 0; n < i_structure.num_children; n++) {
        if (!unify(i_structure[n], j_structure[n]))
            return false;
    }

    return true;
}

// unify_unbound_variables(i, j) binds i, leaves j free
void unification_environment::unify_unbound_variables(
    term_index i, term_index j) {
    // If the variables are the same, do nothing:
    if (i == j)
        return;
    prolog_var& variable = term_vector[i.raw()].variable();
    variable.type = prolog_var_type::BOUND;
    variable.index = j;
    trail.push_back(i);
}

// unify_unbound_variable_term(i, j) binds i to the term j
void unification_environment::unify_unbound_variable_ground(
    term_index i, term_index j) {
    prolog_var& variable = term_vector[i.raw()].variable();
    variable.type = prolog_var_type::BOUND;
    variable.index = j;
    trail.push_back(i);
}

void unification_environment::unwind_trail(trail_index i) {
    if (trail.empty())
        return;

    for (size_t j = trail.size(); j-- > i.raw();) {
        term_index variable_index = trail[j];
        term_vector[variable_index.raw()].variable().type =
            prolog_var_type::UNBOUND;
    }

    trail.erase(trail.begin() + static_cast<std::ptrdiff_t>(i.raw()),
                trail.end());
}

void unification_environment::unwind_term_vector(term_index i) {
    if (term_vector.empty())
        return;
    term_vector.erase(
        term_vector.begin() + static_cast<std::ptrdiff_t>(i.raw()),
        term_vector.end());
}

void unification_environment::unwind_clause_vector(clause_index i) {
    if (clause_vector.empty())
        return;
    clause_vector.erase(
        clause_vector.begin() + static_cast<std::ptrdiff_t>(i.raw()),
        clause_vector.end());
}

void unification_environment::test_unification() {
    std::string s1, s2;
    std::cout << "Enter first term: ";
    std::cin >> s1;
    std::cout << "Enter second term: ";
    std::cin >> s2;

    lexer lexer{std::move(s1)};
    parser parser{lexer.run(), lexer};
    node n1 = parser.term();

    lexer.reset(std::move(s2));
    parser.reset(lexer.run());
    node n2 = parser.term();

    term_index i1 = add_node(n1);
    term_index i2 = add_node(n2);

    bool success = unify(i1, i2);

    std::cout << (success ? "Unification succeeded" : "Unification failed") <<
        '\n';
    if (success)
        log_variables();

    variable_name_map.clear();
    name_variable_map.clear();
    unwind_trail(trail_index{0});
    unwind_term_vector(term_index{0});
}

void unification_environment::test_clauses(const std::string& path) {
    std::string s = read_file(path);
    lexer lexer{std::move(s)};
    parser parser{lexer.run(), lexer};

    std::vector<node> clauses = parser.program();
    add_clauses(clauses);

    log_clauses();

    std::cout << "Identifier vector: " << identifier_vector << '\n';
}

void unification_environment::test_duplication(const std::string& path) {
    std::string s = read_file(path);
    lexer lexer{std::move(s)};
    parser parser{lexer.run(), lexer};

    std::vector<node> clauses = parser.program();
    add_clauses(clauses);

    // Create one copy of each clause, and add it to the set of clauses:

    clause_index n{clause_vector.size()};

    for (clause_index i{0}; i < n; ++i) {
        duplicate_clause(i);
    }

    log_clauses();
}


void unification_environment::log_term(term_index i, int depth) {
    if (depth == 0) {
        std::cout << "...";
        return;
    }

    i = resolve_bound_variable(i);

    switch (term_vector[i.raw()].type) {
        using enum prolog_term_type;
    case VARIABLE:
        log_variable(i);
        return;
    case STRUCTURE:
        log_structure(i, depth);
        return;
    case INTEGER: {
        std::cout << get_integer(i);
    }
    }
}

void unification_environment::log_bracketed_term(term_index i, int depth) {
    std::cout << '(';
    log_term(i, depth);
    std::cout << ')';
}


bool unification_environment::is_compound_term(term_index i) {
    if (!term_vector[i.raw()].is_structure())
        return false;
    switch (get_struct(i).index.raw()) {
    case get_reserved_identifier_index(",").raw():
    case get_reserved_identifier_index(";").raw():
        return true;
    default:
        return false;
    }
}

bool unification_environment::is_infix_term(term_index i) {
    if (!term_vector[i.raw()].is_structure())
        return false;
    switch (get_struct(i).index.raw()) {
    case get_reserved_identifier_index("+").raw():
    case get_reserved_identifier_index("-").raw():
    case get_reserved_identifier_index("*").raw():
    case get_reserved_identifier_index("/").raw():
    case get_reserved_identifier_index("is").raw():
    case get_reserved_identifier_index("=").raw():
    case get_reserved_identifier_index("\\=").raw():
    case get_reserved_identifier_index("\\+").raw():
    case get_reserved_identifier_index("<").raw():
    case get_reserved_identifier_index(">").raw():

        return true;
    default:
        return false;
    }

}


#define COMPOUND_CASE_STATEMENT(s, c)\
case get_reserved_identifier_index(s).raw(): log_compound_term(structure_index, depth, c); return

#define INFIX_CASE_STATEMENT(s)\
case get_reserved_identifier_index(s).raw(): log_infix_term(structure_index, depth, s); return

void unification_environment::log_structure(term_index structure_index,
                                            int depth) {
    prolog_struct& structure = get_struct(structure_index);
    identifier_index id = structure.index;

    switch (id.raw()) {
    case get_reserved_identifier_index(".").raw():
        return log_list(structure_index, depth);
    COMPOUND_CASE_STATEMENT(",", ',');
    COMPOUND_CASE_STATEMENT(";", ';');
    INFIX_CASE_STATEMENT("+");
    INFIX_CASE_STATEMENT("-");
    INFIX_CASE_STATEMENT("*");
    INFIX_CASE_STATEMENT("/");
    INFIX_CASE_STATEMENT("=");
    INFIX_CASE_STATEMENT("<");
    INFIX_CASE_STATEMENT(">");
    INFIX_CASE_STATEMENT(">=");
    INFIX_CASE_STATEMENT("=<");
    INFIX_CASE_STATEMENT("is");
    default:


    }

    std::cout << identifier_vector[id.raw()];

    if (structure.num_children > 0) {
        std::cout << '(';
        log_term(structure[0], depth - 1);
        for (int j = 1; j < structure.num_children; j++) {
            std::cout << ", ";
            log_term(structure[j], depth - 1);
        }
        std::cout << ')';
    }
}

void unification_environment::log_list(term_index list_index, int depth) {
    prolog_struct& structure = get_struct(list_index);
    std::cout << '[';

    // Progress through the list:

    if (is_compound_term(structure[0])) {
        log_bracketed_term(structure[0], depth - 1);
    } else {
        log_term(structure[0], depth - 1);
    }

    prolog_struct* current_structure = &structure;

    while (depth > 0) {
        depth--;

        term_index next_index = (*current_structure)[1];
        next_index = resolve_bound_variable(next_index);

        if (term_vector[next_index.raw()].is_structure()) {
            current_structure = &term_vector[next_index.raw()].structure();
            std::string& id = identifier_vector[current_structure->index.raw()];

            if (id == "[]") {
                std::cout << ']';
                return;
            }

            if (id == ".") {
                std::cout << ", ";
                if (is_compound_term(current_structure->at(0))) {
                    log_bracketed_term(current_structure->at(0), depth);
                } else {
                    log_term(current_structure->at(0), depth);
                }
                continue;
            }
        }

        // Non-cons structure OR variable -> treat as tail
        std::cout << "| ";
        log_term(next_index, depth);
        std::cout << ']';
        return;
    }

    std::cout << ']';
}


void unification_environment::log_variable(term_index variable_index) {
    prolog_var& variable = term_vector[variable_index.raw()].variable();

    if (variable.identifier == get_reserved_identifier_index("_")) {
        std::cout << "V";
    } else {
        std::cout << identifier_vector[variable.identifier.raw()];
    }

    std::cout << subscript_number(static_cast<int>(variable_index.raw()));
}

void unification_environment::log_variables() {
    for (auto& [name, index] : name_variable_map) {
        std::cout << name << " = ";
        log_term(index);
        std::cout << '\n';
    }
}

void unification_environment::log_clauses() {
    for (clause_index i{0}; i < clause_index{clause_vector.size()}; ++i) {
        log_clause(i, max_logging_depth);
        std::cout << '\n';
    }
}


void unification_environment::log_clause(clause_index idx, int depth) {
    prolog_clause& clause = clause_vector[idx.raw()];
    log_term(clause.head, depth);
    if (clause.body != term_index::invalid()) {
        std::cout << " :- ";
        log_term(clause.body, depth);
    }
    std::cout << '.';
}

void unification_environment::log_compound_term(term_index i, int depth,
                                                char seperator) {
    prolog_struct& structure = get_struct(i);
    if (is_compound_term(structure[0])) {
        log_bracketed_term(structure[0], depth - 1);
    } else {
        log_term(structure[0], depth - 1);
    }

    for (int j = 1; j < structure.num_children; j++) {
        std::cout << seperator << ' ';
        if (is_compound_term(structure[j])) {
            log_bracketed_term(structure[j], depth - 1);
        } else {
            log_term(structure[j], depth - 1);
        }
    }
}

void unification_environment::log_infix_term(term_index i, int depth,
                                             const std::string&
                                             infix_operator) {
    prolog_struct& structure = get_struct(i);
    term_index left_index = structure[0];
    term_index right_index = structure[1];

    if (is_infix_term(left_index) || is_compound_term(left_index)) {
        log_bracketed_term(left_index, depth - 1);
    } else {
        log_term(left_index, depth - 1);
    }

    std::cout << ' ' << infix_operator << ' ';

    if (is_infix_term(right_index) || is_compound_term(right_index)) {
        log_bracketed_term(right_index, depth - 1);
    } else {
        log_term(right_index, depth - 1);
    }
}

term_index unification_environment::evaluate_and_create_arithmetic_term(
    term_index i) {
    return add_integer(evaluate_arithmetic_term(i));
}

#define CASE_STATEMENT(op)\
case get_reserved_identifier_index(#op).raw(): {                    \
    int left = evaluate_arithmetic_term(structure[0]);              \
    int right = evaluate_arithmetic_term(structure[1]);             \
    return left op right;                                           \
}

int unification_environment::evaluate_arithmetic_term(term_index i) {
    i = resolve_bound_variable(i);
    prolog_term& term = term_vector[i.raw()];
    if (term.is_integer())
        return term.integer();
    if (term.is_structure()) {
        prolog_struct& structure = get_struct(i);
        switch (structure.index.raw()) {
        CASE_STATEMENT(+);
        CASE_STATEMENT(*);
        CASE_STATEMENT(-);
        default: {
            throw formatted_error(
                "ERROR: '{}' is not a valid arithmetic function",
                identifier_vector[structure.index.raw()]);
        }

        }
    }
    throw formatted_error(
        "ERROR: Unable to evaluate variable in arithmetic expression");
}

#undef CASE_STATEMENT


void unification_environment::run_interpreter() {
    term_index base_term{term_vector.size()};
    for (;;) {
        unwind_term_vector(base_term);
        name_variable_map.clear();
        variable_name_map.clear();
        std::cout << "?- ";
        std::string s;
        std::getline(std::cin, s);
        if (s.empty())
            continue;

        node n;

        try {
            lexer lexer(std::move(s));
            parser parser(lexer.run(), lexer);
            n = parser.query();
        } catch (std::logic_error& e) {
            std::println("{}{}{}", RED, e.what(), RESET);
            continue;
        }

        term_index goal_index = add_node(n);

        frame_solver solver{*this};
        solver.solve(goal_index);
        ++solver;

        while (!solver.at_end()) {
            std::println("{}true{}", GREEN, RESET);
            log_variables();
            std::string command;
            std::getline(std::cin, command);
            ++solver;
        }

        std::println("{}false{}", RED, RESET);
    }
}

prolog_timestamp unification_environment::get_timestamp() {
    return {
        term_index{term_vector.size()},
        clause_index{clause_vector.size()},
        trail_index{trail.size()}
    };
}

void unification_environment::apply_timestamp(prolog_timestamp timestamp) {
    unwind_trail(timestamp.trail_index_);
    unwind_clause_vector(timestamp.clause_index_);
    unwind_term_vector(timestamp.term_index_);
}

std::vector<term_index>& unification_environment::get_trail() {
    return trail;
}

