#include <iostream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"
#include "unification_environment.h"

int main() {
    unification_environment environment {};
    lexer lexer(read_file("../examples/source.txt"));
    parser parser(lexer.run(), lexer);
    std::vector<node> clauses = parser.program();

    environment.add_clauses(clauses);
    try {
        environment.run_interpreter();
    } catch (std::logic_error& e) {
        if (std::string(e.what()) == "EXECUTION HALTED") {
            std::println("Execution halted");
            return 0;
        }
        throw;
    }
}