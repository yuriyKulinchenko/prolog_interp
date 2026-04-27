#include "unification_environment.h"
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
void unification_environment::validate_type(term_idx index) {
    prolog_term& term = term_vector[index];
    if (!(term.type == type)) {
        std::string expected_type = prolog_term_type_to_string(type);
        std::string received_type = prolog_term_type_to_string(term.type);
        throw formatted_error("ERROR: Expected {}, got {}", expected_type,
                              received_type);
    }
}

prolog_term& unification_environment::get_term(term_idx index) {
    return term_vector[index];
}

prolog_struct& unification_environment::get_struct(term_idx index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::STRUCTURE>(index);
#endif
    return term_vector[index].structure();
}

prolog_var& unification_environment::get_var(term_idx index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::VARIABLE>(index);
#endif
    return term_vector[index].variable();
}

int unification_environment::get_integer(term_idx index) {
#ifdef PERFORM_UNIFICATION_TYPE_CHECK
    validate_type<prolog_term_type::INTEGER>(index);
#endif
    return term_vector[index].integer();
}

prolog_clause& unification_environment::get_clause(clause_idx index) {
    return clause_vector[index];
}

term_idx unification_environment::add_node(node& node) {
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

clause_idx unification_environment::add_clause(node& node_instance) {
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

    clause_vector.emplace_back(add_node(node_instance.children[0]), node_instance.range);
    clause_idx idx{clause_vector.size() - 1};
    if (node_instance.children.size() == 2) {
        // The body is also present:
        clause_vector[idx].body = add_node(node_instance.children[1]);
    }
    // Remove all variable bindings:
    variable_name_map.clear();
    name_variable_map.clear();
    return idx;
}

// 'variable_bindings' and 'dirty_indices' are used during clause duplication

strong_vector<var_name_idx, term_idx> variable_bindings;
std::vector<size_t> dirty_indices;
size_t dirty_indices_length;
int anonymous_var_count = 0;

void add_variable_binding(var_name_idx original, term_idx duplicate) {
    variable_bindings[original] = duplicate;
    dirty_indices[dirty_indices_length++] = original.raw();
}

void clear_variable_bindings() {
    for (int i = 0; i < dirty_indices_length; i++) {
        variable_bindings[var_name_idx{dirty_indices[i]}] = term_idx::invalid();
    }
    dirty_indices_length = 0;
}

clause_idx unification_environment::duplicate_clause(clause_idx index) {
    // Duplicates a clause, returns its index:
    clause_idx duplicated_clause_index{clause_vector.size()};
    prolog_clause& original_clause = clause_vector[index];

    term_idx duplicated_head = duplicate_term(original_clause.head);
    term_idx duplicated_body =
        original_clause.body == term_idx::invalid()
            ? term_idx::invalid()
            : duplicate_term(original_clause.body);
    clause_vector.emplace_back(duplicated_head, duplicated_body);

    clear_variable_bindings();
    return duplicated_clause_index;
}

term_idx unification_environment::duplicate_term(term_idx index) {
    prolog_term& term = term_vector[index];

    switch (term.type) {
        using enum prolog_term_type;
    case VARIABLE: {
#ifdef UNIFICATION_DEBUG
        if (term.is_bound_variable()) {
            log_term(index);
            throw std::logic_error("ERROR: Cannot duplicate bound variable");
        }
#endif
        return duplicate_variable(index);
    }

    case STRUCTURE: {
        return duplicate_structure(index);
    }

    case INTEGER: {
        return index;
    }
    }
    return term_idx::invalid();
}

term_idx unification_environment::duplicate_structure(term_idx index) {
    size_t n = get_struct(index).num_children;
    if (n == 0)
        return index;

    name_idx orig_id = get_struct(index).index;
    prolog_struct dup(n, orig_id);

    for (size_t i = 0; i < n; i++) {
        dup[i] = duplicate_term(get_struct(index)[i]);
    }

    term_idx duplicate_index{term_vector.size()};
    term_vector.emplace_back(prolog_term_type::STRUCTURE);
    term_vector[duplicate_index].structure() = std::move(dup);
    return duplicate_index;
}

term_idx unification_environment::duplicate_variable(term_idx index) {
    prolog_var& original_variable = term_vector[index].variable();

    bool is_anonymous_variable = original_variable.identifier == var_name_idx::invalid();

    if (!is_anonymous_variable) {
        if (variable_bindings[original_variable.identifier] != term_idx::invalid())
            return variable_bindings[original_variable.identifier];
    }

    term_idx duplicated_variable_index{term_vector.size()};

    int new_version = is_anonymous_variable ?
    ++anonymous_var_count :
    ++variable_version_vector[original_variable.identifier];

    term_vector.emplace_back(prolog_var{original_variable.identifier, new_version});

    if (!is_anonymous_variable)
        add_variable_binding(original_variable.identifier, duplicated_variable_index);

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
    identifier_clause_map = strong_vector<name_idx, std::pair<clause_idx, clause_idx>>(
        identifier_vector.size(), std::pair{clause_idx{0}, clause_idx{0}});

    name_idx current_id = name_idx::invalid();
    clause_idx start_index{0};

    for (clause_idx n{0}; n < clause_idx{clause_vector.size()}; ++n) {
        name_idx id = projection(clause_vector[n]);

        if (id != current_id) {
            // close previous range
            if (current_id != name_idx::invalid()) {
                identifier_clause_map[current_id] = {start_index, n};
            }

            // start new range
            current_id = id;
            start_index = n;
        }
    }

    // close final range
    if (current_id != name_idx::invalid()) {
        identifier_clause_map[current_id] =
            {start_index, clause_idx{clause_vector.size()}};
    }

    variable_bindings = strong_vector<var_name_idx, term_idx>
    (variable_identifier_vector.size(), term_idx::invalid());

    dirty_indices =
        std::vector<size_t>(variable_identifier_vector.size());
}


term_idx unification_environment::add_structure(node& node_instance) {
    if (node_instance.type == node_type::GOAL)
        return add_node(node_instance.children[0]);

    name_idx id{};

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

    term_idx structure_index{term_vector.size()};
    term_vector.emplace_back(prolog_term_type::STRUCTURE);
    term_vector[structure_index].structure() = std::move(s);
    return structure_index;
}

term_idx unification_environment::add_variable(std::string& variable_name) {
    var_name_idx variable_name_index =
        add_variable_identifier(variable_name);

    if (name_variable_map.contains(variable_name) && variable_name != "_") {
        return name_variable_map[variable_name];
    }
    // Otherwise, this name is not yet mapped, or is anonymous:
    term_idx variable_index{term_vector.size()};

    prolog_var variable{variable_name_index};
    term_vector.emplace_back(variable);

    if (variable_name != "_") {
        name_variable_map[variable_name] = variable_index;
        variable_name_map[variable_index] = variable_name;
    }

    return variable_index;
}

term_idx unification_environment::add_integer(int integer) {
    term_vector.emplace_back(integer);
    return term_idx{term_vector.size() - 1};
}

name_idx unification_environment::add_identifier(
    const std::string& name) {
    name_idx index{};
    auto it = identifier_map.find(name);
    if (it != identifier_map.end()) {
        index = it->second;
    } else {
        index = name_idx{identifier_vector.size()};
        identifier_vector.push_back(name);
        identifier_map[name] = index;
    }
    return index;
}

var_name_idx unification_environment::add_variable_identifier(
    const std::string& variable_name) {
    if (variable_name == "_") return var_name_idx::invalid();
    var_name_idx index{};
    auto it = variable_identifier_map.find(variable_name);
    if (it != variable_identifier_map.end()) {
        index = it->second;
    } else {
        index = var_name_idx{variable_identifier_vector.size()};
        variable_identifier_vector.push_back(variable_name);
        variable_version_vector.push_back(0);
        variable_identifier_map[variable_name] = index;
    }
    return index;
}


term_idx unification_environment::resolve_bound_variable(
    term_idx variable_index) {

    while (get_term(variable_index).is_bound_variable()) {
        variable_index = get_var(variable_index).index;
    }

    return variable_index;
}


bool unification_environment::unify(term_idx i, term_idx j) {
    trail_idx saved_trail{trail.size()};

    // If a variable is bound, you can keep going deeper:
    i = resolve_bound_variable(i);
    j = resolve_bound_variable(j);

    prolog_term& i_term = term_vector[i];
    prolog_term& j_term = term_vector[j];

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

bool unification_environment::unify_ground(term_idx i, term_idx j) {
    prolog_term& i_term = term_vector[i];
    prolog_term& j_term = term_vector[j];

    if (i_term.type != j_term.type)
        return false;
    if (i_term.is_integer() && j_term.is_integer())
        return i_term.integer() == j_term.integer();

    prolog_struct& i_structure = get_struct(i);
    prolog_struct& j_structure = get_struct(j);
    if (i_structure.index != j_structure.index) {
#ifdef UNIFICATION_DEBUG
        std::cout <<
            "Identifiers: " << identifier_vector[i_structure.index]
            << ", " << identifier_vector[j_structure.index] <<
            " do not match.\n";
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
    term_idx i, term_idx j) {
    // If the variables are the same, do nothing:
    if (i == j)
        return;
    prolog_var& variable = term_vector[i].variable();
    variable.type = prolog_var_type::BOUND;
    variable.index = j;
    trail.push_back(i);
}

// unify_unbound_variable_term(i, j) binds i to the term j
void unification_environment::unify_unbound_variable_ground(
    term_idx i, term_idx j) {
    prolog_var& variable = term_vector[i].variable();
    variable.type = prolog_var_type::BOUND;
    variable.index = j;
    trail.push_back(i);
}

void unification_environment::unwind_trail(trail_idx i) {
    if (trail.empty())
        return;

    for (size_t j = trail.size(); j-- > i.raw();) {
        term_idx variable_index = trail[trail_idx{j}];
        term_vector[variable_index].variable().type =
            prolog_var_type::UNBOUND;
    }

    trail.erase(trail.begin() + static_cast<std::ptrdiff_t>(i.raw()),
                trail.end());
}

void unification_environment::unwind_term_vector(term_idx i) {
    if (term_vector.empty())
        return;
    term_vector.erase(
        term_vector.begin() + static_cast<std::ptrdiff_t>(i.raw()),
        term_vector.end());
}

void unification_environment::unwind_clause_vector(clause_idx i) {
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

    term_idx i1 = add_node(n1);
    term_idx i2 = add_node(n2);

    bool success = unify(i1, i2);

    std::cout << (success ? "Unification succeeded" : "Unification failed") <<
        '\n';
    if (success)
        log_variables();

    variable_name_map.clear();
    name_variable_map.clear();
    unwind_trail(trail_idx{0});
    unwind_term_vector(term_idx{0});
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

    clause_idx n{clause_vector.size()};

    for (clause_idx i{0}; i < n; ++i) {
        duplicate_clause(i);
    }

    log_clauses();
}


void unification_environment::log_term(term_idx i, int depth) {
    if (depth == 0) {
        std::cout << "...";
        return;
    }

    i = resolve_bound_variable(i);

    switch (term_vector[i].type) {
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

void unification_environment::log_bracketed_term(term_idx i, int depth) {
    std::cout << '(';
    log_term(i, depth);
    std::cout << ')';
}


bool unification_environment::is_compound_term(term_idx i) {
    if (!term_vector[i].is_structure())
        return false;
    switch (get_struct(i).index.raw()) {
    case get_reserved_identifier_index(",").raw():
    case get_reserved_identifier_index(";").raw():
        return true;
    default:
        return false;
    }
}

bool unification_environment::is_infix_term(term_idx i) {
    if (!term_vector[i].is_structure())
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

void unification_environment::log_structure(term_idx structure_index,
                                            int depth) {
    prolog_struct& structure = get_struct(structure_index);
    name_idx id = structure.index;

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

    std::cout << identifier_vector[id];

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

void unification_environment::log_list(term_idx list_index, int depth) {
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

        term_idx next_index = (*current_structure)[1];
        next_index = resolve_bound_variable(next_index);

        if (term_vector[next_index].is_structure()) {
            current_structure = &term_vector[next_index].structure();
            std::string& id = identifier_vector[current_structure->index];

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


void unification_environment::log_variable(term_idx variable_index) {
    prolog_var& variable = term_vector[variable_index].variable();

    // TODO: Handle anonymous variable '_' properly
    std::cout << variable_identifier_vector[variable.identifier] << subscript_number(variable.version);
}

void unification_environment::log_variables() {
    for (auto& [name, index] : name_variable_map) {
        std::cout << name << " = ";
        log_term(index);
        std::cout << '\n';
    }
}

void unification_environment::log_clauses() {
    for (clause_idx i{0}; i < clause_idx{clause_vector.size()}; ++i) {
        log_clause(i, max_logging_depth);
        std::cout << '\n';
    }
}


void unification_environment::log_clause(clause_idx idx, int depth) {
    prolog_clause& clause = clause_vector[idx];
    log_term(clause.head, depth);
    if (clause.body != term_idx::invalid()) {
        std::cout << " :- ";
        log_term(clause.body, depth);
    }
    std::cout << '.';
}

void unification_environment::log_compound_term(term_idx i, int depth,
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

void unification_environment::log_infix_term(term_idx i, int depth,
                                             const std::string&
                                             infix_operator) {
    prolog_struct& structure = get_struct(i);
    term_idx left_index = structure[0];
    term_idx right_index = structure[1];

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

term_idx unification_environment::evaluate_and_create_arithmetic_term(
    term_idx i) {
    return add_integer(evaluate_arithmetic_term(i));
}

#define CASE_STATEMENT(op)\
case get_reserved_identifier_index(#op).raw(): {                    \
    int left = evaluate_arithmetic_term(structure[0]);              \
    int right = evaluate_arithmetic_term(structure[1]);             \
    return left op right;                                           \
}

int unification_environment::evaluate_arithmetic_term(term_idx i) {
    i = resolve_bound_variable(i);
    prolog_term& term = term_vector[i];
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
                identifier_vector[structure.index]);
        }

        }
    }
    throw formatted_error(
        "ERROR: Unable to evaluate variable in arithmetic expression");
}

#undef CASE_STATEMENT


void unification_environment::run_interpreter() {
    term_idx base_term{term_vector.size()};
    for (;;) {
        unwind_term_vector(base_term);
        name_variable_map.clear();
        variable_name_map.clear();
        std::cout << "?- ";
        std::string s;
        std::getline(std::cin, s);
        if (s.empty()) continue;

        try {
            lexer lexer(std::move(s));
            parser parser(lexer.run(), lexer);
            node n = parser.query();

            term_idx goal_index = add_node(n);

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
        } catch (std::logic_error& e) {
            std::println("{}{}{}", RED, e.what(), RESET);
        }
    }
}

prolog_timestamp unification_environment::get_timestamp() {
    return {
        term_idx{term_vector.size()},
        clause_idx{clause_vector.size()},
        trail_idx{trail.size()}
    };
}

void unification_environment::apply_timestamp(prolog_timestamp timestamp) {
    unwind_trail(timestamp.trail);
    unwind_clause_vector(timestamp.clause);
    unwind_term_vector(timestamp.term);
}

trail_vec& unification_environment::get_trail() {
    return trail;
}

const name_vec&
unification_environment::get_names() const {
    return identifier_vector;
}

const var_name_vec&
unification_environment::get_var_names() const {
    return variable_identifier_vector;
}

prolog_term& unification_environment::get_term_at(term_idx i) {
    return term_vector[i];
}

size_t unification_environment::get_term_count() const {
    return term_vector.size();
}

const std::unordered_map<std::string, term_idx>&
unification_environment::get_name_variable_map() const {
    return name_variable_map;
}

const prolog_clause& unification_environment::get_clause_at(clause_idx idx) const {
    return clause_vector[idx];
}

size_t unification_environment::get_clause_count() const {
    return clause_vector.size();
}
