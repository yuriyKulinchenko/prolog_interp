#ifndef UNIFICATION_H
#define UNIFICATION_H

#include <vector>
#include <unordered_map>
#include "parser.h"
#include "prolog_term.h"

#define UNIFICATION_DEBUG

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
*/


/*

Current problem: Store and instantiate goals
- Goals are stored as a head and a body
- Suppose that I have the goal: p(X,Y) :- q(X), r(Y)
- When I want to instantiate p(A,B), I want to create a copy of the goal:
- p(v1,v2) :- q(v1), r(v2)
- And then attempt to unify p(A,B) with p(v1,v2)
- I can then add q(v1), r(v2) to the goal stack
- When adding a goal, the name mapping should be cleared

*/

class unification_environment {
public:
    // Adds the term specified by the passed node to the term_vector
    // Returns a pair specifying if the term is a raw term or variable, and the corresponding index

    int add_node(node& node);
    int add_clause(node& node);
    void add_clauses(std::vector<node>& nodes);

    void test_unification();
    void test_clauses(const std::string& path);

private:
    int add_structure(node& node_instance);
    int add_variable(std::string& variable_name);
    int add_identifier(const std::string& name);

    bool unify(int i, int j);
    bool unify_structures(int i, int j);
    void unify_unbound_variables(int i,int j);
    void unify_unbound_variable_term(int i, int j);
    int resolve_bound_variable(int variable_index);

    void unwind_term_vector(int i);
    void unwind_trail(int i);

    std::ostream& log_variables(std::ostream& stream);
    std::ostream& log_clauses(std::ostream& stream);
    std::ostream& log_term(std::ostream& stream, int term_index, int depth = max_logging_depth);
    std::ostream& log_clause(std::ostream& stream, int clause_index, int depth = max_logging_depth);
    std::ostream& log_structure(std::ostream& stream, int structure_index, int depth);
    std::ostream& log_variable(std::ostream& stream, int variable_index, int depth);

    std::vector<prolog_term> term_vector;
    std::vector<std::string> identifier_vector;
    std::vector<prolog_clause> clause_vector;

    std::unordered_map<std::string, int> name_variable_map;
    std::unordered_map<int, std::string> variable_name_map;
    std::unordered_map<std::string, int> identifier_map;

    static constexpr int max_logging_depth = 20;

    // For the operation of the interpreter:
    std::vector<int> trail;
};



#endif //UNIFICATION_H
