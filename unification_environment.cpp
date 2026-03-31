//
// Created by Yuriy Kulinchenko on 31/03/2026.
//

#include "unification_environment.h"
#include <iostream>

std::pair<prolog_term_type, int> unification_environment::add_node(node& node) {
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
        term.children.push_back(add_term(child));
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

void unification_environment::test_unification() {
    std::string s1, s2;
    std::cout << "Enter first term: ";
    std::cin >> s1;
    std::cout << "Enter second term: ";
    std::cin >> s2;

    // lexer lexer{s1};
    // lexer.run();
    // parser parser{lexer.run()};
    // lexer lexer;
    //
    // node n1 =
}
