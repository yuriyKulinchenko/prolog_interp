#ifndef UNIFICATION_H
#define UNIFICATION_H

#include <vector>
#include <unordered_map>
#include "parser.h"
#include "strong_indices.h"
#include "prolog_types.h"

// #define UNIFICATION_DEBUG
#define PERFORM_UNIFICATION_TYPE_CHECK

/*

Current purpose of unification_environment class is defined as follows (for now):

- Create an environment in which unification is possible
- This includes bound variables, variable mappings etc.
- Support facilities for unification: unification either succeeds and binds, or fails, and reverts

What does an environment look like?
- A list of bound / unbound variables, which I shall choose to model as a vector
- A list of terms, which can reference variables
- Terms can be added to the arena

Another facility: conversion from AST to term

Current problem: Store and instantiate goals
- Goals are stored as a head and a body
- Suppose that I have the goal: p(X,Y) :- q(X), r(Y)
- When I want to instantiate p(A,B), I want to create a copy of the goal:
- p(v1,v2) :- q(v1), r(v2)
- And then attempt to unify p(A,B) with p(v1,v2)
- I can then add q(v1), r(v2) to the goal stack
- When adding a goal, the name mapping should be cleared

What is a sensible representation for a decision point?
A decision point is uniquely identified by:
- A goal (index in the term_vector)
- A trail index (variable bindings)
- The index of the goal chosen

*/

class unification_environment {
public:
    friend class solver;
    friend class frame_solver;
    unification_environment();

    term_idx add_node(node& node);
    clause_idx add_clause(node& node);
    void add_clauses(std::vector<node>& nodes);

    void test_unification();
    void test_clauses(const std::string& path);
    void test_duplication(const std::string& path);
    void run_interpreter();

    trail_vec& get_trail();

    [[nodiscard]] const name_vec& get_names() const;
    [[nodiscard]] const var_name_vec& get_var_names() const;
    [[nodiscard]] const std::unordered_map<std::string, term_idx>& get_name_variable_map() const;

    [[nodiscard]] prolog_term& get_term_at(term_idx i);
    [[nodiscard]] size_t get_term_count() const;

    [[nodiscard]] const prolog_clause& get_clause_at(clause_idx idx) const;
    [[nodiscard]] size_t get_clause_count() const;

    void log_term(term_idx i, int depth = max_logging_depth);
    void log_bracketed_term(term_idx i, int depth = max_logging_depth);
    void log_variables();

    static consteval name_idx get_reserved_identifier_index(
        std::string_view s) {
        for (size_t i = 0; i < reserved_identifiers.size(); i++) {
            if (reserved_identifiers[i] == s) {
                return name_idx{i};
            }
        }
        return name_idx::invalid();
    }

private:
    template <prolog_term_type type>
    void validate_type(term_idx index);
    prolog_term& get_term(term_idx index);
    prolog_struct& get_struct(term_idx index);
    prolog_var& get_var(term_idx index);
    prolog_clause& get_clause(clause_idx index);
    int get_integer(term_idx index);

    term_idx add_structure(node& node_instance);
    term_idx add_integer(int integer);
    term_idx add_variable(std::string& variable_name);
    name_idx add_identifier(const std::string& name);
    var_name_idx add_variable_identifier(const std::string& variable_name);

    clause_idx duplicate_clause(clause_idx index);
    term_idx duplicate_term(term_idx index);
    term_idx duplicate_structure(term_idx index);
    term_idx duplicate_variable(term_idx index);

    bool unify(term_idx i, term_idx j);
    bool unify_ground(term_idx i, term_idx j);
    void unify_unbound_variables(term_idx i, term_idx j);
    void unify_unbound_variable_ground(term_idx i, term_idx j);
    term_idx resolve_bound_variable(term_idx variable_index);

    void unwind_term_vector(term_idx i);
    void unwind_clause_vector(clause_idx i);
    void unwind_trail(trail_idx i);

    void log_clauses();

    void log_structure(term_idx structure_index, int depth);
    void log_variable(term_idx variable_index);

    void log_list(term_idx list_index, int depth);
    void log_clause(clause_idx idx, int depth = max_logging_depth);
    void log_compound_term(term_idx i, int depth, char seperator);
    void log_infix_term(term_idx i, int depth,
                        const std::string& infix_operator);


    [[nodiscard]] bool is_compound_term(term_idx i);
    [[nodiscard]] bool is_infix_term(term_idx i);

    term_idx evaluate_and_create_arithmetic_term(term_idx i);
    int evaluate_arithmetic_term(term_idx i);

    prolog_timestamp get_timestamp();
    void apply_timestamp(prolog_timestamp timestamp);

    term_vec term_vector;
    clause_vec clause_vector;

    name_vec identifier_vector;
    var_name_vec variable_identifier_vector;
    strong_vector<var_name_idx, int> variable_version_vector;

    std::unordered_map<std::string, name_idx> identifier_map;
    std::unordered_map<std::string, var_name_idx> variable_identifier_map;
    strong_vector<name_idx, std::pair<clause_idx, clause_idx>> identifier_clause_map;

    std::unordered_map<std::string, term_idx> name_variable_map;
    std::unordered_map<term_idx, std::string> variable_name_map;

    static constexpr int max_logging_depth = 20;

    static constexpr std::array<std::string_view, 18> reserved_identifiers{
        ",", ";", "!", ".", "[]",
        "+", "-", "*", "/", "is",
        "=", "\\=", "\\+", "<", ">",
        "=<", ">=", "halt"
    };

    // For the operation of the interpreter:
    trail_vec trail;
};


#endif //UNIFICATION_H
