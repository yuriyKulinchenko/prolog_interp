#ifndef UNIFICATION_H
#define UNIFICATION_H

#include <vector>
#include <unordered_map>
#include "helper.h"
#include "parser.h"

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

enum class prolog_variable_type {
    UNBOUND, BOUND_VARIABLE, BOUND_TERM
};

struct prolog_variable {
    explicit prolog_variable():
        type(prolog_variable_type::UNBOUND), index(0) {}

    prolog_variable(prolog_variable_type type, int index):
        type(type), index(index) {}

    prolog_variable_type type;
    int index; // Useful if bound
};

enum class prolog_term_type {
    VARIABLE, TERM
};

struct prolog_term {
    prolog_term(prolog_term_type type, int index):
        type(type), index(index) {}

    prolog_term_type type;
    int index; // Doubles as variable index OR identifier index
    std::vector<prolog_term> children; // Empty if variable
};

class unification_environment {
public:
    // Adds the term specified by the passed node to the term_vector
    // Returns a pair specifying if the term is a raw term or variable, and the corresponding index

    std::pair<prolog_term_type, int> add_node(node& node) {
        switch (node.type) {
            using enum node_type;
            case TERM: return {prolog_term_type::TERM, add_term(node)};
            case VARIABLE: return {prolog_term_type::VARIABLE, add_variable(node.name)};
            default: throw std::logic_error("Passed node is not a term or variable");
        }
    }

private:

    int add_term(node& node) {
        // Few challenges here:
        // - Figure out identifier map
        // - Recursively map children
        // - Form constructed term
        // - Return index


    }

    int add_variable(std::string& variable_name) {
        if (name_variable_map.contains(variable_name)) {
            return name_variable_map[variable_name];
        }
        // Otherwise, this name is not yet mapped:
        variable_vector.emplace_back();
        int variable_index = static_cast<int>(variable_vector.size()) - 1;
        name_variable_map[variable_name] = variable_index;
        return variable_index;
    }

    std::unordered_map<std::string, int> name_variable_map;
    std::vector<prolog_variable> variable_vector;
    std::vector<prolog_term> term_vector;
    std::unordered_map<std::string, int> identifier_map;
    std::vector<std::string> identifier_vector;
};



#endif //UNIFICATION_H
