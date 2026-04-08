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

Term ::= Sum (('=','is') Sum)?

Sum ::= Product (('+'|'-') Sum)?

Product ::= SimpleTerm (('*'|'/') Product)?

SimpleTerm ::= variable
       | integer
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

class parser {
public:
    parser(std::vector<token>&& token_vector, lexer& lexer_instance):
    token_vector(std::move(token_vector)), i(0), lexer_instance(lexer_instance) {}

    std::vector<node> run();
    void reset(std::vector<token>&& token_vector);


    std::vector<node> program();
    node clause();
    node goalExpr();
    node disjunction();
    node conjunction();
    node simpleGoal();
    node term();
    node sum();
    node product();
    node simple_term();
    node list(token& token_instance);
    std::vector<node> elements();
    node query();

    // Helper functions:

private:
    token& peek();
    token& previous();
    token& next();
    token& advance();
    token& advance(int n);
    token& consume(token_type type, const std::string& error_message, token& erroneous_token);
    bool check(token_type type);
    bool match(token_type type);
    bool at_end();

    std::logic_error parser_error(const std::string& error_message);
    std::logic_error parser_error(const std::string& error_message, const token& token_instance);

    int i;
    std::vector<token> token_vector;
    std::vector<node> goal_vector;

    lexer& lexer_instance;
};

std::ostream& operator<<(std::ostream& stream, node& node);

#endif //PARSER_H
