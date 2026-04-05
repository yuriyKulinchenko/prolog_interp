#include <iostream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"
#include "unification_environment.h"

int main() {
    unification_environment environment {};
    lexer lexer(read_file("../source.txt"));
    parser parser(lexer.run());
    std::vector<node> clauses = parser.program();

    environment.add_clauses(clauses);
    environment.run_interpreter();
}