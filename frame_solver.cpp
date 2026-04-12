/*
Notes for tomorrow:

There is a clean abstraction somewhere here.
Frames with a continuation of 0 will always have to be evaluated,
even if last_popped = true.

If there exists a sequence of frames with non-zero continuations,
then it makes sense that they can all be collapsed upon just one pop:
this represents a sequence of rule invocations that have all succeeded.

The VM is effectively in 2 modes at any given time: growing the stack,
when last_popped = false, and shrinking the stack, when last_popped = true.

Rules:
Rules can have a range of continuation values. For any of their non-zero
continuation values, they will still be popped off if last_popped = true.

If a rule progresses further, i.e. it passes the head unification, a decision point
is dropped, and the body is placed on the stack. If a head unification fails, 2 things
can happen:

If there are decision points left, this is fine: no explicit decision point is added.
However, if there are no decision points left, a previous one must be restored.

Decision points introduced by rules are only done so on successful clause invocation.

This abstraction allows disjunctions to work almost identically to rules: the rough
continuation structure is the same

Cuts are still a headache, but a similar solution to the original solver can likely be used here

 */



#include "frame_solver.h"
#include <print>
#include <iostream>

#define CASE(s) case unification_environment::get_reserved_identifier_index(s)

void frame_solver::solve(int goal_index) {
    found_all = false;
    stack.clear();
    history.clear();
    add_goal(goal_index);
}

#define BACKTRACK()\
do {                                            \
    last_popped = false;                        \
    if (history.empty()) {                      \
        found_all = true;                       \
        return *this;                           \
    }                                           \
    restore_decision_point();                   \
} while(false)                                  \

frame_solver &frame_solver::operator++() {

    bool last_popped = false;

    if (stack.empty()) {
        log_frame_stack(stack);
        log_history();
        BACKTRACK();
    }


    while (!stack.empty()) {

        // DEBUG:
        log_frame_stack(stack);
        log_history();


        frame& top_frame = stack.top();
        prolog_structure& structure = environment.get_structure(top_frame.index);

        // Fall-through success state:
        if (last_popped && top_frame.continuation != 0) {
            stack.pop_back();
            continue;
        }

        last_popped = false;

        switch (top_frame.type) {
            using enum frame_type;

            case RULE: {
                // Clause invocation must be attempted:

                last_popped = false;

                if (!environment.identifier_clause_map.contains(structure.identifier_index)) {
                    BACKTRACK();
                    continue;
                }

                auto [low, high] =
                    environment.identifier_clause_map[structure.identifier_index];

                int duplicate_clause_index = environment.duplicate_clause(low + top_frame.continuation);
                prolog_clause& clause = environment.get_clause(duplicate_clause_index);

                top_frame.continuation++;
                bool exhausted_continuation = top_frame.continuation == high - low;

                // Unification with the head is attempted:

                if (environment.unify(top_frame.index, clause.head)) {

                    // A decision point is only added if another choice remains:

                    if (!exhausted_continuation) {
                        add_decision_point();
                    }

                    // If there is no body, success is immediate: the frame can be popped.

                    if (clause.body == -1) {
                        stack.pop_back();
                        last_popped = true;
                    } else {
                        add_goal(clause.body);
                    }
                } else if (exhausted_continuation) {
                    BACKTRACK();
                }
                continue;
            }

            case CONJUNCTION: {
                top_frame.continuation++;

                // Add everything to the stack:
                size_t num_children = structure.children.size();
                for (size_t i = num_children; i-- > 0;) {
                    add_goal(structure.children[i]);
                }

                continue;
            }

            case DISJUNCTION: {
                int term_index = structure.children[top_frame.continuation];
                top_frame.continuation++;

                if (top_frame.continuation != structure.children.size()) {
                    add_decision_point();
                }

                add_goal(term_index);
                continue;
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

void frame_solver::log_frame(frame &frame_) {
    printf("(");
    switch (frame_.type) {
        using enum frame_type;
        case CONJUNCTION:
        case DISJUNCTION:
            environment.log_bracketed_term(std::cout, frame_.index);
            break;
        case RULE:
            environment.log_term(std::cout, frame_.index);
            break;
        default:
            throw std::logic_error("Error: Unexpected frame type");
    }
    std::print(", {})", frame_.continuation);
}

void frame_solver::log_frame_stack(frame_stack &frame_stack_) {
#ifdef FRAME_SOLVER_DEBUG
    printf("[");
    for (int i = 0; i < frame_stack_.size(); i++) {
        if (i > 0) printf(", ");
        log_frame(frame_stack_[i]);
    }
    printf("]\n");
#endif
}

void frame_solver::log_history() {
#ifdef FRAME_SOLVER_DEBUG
    printf("[");
    for (int i = 0; i < history.size(); i++) {
        if (i > 0) printf(", ");
        std::print("{}", history[i].stack_pointer);
    }
    printf("]\n");
#endif
}

bool frame_solver::operator*() {
    return stack.empty();
}

bool frame_solver::at_end() const {
    return found_all;
}


#undef CASE