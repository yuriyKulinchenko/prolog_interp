#include "frame_solver.h"
#include "helper.h"
#include "lexer_parser.h"
#include "unification_environment.h"

#define PATH "../examples/source.txt"

void test_step(unification_environment& environment, lexer_parser& frontend) {
    using namespace frame_solver_types;
    std::string query_string;
    std::cout << "ENTER QUERY: ";
    std::cin >> query_string;
    node query = frontend.query(query_string);

    frame_solver solver{environment};
    solver.solve(environment.add_node(query));
    for (int i = 1;; i++) {
        std::println("STEP NUMBER: {}", i);
        step_type res = solver.step();
        std::println("STEP TYPE: {}", step_type_to_string_(res));
    }
}

void run_interpreter(unification_environment& environment) {
    try {
        environment.run_interpreter();
    } catch (std::logic_error& e) {
        if (std::string(e.what()) == "EXECUTION HALTED") {
            std::println("Execution halted");
            return;
        }
        throw;
    }
}


int main() {
    unification_environment environment{};
    lexer_parser frontend{};
    std::vector<node> clauses = frontend.program(read_file(PATH));
    environment.add_clauses(clauses);

    // test_step(environment, frontend);
    run_interpreter(environment);
    return 0;
}