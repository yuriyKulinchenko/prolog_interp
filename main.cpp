#include "frame_solver.h"
#include "frame_solver_types.h"
#include "helper.h"
#include "lexer_parser.h"
#include "unification_environment.h"

#define PATH "../examples/source.txt"

void run_interpreter(unification_environment& env, lexer_parser& frontend) {
    prolog_timestamp timestamp = env.get_timestamp();
    for (;;) {
        env.apply_timestamp(timestamp);
        env.get_name_variable_map().clear();
        std::cout << "?- ";
        std::string s;
        std::getline(std::cin, s);
        if (s.empty()) continue;
        try {
            node n = frontend.query(s);

            frame_solver solver{env};
            solver.solve(env.add_node(n));

            while (solver.next()) {
                std::println("{}true{}", GREEN, RESET);
                env.log_variables();
                std::string command;
                std::getline(std::cin, command);
            }

        } catch (frame_solver_types::prolog_user_error& e) {
            if (std::string{e.what()} == "halt/0: execution halted by user") {
                return;
            }
            std::println("{}{}{}", RED, e.what(), RESET);
        }
    }
}

void test_step(unification_environment& env, lexer_parser& frontend) {
    using namespace frame_solver_types;
    std::string query_string;
    std::cout << "?- ";
    std::cin >> query_string;
    node query = frontend.query(query_string);

    frame_solver solver{env};
    solver.solve(env.add_node(query));
    for (int i = 1;; i++) {
        std::println("STEP NUMBER: {}", i);
        step_result res = solver.step();
        std::println("STEP TYPE: {}", step_type_to_string_(res.type));
        if (res.type == step_type::FINISH) break;
    }
}

int main() {
    unification_environment environment{};
    lexer_parser frontend{};
    std::vector<node> clauses = frontend.program(read_file(PATH));
    environment.add_clauses(clauses);

    run_interpreter(environment, frontend);
    return 0;
}
