#ifndef FRAME_SOLVER_H
#define FRAME_SOLVER_H

#include "frame_solver_types.h"
#include "unification_environment.h"

/*
The goal of the frame_solver is the same as the solver: it has the same interface
However, the implementation is very different: Instead of maintaining a current goal stack,
it will rely on stack frames, an abstraction similar to function execution.
This will significantly optimize memory usage, and bring this interpreter closer in line with
real prolog interpreters. Here is an example of how execution may occur for the following rules:

p :- q, r, s

In the old approach:

[p] -> [q, r, s] -> [r, s] -> [s] -> []

In the new approach:

[{p}] -> [{p}, {q}] -> [{p}, {r}] -> [{p}, {s}] -> []

The key idea is that rule invocation does not wipe the rule from the current state.
This will prevent the solver from having to keep track of the full goal stack in each
decision point.

Specifics of implementation:

On top of the unification environment, there will be 2 data structures:
the frame stack, and the decision point stack.

Structure of the frame:

Each frame has the following data:
- Frame type (clause invocation, conjunction, disjunction)
- Continuation index

Each decision point has the following data:
- unification_environment timestamp
- pointer to the frame to which to backtrack

The success state for when a solution is found is roughly the same: the goal is to reduce
the stack to empty. However, there is a very big difference: the stack MUST preserve the
elements that are popped, instead of discarding them. For instance:

[{p}] -> [{p}, {q}] -> [{p}, {r}] -> [{p}, {s}] -> []

Here, [{p}, {s}] still continues to exist, it is just that the front has moved.
This necessitates a custom data structure that is non-destructive on pop_back.

*/

#define FRAME_SOLVER_DEBUG

class frame_solver {
public:
    explicit frame_solver(unification_environment& environment):
    environment(environment), found_all(false) {}
    void solve(int goal_index);
    frame_solver& operator++();
    bool operator*();

    void log_frame_stack(frame_stack& frame_stack_);
    void log_history();
    [[nodiscard]] bool at_end() const;

private:
    void add_goal(int goal_index);
    void add_decision_point();
    void restore_decision_point();
    prolog_timestamp get_timestamp();

    void log_frame(frame& frame_);

    unification_environment& environment;
    std::vector<frame_decision_point> history;
    frame_stack stack;
    bool found_all;
};



#endif //FRAME_SOLVER_H
