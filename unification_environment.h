#ifndef UNIFICATION_H
#define UNIFICATION_H

#include <vector>
#include <unordered_map>
#include "parser.h"
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
    // Adds the term specified by the passed node to the term_vector
    // Returns a pair specifying if the term is a raw term or variable, and the corresponding index

    friend class solver;
    friend class frame_solver;
    unification_environment();

    term_index add_node(node& node);
    clause_index add_clause(node& node);
    void add_clauses(std::vector<node>& nodes);

    void test_unification();
    void test_clauses(const std::string& path);
    void test_duplication(const std::string& path);
    void run_interpreter();

    std::vector<term_index>& get_trail();

    [[nodiscard]] const std::vector<std::string>& get_identifier_vector() const;
    [[nodiscard]] prolog_term& get_term_at(size_t i);
    [[nodiscard]] size_t get_term_count() const;
    [[nodiscard]] const std::unordered_map<std::string, term_index>&
    get_name_variable_map() const;

    void log_term(term_index i, int depth = max_logging_depth);
    void log_bracketed_term(term_index i, int depth = max_logging_depth);
    void log_variables();

    static consteval identifier_index get_reserved_identifier_index(
        std::string_view s) {
        for (size_t i = 0; i < reserved_identifiers.size(); i++) {
            if (reserved_identifiers[i] == s) {
                return identifier_index{i};
            }
        }
        return identifier_index::invalid();
    }

private:
    template <prolog_term_type type>
    void validate_type(term_index index);
    prolog_term& get_term(term_index index);
    prolog_struct& get_struct(term_index index);
    prolog_var& get_var(term_index index);
    prolog_clause& get_clause(clause_index index);
    int get_integer(term_index index);

    term_index add_structure(node& node_instance);
    term_index add_integer(int integer);
    term_index add_variable(std::string& variable_name);
    identifier_index add_identifier(const std::string& name);

    clause_index duplicate_clause(clause_index index);
    term_index duplicate_term(term_index index,
                              std::unordered_map<term_index, term_index>&
                              variable_map);
    term_index duplicate_structure(term_index index,
                                   std::unordered_map<term_index, term_index>&
                                   variable_map);
    term_index duplicate_variable(term_index index,
                                  std::unordered_map<term_index, term_index>&
                                  variable_map);

    bool unify(term_index i, term_index j);
    bool unify_ground(term_index i, term_index j);
    void unify_unbound_variables(term_index i, term_index j);
    void unify_unbound_variable_ground(term_index i, term_index j);
    term_index resolve_bound_variable(term_index variable_index);

    void unwind_term_vector(term_index i);
    void unwind_clause_vector(clause_index i);
    void unwind_trail(trail_index i);

    void log_clauses();

    void log_structure(term_index structure_index, int depth);
    void log_variable(term_index variable_index);

    void log_list(term_index list_index, int depth);
    void log_clause(clause_index idx, int depth = max_logging_depth);
    void log_compound_term(term_index i, int depth, char seperator);
    void log_infix_term(term_index i, int depth,
                        const std::string& infix_operator);


    [[nodiscard]] bool is_compound_term(term_index i);
    [[nodiscard]] bool is_infix_term(term_index i);

    term_index evaluate_and_create_arithmetic_term(term_index i);
    int evaluate_arithmetic_term(term_index i);

    prolog_timestamp get_timestamp();
    void apply_timestamp(prolog_timestamp timestamp);

    std::vector<prolog_term> term_vector;
    std::vector<std::string> identifier_vector;
    std::vector<prolog_clause> clause_vector;
    std::vector<std::pair<clause_index, clause_index>> identifier_clause_map;
    // Maps identifier index to start of clause

    std::unordered_map<std::string, term_index> name_variable_map;
    std::unordered_map<term_index, std::string> variable_name_map;
    std::unordered_map<std::string, identifier_index> identifier_map;

    static constexpr int max_logging_depth = 20;

    static constexpr std::array<std::string_view, 18> reserved_identifiers{
        ",", ";", "!", ".", "[]",
        "+", "-", "*", "/", "is",
        "=", "\\=", "\\+", "<", ">",
        "=<", ">=", "halt"
    };

    // For the operation of the interpreter:
    std::vector<term_index> trail;
};


#endif //UNIFICATION_H
