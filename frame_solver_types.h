#ifndef FRAME_SOLVER_TYPES_H
#define FRAME_SOLVER_TYPES_H

#include "helper.h"
#include "prolog_types.h"

enum class frame_type {
    RULE, CONJUNCTION, DISJUNCTION,
    CUT, HALT
};

struct continuation_state {
    continuation_state() = default;
    explicit continuation_state(int next, bool remains):
    next(next), remains(remains) {}

    int next = 0;
    bool remains = true;
};

struct frame {
    frame(frame_type type, int index, int parent, continuation_state parent_continuation):
    type(type), index(index), parent(parent), parent_continuation(parent_continuation) {}

    frame_type type;
    int index;
    int parent;

    continuation_state continuation;
    continuation_state parent_continuation;
};

struct frame_decision_point {
    frame_decision_point(prolog_timestamp timestamp, int stack_index, int stack_size, int decision_index):
        timestamp(timestamp), stack_index(stack_index), stack_size(stack_size), decision_index(decision_index) {}
    prolog_timestamp timestamp;
    int stack_index;
    int stack_size;
    int decision_index;
};

enum class application_result {
    RULE, FACT, FAILURE
};

#endif //FRAME_SOLVER_TYPES_H
