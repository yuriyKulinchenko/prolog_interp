#include <iostream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"

int main() {
    std::string input = "p :- q, r.";
    lexer lexer {input};
    auto vec = lexer.run();
    std::cout << vec << '\n';
    parser parser {vec};
    auto node = parser.run();
}