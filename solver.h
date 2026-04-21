#ifndef SOLVER_H
#define SOLVER_H

#include "unification_environment.h"

// #define SOLVER_DEBUG

/*

A decision point is uniquely identified by the following:
- A term_vector, trail and clause_vector timestamp
- A specific goal that is in progress
- The visited index of that goal

For instance:

p :- q
p :- r

Initially, when attempting to resolve p, the decision point will look like this:
{timestamp: {...}, goal: p, choice_number: 0}

When backtracking to here, the goal_index is incremented, and that path is attempted

Main execution loop:

- goal is taken from the top of the goal stack
- goal is compared against available clauses, and the corresponding backtracking entry is added if possible
- otherwise, comparison fails: backtrack occurs

*/

class solver {
public:
    struct goal {
        goal(term_index idx, int cut_barrier):
        term_idx(idx), cut_barrier(cut_barrier) {}
        goal(): term_idx(term_index::invalid()), cut_barrier(-1) {}

        term_index term_idx;
        int cut_barrier;
    };

    struct decision_point {
        decision_point(prolog_timestamp timestamp, std::vector<goal>& goal_stack, int next_choice_number):
        timestamp(timestamp), goal_stack(std::move(goal_stack)), next_choice_number(next_choice_number) {}

        prolog_timestamp timestamp;
        std::vector<goal> goal_stack;
        int next_choice_number;
    };

    explicit solver(unification_environment& environment):
    environment(environment), found_all(false) {}
    void solve(term_index goal_index);
    solver& operator++();
    bool operator*();

    void log_goal_stack(std::vector<goal> goal_stack_instance);
    void log_history();
    [[nodiscard]] bool at_end() const;
private:


    goal pop_goal();
    goal peek_goal();
    decision_point pop_history();

    void log_decision_point(decision_point& point);
    void log_goal(goal goal_instance);
    void log_state(int continuation);

    /*
    apply_clause(clause_index) takes the clause specified by clause_index, and
    applies it to the top of the goal stack. The method returns whether this
    rule application was successful. For example, if the clause specified by
    clause_index is:

    p(X) :- q(X), r(X),

    and the goal stack is:

    [p(1), a(2,3), b(4,5)]

    Then the specified clause will be duplicated, unification will be attempted
    with p(1), and the stack will be transformed to the following:

    [q(1), r(1), a(2,3), b(4,5)]

    The history stack will also be updated to reflect the possible decision points that have been added

     */

    bool apply_clause(clause_index clause_idx, int choice_number, bool add_decision_point = true);

    /*

    restores the state of the solver and unification_environment to just before the
    most recent decision point was made. Take the example above. After calling
    restore_decision_point(), the goal stack would revert to:

    [p(1), a(2,3), b(4,5)]

    Note that this method is NOT responsible for keeping track of which choice_number
    is to be selected next: this information is stored in the history, but it must be
    managed by the main execution loop.

    */

    int restore_decision_point();

    prolog_timestamp get_timestamp();
    void apply_timestamp(prolog_timestamp timestamp);


    unification_environment& environment;
    std::vector<decision_point> history;
    std::vector<goal> goal_stack;
    bool found_all;
};



#endif //SOLVER_H
