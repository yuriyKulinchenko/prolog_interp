//
// Created by Yuriy Kulinchenko on 11/04/2026.
//

#include "frame_solver.h"

#define CASE(s) case unification_environment::get_reserved_identifier_index(s)

void frame_solver::solve(int goal_index) {
    found_all = false;
    stack.clear();
    history.clear();
    add_goal(goal_index);
}

frame_solver &frame_solver::operator++() {
    if (stack.empty()) {
        if (!history.empty()) {
            restore_decision_point();
        }
    }

    while (!stack.empty()) {
        frame& top_frame = stack.top();
        switch (top_frame.type) {
            using enum frame_type;

            case RULE: {
                prolog_structure& structure = environment.get_structure(top_frame.index);
                auto [low, high] =
                    environment.identifier_clause_map[structure.identifier_index];

                if (top_frame.continuation == high) {
                    stack.pop_back();
                    continue;
                }

                // Clause invocation needs to occur:
                int duplicate_clause_index = environment.duplicate_clause(low + top_frame.continuation);
                prolog_clause& clause = environment.get_clause(duplicate_clause_index);


                top_frame.continuation++;

                if (environment.unify(top_frame.index, clause.head)) {
                    // If the clause is a fact, the frame can be popped:
                    add_decision_point();
                    if (clause.body == -1) {
                        stack.pop_back();
                    } else {
                        add_goal(clause.body);
                    }
                }
                continue;
            }

            case CONJUNCTION: {
                    if (top_frame.continuation == 1) {
                        stack.pop_back();
                    }
                    prolog_structure& structure = environment.get_structure(top_frame.index);



                    continue;
            }

            case DISJUNCTION: {

            }

            default: {
               throw std::logic_error("ERROR: Not implemented");
            }
        }
    }

    return *this;
}


void frame_solver::add_goal(int goal_index) {
    prolog_term& term = environment.term_vector[goal_index];


    if (!term.is_structure()) {
        throw std::logic_error("ERROR: Cannot resolve non-structure goal");
    }

    prolog_structure& structure = term.structure();
    switch (structure.identifier_index) {
        using enum frame_type;
        CASE(","): {
            stack.emplace_back(CONJUNCTION, goal_index);
            return;
        }

        CASE(";"): {
            stack.emplace_back(DISJUNCTION, goal_index);
            return;
        }

        CASE("!"): {
            stack.emplace_back(CUT);
            return;
        }

        default: {
            stack.emplace_back(RULE, goal_index);
        }
    }
}

prolog_timestamp frame_solver::get_timestamp() {
    return
    {
        static_cast<int>(environment.term_vector.size()),
        static_cast<int>(environment.clause_vector.size()),
        static_cast<int>(environment.trail.size()),
    };
}

void frame_solver::add_decision_point() {
    history.emplace_back(get_timestamp(), stack.size());
}

void frame_solver::restore_decision_point() {
    frame_decision_point decision_point = history[history.size() - 1];
    history.pop_back();
    environment.unwind_term_vector(decision_point.timestamp.term_index);
    environment.unwind_clause_vector(decision_point.timestamp.clause_index);
    environment.unwind_trail(decision_point.timestamp.trail_index);
    stack.restore(decision_point.stack_pointer);
}



#undef CASE