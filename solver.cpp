//
// Created by Yuriy Kulinchenko on 04/04/2026.
//

#include "solver.h"
#include "iostream"

void solver::solve(int goal_index) {
    found_all = false;
    goal_stack.clear();
    history.clear();
    goal_stack.emplace_back(goal_index, 0);
}

void solver::log_state(int continuation) {
    std::cout << "Continuation: " << continuation << '\n';
    std::cout << "Goals: ";
    log_goal_stack(goal_stack);
    std::cout << '\n';
    std::cout << "History: ";
    log_history();
    std::cout << "\n";
}


solver &solver::operator++() {
    int continuation = 0;

#ifdef SOLVER_DEBUG
    bool first_log = true;
    log_state(continuation);
    std::cout << '\n';
#endif


    // Edge case handling:
    if (goal_stack.empty()) {
        if (history.empty()) {
            found_all = true;
            return *this;
        }
        continuation = restore_decision_point();
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

        int goal_index = environment.resolve_bound_variable(peek_goal().term_index);
        int identifier_index = environment.get_structure(goal_index).identifier_index;

        // Check for special cases:
        
        switch (identifier_index) {
            case unification_environment::get_reserved_identifier_index(","): {
                goal_stack.pop_back();
                prolog_structure& conjunction_structure = environment.get_structure(goal_index);
                for (int child_index: conjunction_structure.children) {
                    goal_stack.emplace_back(child_index, peek_goal().cut_barrier);
                }
                continue;
            }

            case unification_environment::get_reserved_identifier_index(";"): {
                // Decision points must be placed:
                std::vector<int>& disjunction_children = environment.get_structure(goal_index).children;

                // decision point can be placed:
                if (continuation < disjunction_children.size() - 1) {
                    // Create copy of goal_stack:
                    auto goal_stack_copy {goal_stack};
                    history.emplace_back(get_timestamp(), goal_stack_copy, continuation + 1);
                }

                // Add the relevant child:
                int cut_barrier = peek_goal().cut_barrier;
                goal_stack.pop_back();
                goal_stack.emplace_back(disjunction_children[continuation], cut_barrier);

                continuation = 0;
                continue;
            }

            case unification_environment::get_reserved_identifier_index("!"): {
                // Remove the cut, adjust the history:
                int i = peek_goal().cut_barrier;
                goal_stack.pop_back();
                history.erase(history.begin() + i, history.end());
                continue;
            }

            default:
        }

        std::pair clause_range = {0, 0};

        if (environment.identifier_clause_map.contains(identifier_index)) {
            clause_range = environment.identifier_clause_map[identifier_index];
        }

        int lower_bound = clause_range.first;
        int upper_bound = clause_range.second;
        int choice_count = upper_bound - lower_bound;


        bool progress_made = false;
        for (int choice = continuation; choice < choice_count && !progress_made; choice++) {
            // Attempt each clause:
            bool add_decision_point = choice < choice_count - 1;
            bool success = apply_clause(lower_bound, choice, add_decision_point);
            if (success) progress_made = true;
        }

        if (progress_made) {
            continuation = 0;
            continue;
        }

        // Otherwise, backtracking is necessary:

        if (history.empty()) {
            found_all = true;
            return *this;
        }

        continuation = restore_decision_point();
    }

    return *this;
}

bool solver::operator*() {
    return goal_stack.empty();
}

bool solver::at_end() const {
    return found_all;
}

goal solver::pop_goal() {
    goal return_index = goal_stack[goal_stack.size() - 1];
    goal_stack.pop_back();
    return return_index;
}

goal solver::peek_goal() {
    return goal_stack[goal_stack.size() - 1];
}


decision_point solver::pop_history() {
    decision_point return_point = history[history.size() - 1];
    history.pop_back();
    return return_point;
}
bool solver::apply_clause(int clause_index, int choice_number, bool add_decision_point) {
    clause_index = clause_index + choice_number;
    prolog_timestamp timestamp = get_timestamp();
    goal goal_instance = pop_goal();

    int duplicate_clause_index = environment.duplicate_clause(clause_index);
    prolog_clause& duplicate_clause = environment.get_clause(duplicate_clause_index);

    if (!environment.unify(goal_instance.term_index, duplicate_clause.head)) {
        apply_timestamp(timestamp);
        goal_stack.push_back(goal_instance);
        return false;
    }

    if (add_decision_point) {
        std::vector<goal> saved_stack = goal_stack;
        saved_stack.push_back(goal_instance);
        history.emplace_back(timestamp, saved_stack, choice_number + 1);
    }

    if (duplicate_clause.body == -1) return true;

    prolog_term& body = environment.term_vector[duplicate_clause.body];

    if (body.is_structure() &&
        body.as.structure.identifier_index == unification_environment::get_reserved_identifier_index(",")) {
        prolog_structure& conjunction = body.as.structure;
        for (int i = static_cast<int>(conjunction.children.size()) - 1; i >= 0; --i) {
            goal_stack.emplace_back(conjunction.children[i], goal_instance.cut_barrier);
        }
        } else {
            goal_stack.emplace_back(duplicate_clause.body, goal_instance.cut_barrier);
        }

    return true;
}

int solver::restore_decision_point() {
    decision_point point = pop_history();
    apply_timestamp(point.timestamp);
    goal_stack = point.goal_stack;
    return point.next_choice_number;
}

prolog_timestamp solver::get_timestamp() {
    return
    {
        static_cast<int>(environment.term_vector.size()),
        static_cast<int>(environment.clause_vector.size()),
        static_cast<int>(environment.trail.size()),
    };
}

void solver::apply_timestamp(prolog_timestamp timestamp) {
    environment.unwind_term_vector(timestamp.term_index);
    environment.unwind_clause_vector(timestamp.clause_index);
    environment.unwind_trail(timestamp.trail_index);
}

void solver::log_goal(goal goal_instance) {
    // std::cout << '{';
    environment.log_term(std::cout, goal_instance.term_index);
    // << ", " << goal_instance.cut_barrier << '}';
}


void solver::log_goal_stack(std::vector<goal> goal_stack_instance) {
    std::cout << '[';
    if (!goal_stack_instance.empty()) {
        log_goal(goal_stack_instance[0]);
        for (int i = 1; i < goal_stack_instance.size(); i++) {
            std::cout << ", ";
            log_goal(goal_stack_instance[i]);
        }
    }
    std::cout << ']';
}

void solver::log_decision_point(decision_point &point) {
    std::cout << '{';
    log_goal_stack(point.goal_stack);
    std::cout << ", next_choice=" << point.next_choice_number;
    std::cout << '}';
}


void solver::log_history() {
    std::cout << '[';
    if (!history.empty()) {
        log_decision_point(history[0]);
        for (int i = 1; i < history.size(); i++) {
            std::cout << ", ";
            log_decision_point(history[i]);
        }
    }
    std::cout << ']';
}

