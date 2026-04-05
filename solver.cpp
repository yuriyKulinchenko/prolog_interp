//
// Created by Yuriy Kulinchenko on 04/04/2026.
//

#include "solver.h"
#include "iostream"

#define SOLVER_DEBUG


void solver::solve(int goal_index) {
    found_all = false;
    goal_stack.clear();
    goal_stack.push_back(goal_index);
}

/*

p :- q.
p :- r.
p :- s.

s :- a.
a :- b.
b.

goal_stack = [p, x, y, z], history = []
goal_stack = [q, x, y, z], history = [(p, next=1)]
goal_stack = [r, x, y, z], history = [(p, next=2)]
goal_stack = [s, x, y, z], history = []

*/

solver &solver::operator++() {

    if (goal_stack.empty()) {
        if (history.empty()) {
            // All possible solutions have been found:
            found_all = true;
            return *this;
        }

        // Another decision point to try:
        restore_decision_point();
    }

    while (!goal_stack.empty()) {
#ifdef SOLVER_DEBUG
    log_goal_stack();
    std::cout << '\n';
#endif
        // Query the top of the stack:
        int goal_index = peek_goal();
        goal_index = environment.resolve_bound_variable(goal_index);

        prolog_structure& structure = environment.get_structure(goal_index);

        // Check if range exists:

        std::pair range = {0, 0};

        if (environment.identifier_clause_map.contains(structure.identifier_index)) {
            range = environment.identifier_clause_map[structure.identifier_index];
        }

        bool add_decision_point = range.second - range.first > 1;

        bool found_success = false;
        for (int i = range.first; i < range.second && !found_success; i++) {
            int choice_number = i - range.first;
            if (apply_clause(range.first, choice_number, add_decision_point)) {
                found_success = true;
            }
        }

        if (found_success) continue;

        // Backtracking must now occur:

        if (history.empty()) {
            found_all = true;
            return *this; // No more backtracking can happen, failure state
        }
        restore_decision_point();

    }
    return *this;
}

bool solver::operator*() {
    return goal_stack.empty();
}

bool solver::at_end() {
    return found_all;
}

int solver::pop_goal() {
    int return_index = goal_stack[goal_stack.size() - 1];
    goal_stack.pop_back();
    return return_index;
}

int solver::peek_goal() {
    return goal_stack[goal_stack.size() - 1];
}


decision_point solver::pop_history() {
    decision_point return_point = history[history.size() - 1];
    goal_stack.pop_back();
    return return_point;
}


bool solver::apply_clause(int clause_index, int choice_number, bool add_decision_point) {
    clause_index = clause_index + choice_number;
    int goal_index = pop_goal();
    prolog_timestamp timestamp = get_timestamp();

#ifdef SOLVER_DEBUG
    prolog_structure& goal_structure = environment.get_structure(goal_index);
    prolog_clause& original_clause = environment.get_clause(clause_index);

    if (environment.get_structure(original_clause.head).identifier_index != goal_structure.identifier_index) {
        std::cerr << "Term: ";
        environment.log_term(std::cerr, goal_index) << " does not match rule head: ";
        environment.log_term(std::cerr, original_clause.head);
        throw std::logic_error("ERROR: rule instantiation failed");
    }
#endif

    int duplicate_clause_index = environment.duplicate_clause(clause_index);
    prolog_clause& duplicate_clause = environment.get_clause(duplicate_clause_index);

    // Attempt unification with the head:

    if (!environment.unify(goal_index, duplicate_clause.head)) {
        apply_timestamp(timestamp);
        return false;
    }

    // If successful, add decision point and clause body:

    if (add_decision_point)
        history.emplace_back(timestamp, goal_index, choice_number);

    // If the clause body is empty, we are done:

    if (duplicate_clause.body == -1) return true;

    prolog_term& body = environment.term_vector[duplicate_clause.body];

    if (body.is_structure() &&
        environment.identifier_vector[body.as.structure.identifier_index] == ",") {
        // Body is a conjunction of terms:
        prolog_structure& conjunction = body.as.structure;
        int num_children = static_cast<int>(conjunction.children.size());
        for (int i = num_children - 1; i >= 0; i--) {
            goal_stack.emplace_back(conjunction.children[i]);
        }
    } else {
        goal_stack.emplace_back(duplicate_clause.body);
    }

    return true;
}

int solver::restore_decision_point() {
    decision_point point = pop_history();
    apply_timestamp(point.timestamp);
    goal_stack[goal_stack.size() - 1] = point.timestamp.goal_stack_index;
    return point.next_choice_number;
}

prolog_timestamp solver::get_timestamp() {
    return
    {
        static_cast<int>(environment.term_vector.size()),
        static_cast<int>(environment.clause_vector.size()),
        static_cast<int>(environment.trail.size()),
        static_cast<int>(goal_stack.size())
    };
}

void solver::apply_timestamp(prolog_timestamp timestamp) {
    environment.unwind_term_vector(timestamp.term_index);
    environment.unwind_clause_vector(timestamp.clause_index);
    environment.unwind_trail(timestamp.trail_index);
    goal_stack.resize(timestamp.goal_stack_index, -1); // May increase the size
}

void solver::log_goal_stack() {
    std::cout << '[';
    if (!goal_stack.empty()) {
        environment.log_term(std::cout, goal_stack[0]);
        for (int i = 1; i < goal_stack.size(); i++) {
            std::cout << ", ";
            environment.log_term(std::cout, goal_stack[i]);
        }
    }
    std::cout << ']';
}
