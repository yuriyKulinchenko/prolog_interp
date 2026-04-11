#ifndef FRAME_SOLVER_TYPES_H
#define FRAME_SOLVER_TYPES_H

#include "helper.h"
#include "prolog_types.h"

enum class frame_type {
    RULE, CONJUNCTION, DISJUNCTION,
    CUT, HALT
};

struct frame {
    frame(frame_type type, int index):
    type(type), index(index), continuation(0) {}

    explicit frame(frame_type type):
    type(type), index(0), continuation(0) {}

    frame_type type;
    int index;
    int continuation;
    // TODO: Adding a final continuation index is a sensible optimization
};

struct frame_decision_point {
    prolog_timestamp timestamp;
    int stack_pointer;
};

class frame_stack {
public:
    frame_stack(): size_(0) {}

    void push_back(const frame& frame) {
        stack.push_back(frame);
    }

    void emplace_back(frame_type type, int index) {
        stack.emplace_back(type, index);
        size_++;
    }

    void emplace_back(frame_type type) {
        stack.emplace_back(type);
        size_++;
    }

    frame& pop_back() {
        if (size_ == 0) {
            throw std::logic_error("ERROR: frame_stack is already empty");
        }
        size_--;
        return stack[size_];
    }

    frame& operator[](size_t index) {
        return stack[index];
    }

    [[nodiscard]] size_t size() const {
        return size_;
    }

    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    void clear() {
        size_ = 0;
        stack.clear();
    }

    void restore(size_t index) {
        size_ = index;
    }

    frame& top() {
        return stack[size_ - 1];
    }

private:
    size_t size_;
    std::vector<frame> stack;
};



#endif //FRAME_SOLVER_TYPES_H
