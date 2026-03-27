//
// Created by Yuriy Kulinchenko on 27/03/2026.
//

#ifndef PARSER_H
#define PARSER_H

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

#define NODE_TYPE_LIST(X) \
X(CLAUSE)                  \
X(TERM)                    \
X(AND)                     \
X(OR)                      \
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
    node_type type;
    union {
        std::vector<node> children;
        // There may be more things in this union later
    };
};


class parser {

public:

    parser(std::vector<token>& token_list): token_vector(token_list), i(0) {}

    std::vector<node> run();

private:
    int i;
    std::vector<token>& token_vector;
    std::vector<node> goal_vector;
};



#endif //PARSER_H
