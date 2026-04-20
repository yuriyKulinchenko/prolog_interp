#ifndef FRAME_SOLVER_TYPES_H
#define FRAME_SOLVER_TYPES_H

#include "helper.h"
#include "prolog_types.h"

struct frame_index_base {};
struct frame_history_index_base {};

using frame_index = strong_index<frame_index_base>;
using frame_history_index = strong_index<frame_history_index_base>;

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
        frame():
        type(frame_type::HALT),
        index(term_index::invalid()),
        parent(frame_index::invalid()),
        cut_point(frame_history_index::invalid()) {}

        frame(frame_type type, term_index index, frame_index parent,
            frame_history_index cut_point, continuation_state parent_continuation):
        type(type),
        index(index),
        parent(parent),
        cut_point(cut_point),
        parent_continuation(parent_continuation) {}

        frame_type type;
        term_index index;
        frame_index parent;
        size_t decision_index = 0;
        frame_history_index cut_point;

        continuation_state continuation;
        continuation_state parent_continuation;
    };

    struct decision_point {
        decision_point(prolog_timestamp timestamp,
            frame_index stack_index,
            size_t stack_size,
            size_t decision_index):

        timestamp(timestamp),
        stack_index(stack_index),
        stack_size(stack_size),
        decision_index(decision_index) {}

        prolog_timestamp timestamp;
        frame_index stack_index;
        size_t stack_size;
        size_t decision_index;
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
