#include "parser.h"
#include <iostream>
#include <optional>

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

std::vector<node> parser::run() {

}

std::vector<node> parser::program() {

}

// Clause ::= Term ':-' GoalExpr '.' | Term '.'

node parser::clause() {
    node term_instance = term();
    std::optional<node> goal_instance;
    if (match(token_type::RULE_OPERATOR))
        goal_instance = goalExpr();
    consume(token_type::DOT, "ERROR: Expect '.' after clause");
    if (goal_instance) {
        return {.type = node_type::CLAUSE, .children = {term_instance, *goal_instance}};
    }
    return {.type = node_type::CLAUSE, .children = {term_instance}};
}

node parser::goalExpr() {

}

node parser::disjunction() {

}

node parser::conjunction() {

}
node parser::simpleGoal() {

}
node parser::term() {

}
node parser::list() {

}
node parser::elements() {

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
    if (peek().type != type) throw std::logic_error(error_message);
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
