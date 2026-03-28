//
// Created by Yuriy Kulinchenko on 27/03/2026.
//

#ifndef PARSER_H
#define PARSER_H

#include <utility>
#include <vector>
#include "lexer.h"


/*

Program ::= Clause*

Clause ::= Term ':-' GoalExpr '.'
         | Term '.'

GoalExpr ::= Disjunction

Disjunction ::= Conjunction (';' Conjunction)*

Conjunction ::= SimpleGoal (',' SimpleGoal)*

SimpleGoal ::= Term
             | '(' GoalExpr ')'
             | '!'

Term ::= variable
       | identifier
       | identifier '(' Elements ')'
       | List

List ::= '[]'
       | '[' Elements ']'
       | '[' Elements '|' Term ']'

Elements ::= Term (',' Term)*

For now, the assumption is that only a set of rules will be parsed.
The querying will be done through the terminal

 */

#define NODE_TYPE_LIST(X)  \
X(CLAUSE)                  \
X(GOAL)                    \
X(TERM)                    \
X(VARIABLE)                \
X(CONJUNCTION)             \
X(DISJUNCTION)             \
X(CUT)


enum class node_type {
#define X(name) name,
    NODE_TYPE_LIST(X)
#undef X
};

inline std::string node_type_to_string(node_type type) {
    switch (type) {
#define X(name) case node_type::name: return #name;
        NODE_TYPE_LIST(X)
#undef X
    }
    return "UNKNOWN";
}

struct node {
    explicit node(node_type type): type(type) {}
    node(node_type type, std::string name):
        type(type), name(std::move(name)) {}
    node(node_type type, std::vector<node> children):
        type(type), children(std::move(children)) {}
    node(node_type type, std::string name, std::vector<node> children):
        type(type), name(std::move(name)), children(std::move(children)) {}

    node_type type;
    std::string name; // Optionally present
    std::vector<node> children;
};


class parser {

public:

    parser(std::vector<token>& token_list): token_vector(token_list), i(0) {}

    std::vector<node> run();

    std::vector<node> program();
    node clause();
    node goalExpr();
    node disjunction();
    node conjunction();
    node simpleGoal();
    node term();
    node list();
    std::vector<node> elements();

    // Helper functions:

    token& peek();
    token& next();
    token& advance();
    token& advance(int n);
    token& consume(token_type type, const std::string& error_message);
    bool check(token_type type);
    bool match(token_type type);
    bool at_end();

private:
    int i;
    std::vector<token>& token_vector;
    std::vector<node> goal_vector;
};



#endif //PARSER_H
