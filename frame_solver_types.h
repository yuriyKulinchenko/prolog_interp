#ifndef FRAME_SOLVER_TYPES_H
#define FRAME_SOLVER_TYPES_H

#include <stdexcept>
#include <string>

#include "helper.h"
#include "prolog_types.h"
#include "strong_indices.h"

struct frame_tag {
};

struct choice_tag {
};

using frame_idx = strong_index<frame_tag>;
using choice_idx = strong_index<choice_tag>;

namespace frame_solver_types {
enum class frame_type {
    RULE, CONJUNCTION, DISJUNCTION,
    CUT, HALT
};

struct continuation_state {
    continuation_state() = default;

    explicit continuation_state(int next, bool remains):
        next(next), remains(remains) {
    }

    int next = 0;
    bool remains = true;
};


struct frame {
    frame():
        type(frame_type::HALT),
        index(term_idx::invalid()),
        parent(frame_idx::invalid()),
        cut_point(choice_idx::invalid()) {
    }

    frame(frame_type type, term_idx index, frame_idx parent,
          choice_idx cut_point,
          continuation_state parent_continuation):
        type(type),
        index(index),
        parent(parent),
        cut_point(cut_point),
        parent_continuation(parent_continuation) {
    }

    frame_type type;
    term_idx index;
    frame_idx parent;

    size_t decision_index = 0;
    clause_idx clause_lower_bound = clause_idx::invalid();
    clause_idx clause_upper_bound = clause_idx::invalid();
    clause_idx original_clause = clause_idx::invalid();

    choice_idx cut_point;

    continuation_state continuation;
    continuation_state parent_continuation;
};

enum class choice_point_type {
    CHOICE, NEGATION

};

struct choice_point {
    choice_point(const prolog_timestamp& timestamp,
                 frame_idx stack_index,
                 size_t stack_size,
                 size_t decision_index,
                 choice_point_type type = choice_point_type::CHOICE
        ):

        type(type),
        timestamp(timestamp),
        stack_index(stack_index),
        stack_size(stack_size),
        decision_index(decision_index) {
    }

    choice_point_type type;
    prolog_timestamp timestamp;
    frame_idx stack_index;
    size_t stack_size;
    size_t decision_index;
};

enum class application_result {
    RULE, FACT, FAILURE
};

enum class step_type {
    FINISH, BACKTRACK, INVOKE_RULE, SUCCESS_CUT, SUCCESS_RULE,
    INVOKE_CONJUNCTION,
    INVOKE_DISJUNCTION, CUT
};

struct step_result {
    explicit step_result(step_type type):
        type(type), data(std::monostate{}) {
    }

    explicit step_result(step_type type, clause_idx index):
        type(type), data(index) {
    }

    step_type type;
    std::variant<std::monostate, clause_idx> data;
};

inline std::string step_type_to_string_(step_type type) {
    switch (type) {
        using enum step_type;
    case FINISH:
        return "FINISH";
    case BACKTRACK:
        return "BACKTRACK";
    case SUCCESS_CUT:
        return "SUCCESS_CUT";
    case SUCCESS_RULE:
        return "SUCCESS_RULE";
    case INVOKE_RULE:
        return "INVOKE_RULE";
    case INVOKE_CONJUNCTION:
        return "INVOKE_CONJUNCTION";
    case INVOKE_DISJUNCTION:
        return "INVOKE_DISJUNCTION";
    case CUT:
        return "CUT";
    }
    return "UNKNOWN";
}

struct frame_logger_configuration {
    static constexpr bool log_continuation = true;
    static constexpr bool log_parent = true;
    static constexpr bool log_cut_point = true;
};

struct prolog_user_error : std::exception {
    explicit prolog_user_error(std::string message) : message_(
        std::move(message)) {
    }

    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};
}

#endif //FRAME_SOLVER_TYPES_H
