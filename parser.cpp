#include "parser.h"
#include <iostream>
#include <optional>




std::vector<node> parser::run() {
    return program();
}

std::vector<node> parser::program() {
    std::vector<node> clauses {};
    while (!at_end()) {
        clauses.push_back(clause());
    }
    return clauses;
}

// Clause ::= Term ':-' GoalExpr '.' | Term '.'

node parser::clause() {
    node term_instance = term();
    std::optional<node> goal_instance;
    if (match(token_type::RULE_OPERATOR))
        goal_instance = goalExpr();
    consume(token_type::DOT, "ERROR: Expect '.' after clause");

    node clause_instance = {node_type::CLAUSE, {term_instance}};
    if (goal_instance) clause_instance.children.push_back(*goal_instance);
    return clause_instance;
}

// GoalExpr ::= Disjunction

node parser::goalExpr() {
    return {node_type::GOAL, {disjunction()}};
}

// Disjunction ::= Conjunction (';' Conjunction)*

node parser::disjunction() {
    std::vector nodes {conjunction()};
    while (match(token_type::SEMI_COLON)) {
        nodes.push_back(conjunction());
    }

    return {node_type::DISJUNCTION, nodes};
}

// Conjunction ::= SimpleGoal (',' SimpleGoal)*

node parser::conjunction() {
    std::vector nodes {simpleGoal()};
    while (match(token_type::COMMA)) {
        nodes.push_back(simpleGoal());
    }
    return node{node_type::CONJUNCTION, nodes};
}

// SimpleGoal ::= Term | '(' GoalExpr ')' | '!'

node parser::simpleGoal() {
    if (match(token_type::EXCLAMATION_MARK)) {
        return node{node_type::CUT};
    }

    if (match(token_type::PAREN_OPEN)) {
        node inner = goalExpr();
        consume(token_type::PAREN_CLOSED, "ERROR: Unmatched bracket");
        return inner;
    }

    return term();
}

// Term ::= variable | identifier | identifier '(' Elements ')' | List

node parser::term() {
    if (check(token_type::VARIABLE)) {
        return {node_type::VARIABLE, advance().identifier};
    }

    // Parsing identifier:
    if (check(token_type::SYMBOL)) {
        std::string identifier_name = advance().identifier;
        std::optional<std::vector<node>> nodes;
        if (match(token_type::PAREN_OPEN)) {
            nodes = elements();
            consume(token_type::PAREN_CLOSED, "ERROR: Unmatched bracket");
        }

        node term_instance = {node_type::TERM, identifier_name};
        term_instance.children = nodes ? *nodes : std::vector<node>{};
        return term_instance;
    }

    // Parsing list:

    if (match(token_type::SQUARE_OPEN)) {
        return list();
    }

    throw std::logic_error("ERROR: Unrecognized term: term must be a variable, identifier or list");
}

node parser::list() {
    // Empty list:
    if (match(token_type::SQUARE_CLOSED)) {
        return {node_type::TERM, "[]"};
    }

    std::vector<node> nodes = elements();
    // Elements have to be converted into a linked list:
    node head = {node_type::TERM, ".", {nodes[0]}};
    std::vector<node>* tail = &head.children;
    for (int i = 1; i < nodes.size(); i++) {
        tail->push_back({node_type::TERM, ".", {nodes[i]}});
        tail = &(*tail)[1].children;
    }

    if (match(token_type::PIPE)) {
        tail->push_back(term());
    } else {
        tail->emplace_back(node_type::TERM, "[]");
    }

    return head;
}

std::vector<node> parser::elements() {
    std::vector nodes {term()};
    while (match(token_type::COMMA)) {
        nodes.push_back(term());
    }
    return nodes;
}

token& parser::peek() {
    return token_vector[i];
};

token& parser::next() {
    return token_vector[i + 1];
};

token& parser::advance() {
    return token_vector[i++];
};

token& parser::advance(int n) {
    return token_vector[i+=n];
};

token& parser::consume(token_type type, const std::string& error_message) {
    if (peek().type != type) {
        std::cerr << "parsing at: " << token_type_to_string(peek().type) << '\n';
        std::cerr << "token index: " << i << '\n';
        throw std::logic_error(error_message);
    }
    return advance();
};

bool parser::check(token_type type) {
    return peek().type == type;
}
bool parser::match(token_type type) {
    if (check(type)) {
        i++;
        return true;
    }
    return false;
}
bool parser::at_end() {
    return token_vector.size() == i;
}
