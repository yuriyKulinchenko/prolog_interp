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

    using c_idx = clause_index;
    using f_idx = frame_index;
    using fh_idx = frame_history_index;
    using t_idx = term_index;
    using cs = continuation_state;

    explicit
    frame_solver(unification_environment& environment): env(environment),
        found_all(false) {
    }

    void solve(term_index goal_index);
    frame_solver& operator++();
    bool operator*() const;

    frame_solver_types::step_result step();

    application_result apply_clause(c_idx clause_idx, t_idx head_index,
                                    f_idx parent, fh_idx cut_point,
                                    cs parent_continuation);

    application_result apply_clause_tail(c_idx clause_idx, t_idx head_index,
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
    void add_frame(
        t_idx index, f_idx parent = f_idx::invalid(),
        fh_idx cut_point = fh_idx::invalid(),
        cs parent_continuation = {});

    [[nodiscard]] frame create_frame(
        t_idx index, f_idx parent = f_idx::invalid(),
        fh_idx cut_point = fh_idx::invalid(),
        cs parent_continuation = {}) const;


    bool restore_decision_point();

    frame_index top_index();

    void log_frame(frame& frame_);

    // handle_rule() returns true if and only if rule application is successful.
    bool handle_rule(frame& current_frame);
    bool handle_application_result(application_result result);

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
