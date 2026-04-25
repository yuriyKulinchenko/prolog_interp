#ifndef FRAME_SOLVER_H
#define FRAME_SOLVER_H

#include "frame_solver_types.h"
#include "unification_environment.h"

// #define FRAME_SOLVER_DEBUG

class frame_solver {
public:
    using application_result = frame_solver_types::application_result;
    using continuation_state = frame_solver_types::continuation_state;
    using frame = frame_solver_types::frame;
    using decision_point = frame_solver_types::decision_point;

    explicit
    frame_solver(unification_environment& environment): env(environment),
        found_all(false) {
    }

    void solve(term_index goal_index);
    frame_solver& operator++();
    bool operator*() const;

    frame_solver_types::step_type step();

    application_result apply_clause(clause_index clause_idx,
                                    term_index head_index,
                                    frame_index parent,
                                    frame_history_index cut_point,
                                    continuation_state parent_continuation);

    application_result apply_clause_tail(clause_index clause_idx,
                                         frame_index stack_index,
                                         frame& current_frame);

    void unwind();
    bool backtrack();
    std::vector<decision_point>& get_history();
    std::vector<frame>& get_stack();
    void log_frame_stack();
    void log_history();

    [[nodiscard]] bool at_end() const;

    [[nodiscard]] frame_index get_stack_pointer() const { return stack_index; }

private:
    void add_frame(term_index index,
                   frame_index parent = frame_index::invalid(),
                   frame_history_index cut_point =
                       frame_history_index::invalid(),
                   continuation_state parent_continuation = {});

    bool restore_decision_point();

    frame_index top_index();

    void log_frame(frame& frame_);

    // These return 'true' if execution can proceed
    bool handle_rule(frame& current_frame);

    void handle_conjunction(frame& current_frame);

    void handle_disjunction(frame& current_frame);

    void handle_cut(frame& current_frame);


    unification_environment& env;
    std::vector<decision_point> history;
    std::vector<frame> stack;
    bool found_all;

    frame_index stack_index{0};
    bool solved = false;
};


#endif //FRAME_SOLVER_H
