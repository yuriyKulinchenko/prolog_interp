#ifndef FRAME_SOLVER_TYPES_H
#define FRAME_SOLVER_TYPES_H

#include "helper.h"
#include "prolog_types.h"

namespace frame_solver_types {
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
        frame(): type(frame_type::HALT), index(-1), parent(-1), cut_point(-1) {}

        frame(frame_type type, int index, int parent, int cut_point, continuation_state parent_continuation):
        type(type), index(index), parent(parent),
        parent_continuation(parent_continuation), cut_point(cut_point) {}

        frame_type type;
        int index;
        int parent;
        int decision_index = 0;
        int cut_point;

        continuation_state continuation;
        continuation_state parent_continuation;
    };

    struct decision_point {
        decision_point(prolog_timestamp timestamp, int stack_index, int stack_size, int decision_index):
            timestamp(timestamp), stack_index(stack_index), stack_size(stack_size), decision_index(decision_index) {}
        prolog_timestamp timestamp;
        int stack_index;
        int stack_size;
        int decision_index;
    };

    enum class application_result {
        RULE, FACT, FAILURE
    };
}

struct frame_logger_configuration {
    static constexpr bool log_continuation = true;
    static constexpr bool log_parent = true;
    static constexpr bool log_cut_point = true;
};

#endif //FRAME_SOLVER_TYPES_H
