#ifndef FRAME_SOLVER_H
#define FRAME_SOLVER_H

#include "frame_solver_types.h"
#include "unification_environment.h"
#include "strong_indices.h"

// #define FRAME_SOLVER_DEBUG

using choice_vec =
    strong_vector<choice_idx, frame_solver_types::choice_point>;

using frame_vec =
    strong_vector<frame_idx, frame_solver_types::frame>;

class frame_solver {
public:
    using application_result = frame_solver_types::application_result;
    using continuation_state = frame_solver_types::continuation_state;
    using frame = frame_solver_types::frame;
    using choice_point = frame_solver_types::choice_point;

    explicit
    frame_solver(unification_environment& environment): env(environment),
        found_all(false) {
    }

    void solve(term_idx goal_index);
    frame_solver& operator++();
    bool operator*() const;

    frame_solver_types::step_result step();

    application_result apply_clause(clause_idx clause_index, term_idx head_index,
                                    frame_idx parent, choice_idx cut_point,
                                    continuation_state parent_continuation);

    application_result apply_clause_tail(clause_idx clause_index, term_idx head_index,
                                         frame& current_frame);

    void unwind();
    bool backtrack();
    choice_vec& get_choices();
    frame_vec& get_stack();
    void log_frame_stack();
    void log_choices();

    [[nodiscard]] bool at_end() const;
    [[nodiscard]] frame_idx get_stack_pointer() const { return stack_index; }

private:
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

    // handle_rule() returns true if and only if rule application is successful.
    bool handle_rule(frame& current_frame);
    bool handle_application_result(application_result result);

    void handle_conjunction(frame& current_frame);
    void handle_disjunction(frame& current_frame);
    void handle_cut(frame& current_frame);


    unification_environment& env;
    choice_vec choices;
    frame_vec stack;
    bool found_all;

    frame_idx stack_index{0};
    bool solved = false;
};


#endif //FRAME_SOLVER_H
