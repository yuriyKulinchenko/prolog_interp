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

    explicit frame_solver(unification_environment& environment):
    env(environment), found_all(false) {}
    void solve(int goal_index);
    frame_solver& operator++();
    bool operator*() const;

    application_result apply_clause(int clause_index, int head_index,
        int parent, continuation_state parent_continuation);
    void unwind();

    void log_frame_stack();
    void log_history();
    [[nodiscard]] bool at_end() const;

private:
    void add_frame(int index, int parent, continuation_state parent_continuation);
    bool restore_decision_point();
    prolog_timestamp get_timestamp();
    void apply_timestamp(prolog_timestamp timestamp);
    int top_index();

    void log_frame(frame& frame_);

    unification_environment& env;
    std::vector<decision_point> history;
    std::vector<frame> stack;
    bool found_all;

    int stack_index = 0;
    bool solved = false;
};



#endif //FRAME_SOLVER_H
