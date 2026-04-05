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

// return happens in only 2 cases: when a solution is found,
// or when all possible solutions are exhausted

solver &solver::operator++() {
    int continuation = 0;
    // Edge case handling:
    if (goal_stack.empty()) {
        if (history.empty()) {
            found_all = true;
            return *this;
        }
        continuation = restore_decision_point();
    }

    while (!goal_stack.empty()) {
#ifdef SOLVER_DEBUG
        std::cout << "Continuation: " << continuation << '\n';
        std::cout << "Goals: ";
        log_goal_stack();
        std::cout << '\n';
        std::cout << "History: ";
        log_history();
        std::cout << "\n\n";
#endif

        // Identify the goal, and attempt to make progress:

        int goal_index = environment.resolve_bound_variable(peek_goal());
        int identifier_index = environment.get_structure(goal_index).identifier_index;
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
            bool success = apply_clause(lower_bound, choice);
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
    history.pop_back();
    return return_point;
}


bool solver::apply_clause(int clause_index, int choice_number, bool add_decision_point) {
    clause_index = clause_index + choice_number;
    prolog_timestamp timestamp = get_timestamp();
    int goal_index = pop_goal();

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
        goal_stack[goal_stack.size() - 1] = goal_index;
        return false;
    }

    // If successful, add decision point and clause body:

    if (add_decision_point)
        history.emplace_back(timestamp, goal_index, choice_number + 1);

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
    goal_stack[goal_stack.size() - 1] = point.goal_index;
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

void solver::log_decision_point(decision_point &point) {
    std::cout << '{';
    environment.log_term(std::cout, point.goal_index)
    << ", " << point.next_choice_number;
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

