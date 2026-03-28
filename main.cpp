#include <iostream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"

int main() {
    std::string input = "p :- q, r; s, t.";
    lexer lexer {input};
    auto vec = lexer.run();
    parser parser {vec};
    auto clauses = parser.run();
    std::cout << clauses[0];
}