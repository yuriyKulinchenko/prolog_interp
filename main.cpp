#include <iostream>
#include "helper.h"
#include  "lexer.h"

int main() {
    std::string input = "Variable, symbol, :- ([VariableA|])";
    lexer lexer {input};
    auto vec = lexer.run();
    std::cout << vec;
}