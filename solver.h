#ifndef SOLVER_H
#define SOLVER_H

#include "unification_environment.h"

/*

A decision point is uniquely identified by the following:
- A term_vector, trail and clause_vector timestamp
- A specific goal that is in progress
- The visited index of that goal

For instance:

p :- q
p :- r

Initially, when attempting to resolve p, the decision point will look like this:
{timestamp: {...}, goal: p, goal_index: 0}

When backtracking to here, the goal_index is incremented, and that path is attempted

*/

struct prolog_timestamp {
    prolog_timestamp(int term_index, int clause_index, int trail_index):
    term_index(term_index), clause_index(clause_index), trail_index(trail_index) {}

    int term_index;
    int clause_index;
    int trail_index;
};

struct decision_point {
    decision_point(prolog_timestamp timestamp, int goal_index, int choice_number):
    timestamp(timestamp), goal_index(goal_index), choice_number(choice_number) {}

    prolog_timestamp timestamp;
    int goal_index;
    int choice_number;
};

class solver {
public:
    explicit solver(unification_environment& environment);
    void solve(int goal_index);
    solver& operator++();
    bool operator*();
private:
    unification_environment& environment;
};



#endif //SOLVER_H
