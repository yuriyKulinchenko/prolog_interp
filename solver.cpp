//
// Created by Yuriy Kulinchenko on 04/04/2026.
//

#include "solver.h"
#include "iostream"

[[deprecated]]
void solver::solve(term_index goal_index) {
    found_all = false;
    goal_stack.clear();
    history.clear();
    goal_stack.emplace_back(goal_index, 0);
}

[[deprecated]]
void solver::log_state(int continuation) {
    std::cout << "Continuation: " << continuation << '\n';
    std::cout << "Goals: ";
    log_goal_stack(goal_stack);
    std::cout << '\n';
    std::cout << "History: ";
    log_history();
    std::cout << "\n";
}

#define BACKTRACK()\
do {                                        \
if (history.empty()) {                      \
    found_all = true;                       \
    return *this;                           \
}                                           \
continuation = restore_decision_point();    \
} while(false)


[[deprecated]]
solver& solver::operator++() {
    int continuation = 0;

#ifdef SOLVER_DEBUG
    bool first_log = true;
    log_state(continuation);
    std::cout << '\n';
#endif

    // Edge case handling:
    if (goal_stack.empty()) {
        BACKTRACK();
#ifdef SOLVER_DEBUG
        first_log = false;
#endif
    }

    while (!goal_stack.empty()) {

#ifdef SOLVER_DEBUG
        if (first_log) {
            first_log = false;
        } else {
            log_state(continuation);
            std::cout << '\n';
        }
#endif

        // Identify the goal, and attempt to make progress:

        term_index goal_index = environment.resolve_bound_variable(
            peek_goal().term_idx);
        if (environment.term_vector[goal_index.raw()].is_integer()) {
            // Failure state:
            BACKTRACK();
            continue;
        }
        identifier_index id = environment.get_struct(goal_index).index;

        // Check for special cases:

        switch (id.raw()) {
        case unification_environment::get_reserved_identifier_index(",").raw()
        : {
            goal_stack.pop_back();
            prolog_struct& conjunction_structure = environment.get_struct(
                goal_index);
            size_t num_children = conjunction_structure.num_children;
            for (int i = static_cast<int>(num_children) - 1; i >= 0; i--) {
                goal_stack.emplace_back(
                    conjunction_structure[static_cast<size_t>(i)],
                    peek_goal().cut_barrier);
            }
            continue;
        }

        case unification_environment::get_reserved_identifier_index(";").raw()
        : {
            // Decision points must be placed:
            size_t num_children = environment.get_struct(goal_index).
                                              num_children;

            // decision point can be placed:
            if (continuation < static_cast<int>(num_children) - 1) {
                // Create copy of goal_stack:
                auto goal_stack_copy{goal_stack};
                history.emplace_back(get_timestamp(), goal_stack_copy,
                                     continuation + 1);
            }

            // Add the relevant child:
            int cut_barrier = peek_goal().cut_barrier;
            goal_stack.pop_back();
            goal_stack.emplace_back(
                environment.get_struct(goal_index)[static_cast<size_t>(
                    continuation)],
                cut_barrier);

            continuation = 0;
            continue;
        }

        case unification_environment::get_reserved_identifier_index("!").raw()
        : {
            // Remove the cut, adjust the history:
            int i = peek_goal().cut_barrier;
            goal_stack.pop_back();
            history.erase(history.begin() + i, history.end());
            continue;
        }

        case unification_environment::get_reserved_identifier_index("=").raw()
        : {
            term_index left = environment.get_struct(goal_index)[0];
            term_index right = environment.get_struct(goal_index)[1];
            bool success = environment.unify(left, right);
            if (success) {
                goal_stack.pop_back();
            } else {
                BACKTRACK();
            }
            continue;
        }

        case unification_environment::get_reserved_identifier_index("is").raw()
        : {
            term_index left = environment.get_struct(goal_index)[0];
            term_index right = environment.evaluate_and_create_arithmetic_term(
                environment.get_struct(goal_index)[1]);
            bool success = environment.unify(left, right);
            if (success) {
                goal_stack.pop_back();
            } else {
                BACKTRACK();
            }
            continue;

        }

        case unification_environment::get_reserved_identifier_index("<").raw()
        : {
            int left = environment.evaluate_arithmetic_term(
                environment.get_struct(goal_index)[0]);
            int right = environment.evaluate_arithmetic_term(
                environment.get_struct(goal_index)[1]);
            bool success = left < right;
            if (success) {
                goal_stack.pop_back();
            } else {
                BACKTRACK();
            }
            continue;
        }

        case unification_environment::get_reserved_identifier_index(">").raw()
        : {
            int left = environment.evaluate_arithmetic_term(
                environment.get_struct(goal_index)[0]);
            int right = environment.evaluate_arithmetic_term(
                environment.get_struct(goal_index)[1]);
            bool success = left > right;
            if (success) {
                goal_stack.pop_back();
            } else {
                BACKTRACK();
            }
            continue;
        }

        case unification_environment::get_reserved_identifier_index("fail").
        raw(): {
            BACKTRACK();
            continue;
        }

        case unification_environment::get_reserved_identifier_index("halt").
        raw(): {
            throw std::logic_error("EXECUTION HALTED");
        }

        default:


        }

        auto clause_range = environment.identifier_clause_map[id.raw()];

        if (clause_range.second != clause_index{0}) {
            clause_range = environment.identifier_clause_map[id.raw()];
        }

        clause_index lower_bound = clause_range.first;
        clause_index upper_bound = clause_range.second;
        int choice_count = static_cast<int>(
            upper_bound.raw() - lower_bound.raw());

        bool progress_made = false;
        for (int choice = continuation; choice < choice_count && !progress_made;
             choice++) {
            // Attempt each clause:
            bool add_decision_point = choice < choice_count - 1;
            bool success =
                apply_clause(lower_bound, choice, add_decision_point);
            if (success)
                progress_made = true;
        }

        if (progress_made) {
            continuation = 0;
            continue;
        }

        // Otherwise, backtracking is necessary:
        BACKTRACK();
    }

    return *this;
}

#undef BACKTRACK

[[deprecated]]
bool solver::operator*() {
    return goal_stack.empty();
}

[[deprecated]]
bool solver::at_end() const {
    return found_all;
}


[[deprecated]]
solver::goal solver::pop_goal() {
    goal return_index = goal_stack[goal_stack.size() - 1];
    goal_stack.pop_back();
    return return_index;
}

[[deprecated]]
solver::goal solver::peek_goal() {
    return goal_stack[goal_stack.size() - 1];
}

[[deprecated]]
solver::decision_point solver::pop_history() {
    decision_point return_point = history[history.size() - 1];
    history.pop_back();
    return return_point;
}

[[deprecated]]
bool solver::apply_clause(clause_index clause_idx, int choice_number,
                          bool add_decision_point) {
    clause_index final_idx = clause_idx + static_cast<size_t>(choice_number);
    prolog_timestamp timestamp = get_timestamp();
    goal goal_instance = pop_goal();

    clause_index dup_idx = environment.duplicate_clause(final_idx);
    prolog_clause& duplicate_clause = environment.get_clause(dup_idx);

    if (!environment.unify(goal_instance.term_idx, duplicate_clause.head)) {
        apply_timestamp(timestamp);
        goal_stack.push_back(goal_instance);
        return false;
    }

    if (add_decision_point) {
        std::vector<goal> saved_stack = goal_stack;
        saved_stack.push_back(goal_instance);
        history.emplace_back(timestamp, saved_stack, choice_number + 1);
    }

    if (duplicate_clause.body == term_index::invalid())
        return true;

    prolog_term& body = environment.term_vector[duplicate_clause.body.raw()];

    if (body.is_structure() &&
        body.structure().index ==
        unification_environment::get_reserved_identifier_index(",")) {
        prolog_struct& conjunction = body.structure();
        size_t num_children = conjunction.num_children;
        for (int i = static_cast<int>(num_children) - 1; i >= 0; --i) {
            goal_stack.emplace_back(conjunction[static_cast<size_t>(i)],
                                    goal_instance.cut_barrier);
        }
    } else {
        goal_stack.emplace_back(duplicate_clause.body,
                                goal_instance.cut_barrier);
    }

    return true;
}

[[deprecated]]
int solver::restore_decision_point() {
    decision_point point = pop_history();
    apply_timestamp(point.timestamp);
    goal_stack = point.goal_stack;
    return point.next_choice_number;
}

[[deprecated]]
prolog_timestamp solver::get_timestamp() {
    return {
        term_index{environment.term_vector.size()},
        clause_index{environment.clause_vector.size()},
        trail_index{environment.trail.size()}
    };
}

[[deprecated]]
void solver::apply_timestamp(prolog_timestamp timestamp) {
    environment.unwind_term_vector(timestamp.term_index_);
    environment.unwind_clause_vector(timestamp.clause_index_);
    environment.unwind_trail(timestamp.trail_index_);
}

[[deprecated]]
void solver::log_goal(goal goal_instance) {
    // std::cout << '{';
    environment.log_term(goal_instance.term_idx);
    // << ", " << goal_instance.cut_barrier << '}';
}

[[deprecated]]
void solver::log_goal_stack(std::vector<goal> goal_stack_instance) {
    std::cout << '[';
    if (!goal_stack_instance.empty()) {
        log_goal(goal_stack_instance[0]);
        for (int i = 1; i < static_cast<int>(goal_stack_instance.size()); i++) {
            std::cout << ", ";
            log_goal(goal_stack_instance[i]);
        }
    }
    std::cout << ']';
}

[[deprecated]]
void solver::log_decision_point(decision_point& point) {
    std::cout << '{';
    log_goal_stack(point.goal_stack);
    std::cout << ", next_choice=" << point.next_choice_number;
    std::cout << '}';
}

[[deprecated]]
void solver::log_history() {
    std::cout << '[';
    if (!history.empty()) {
        log_decision_point(history[0]);
        for (int i = 1; i < static_cast<int>(history.size()); i++) {
            std::cout << ", ";
            log_decision_point(history[i]);
        }
    }
    std::cout << ']';
}
