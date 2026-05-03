#ifndef FRAME_SOLVER_H
#define FRAME_SOLVER_H

#include "frame_solver_types.h"
#include "unification_environment.h"
#include "strong_indices.h"

// #define FRAME_SOLVER_DEBUG

/*

The frame_solver class functions as a prolog interpreter heavily inspired by the execution
of the WAM (Warren Abstract Machine). frame_solver interoperates with unification_environment,
which provides all the needed facilities for prolog unification and clause lookup.

The class maintains 2 important pieces of state during the operation of the interpreter, on
top of what is maintained by the unification_environment: the frame stack ('stack'),
and the choice point stack ('choices'). The frame stack maintains the current execution
state, including all explored conjunction branches - these need to be maintained for
the sake of efficient backtracking.

Each frame on the frame stack represents a goal invocation - there are a few types of
goal, including 'Rule', 'Conjunction' and 'Disjunction'. When a new rule is invoked,
it is added to the top of the stack. When a goal is solved, the stack 'unwinds', and
the interpreter navigates back to the parent of the just-solved goal. The parent
then decides how to proceed - in the case of a conjunction, this means calling the
next conjunct. Some rule invocations add a choice point to the choice point stack,
which can be further explored later.

When a rule invocation fails, or the first goal (the root) has been solved, backtracking
occurs. The top of the choice stack is popped, and the state of execution when the top
choice point was made is completely recovered. Execution then proceeds as normal.

There are 2 main interfaces to frame_solver: the first one is next(), which navigates
to the next complete solution. The other one is step(), which takes one step of
execution, and returns a step_result - this can be used by a front end interface
to explain prolog execution in greater detail.

*/

using choice_vec =
strong_vector<choice_idx, frame_solver_types::choice_point>;

using frame_vec =
strong_vector<frame_idx, frame_solver_types::frame>;

struct frame_solver_config {
    bool enable_tail_optimisation = true;
};

class frame_solver {
public:
    using application_result = frame_solver_types::application_result;
    using continuation_state = frame_solver_types::continuation_state;
    using choice_point = frame_solver_types::choice_point;
    using step_result = frame_solver_types::step_result;
    using frame = frame_solver_types::frame;

    explicit
    frame_solver(unification_environment& environment,
                 frame_solver_config config = {}):
        env(environment),
        found_all(false),
        config(config) {
    }

    void solve(term_idx goal_index);
    step_result step();
    bool next();

    choice_vec& get_choices();
    frame_vec& get_stack();
    void log_frame_stack();
    void log_choices();

    [[nodiscard]] bool at_end() const;
    [[nodiscard]] frame_idx get_stack_pointer() const { return stack_index; }

private:
    application_result apply_clause(clause_idx clause_index,
                                    term_idx head_index,
                                    frame_idx parent, choice_idx cut_point,
                                    continuation_state parent_continuation);

    application_result apply_clause_tail(clause_idx clause_index,
                                         term_idx head_index,
                                         frame& current_frame);

    void unwind(frame& current_frame);
    bool backtrack();

    void add_frame(
        term_idx index, frame_idx parent = frame_idx::invalid(),
        choice_idx cut_point = choice_idx::invalid(),
        continuation_state parent_continuation = {});

    [[nodiscard]] frame create_frame(
        term_idx index, frame_idx parent = frame_idx::invalid(),
        choice_idx cut_point = choice_idx::invalid(),
        continuation_state parent_continuation = {}) const;


    bool restore_decision_point();

    frame_idx top_index();

    void log_frame(frame& frame_);

    [[nodiscard]] int eval_arith(term_idx i) const;

    // handle_rule() returns true if and only if rule application is successful.
    bool handle_rule(frame& current_frame);
    bool handle_application_result(application_result result,
                                   frame& current_frame);

    void handle_conjunction(frame& current_frame);
    void handle_disjunction(frame& current_frame);
    void handle_cut(frame& current_frame);

    void erase_choices(choice_idx cut_point);

    unification_environment& env;
    choice_vec choices;
    frame_vec stack;
    bool found_all;

    frame_idx stack_index{0};
    bool solved = false;

    frame_solver_config config;
};


#endif //FRAME_SOLVER_H
