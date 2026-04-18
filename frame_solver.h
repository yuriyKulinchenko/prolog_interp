#ifndef FRAME_SOLVER_H
#define FRAME_SOLVER_H

#include "frame_solver_types.h"
#include "unification_environment.h"

#define FRAME_SOLVER_DEBUG

class frame_solver {
public:
    explicit frame_solver(unification_environment& environment):
    env(environment), found_all(false) {}
    void solve(int goal_index);
    frame_solver& operator++();
    bool operator*() const;

    application_result apply_clause(int clause_index, int head_index,
        int parent, continuation_state parent_continuation);
    void unwind();

    void log_frame_stack(std::vector<frame>& frame_stack_);
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
    std::vector<frame_decision_point> history;
    std::vector<frame> stack;
    bool found_all;

    int decision_index = 0;
    int stack_index = 0;
    bool solved = false;
};



#endif //FRAME_SOLVER_H
