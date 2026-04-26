/*

The key idea with the frame based solver is that completed branches are NOT popped from
the frame stack, unlike the original solver class, and the previous implementation of
the frame solver. For instance:

p :- q, r.

q :- a1.
r :- b1.

a1 :- a2. a2 :- a3. ... a99 :- a100.
b1 :- b2. b2 :- b3. ... b99 :- b100.
a100 :- x; y.
b100.

Using the implementation of the old frame solver, the stack behaviour would look like this:

[p] -> [p][q, r] -> [p][q, r][a1]...[a100][x] -> stack collapses -> [p][q, r][b1]...[b100]

The key issue is that the decision point make by a100 needs to be recoverable: this is not possible
under this model without storing the entire stack.

Here is the new execution model:

[p] -> [p][q, r] -> [p][q, r][a1]...[a100][x] -> [p][q, r][a1]...[a100][x][b1]...[b100]

The key difference is that now, every branch is stored. the decision point now only has to
truncate the stack, instead of performing any complex recovery. This saves significant space
in each decision point, and also preserves the decision graph.

Note that this relies on every frame having a pointer to its parent - something that didn't need
to be present before.

PRECISE EXECUTION MODEL:

The frame_solver will effectively execute as a VM, with the following behaviour:

There are several distinct frame types, currently including:
- RULE
- CONJUNCTION
- DISJUNCTION

The VM is in 2 possible states of execution. It is either growing the stack, or following
the stack backwards via the parent pointers of each of the frames (shrinking state).
Each of the following behaviours assume that the VM is in the growing state.
The shrinking state is not that difficult to implement, as it is effectively
pointer traversal.

When the VM encounters a rule, it does the following:
- The continuation is incremented from 0 to 1
- It chooses the correct decision index (more on this when discussing decision points)
- It places the body of the chosen rule onto the stack

More details to follow...


More complex predicates: negation and findall


find_lessons(Bag) :-
    findall(X, happening(X,chemistry), Bag).

*/


#include "frame_solver.h"

#include <cassert>
#include <iostream>

#define CASE(s) case unification_environment::get_reserved_identifier_index(s).raw()

using namespace frame_solver_types;

bool is_fact(application_result result) {
    return result == application_result::FACT;
}

bool is_rule(application_result result) {
    return result == application_result::RULE;
}

bool is_success(application_result result) {
    return is_fact(result) || is_rule(result);
}

template <typename T>
bool invalid(strong_index<T> index) {
    return index == strong_index<T>::invalid();
}

void frame_solver::solve(term_index goal_index) {
    stack_index = frame_index{0};
    found_all = false;
    stack.clear();
    history.clear();
    add_frame(goal_index);
}

#ifdef FRAME_SOLVER_DEBUG
#define PRINT_TRACE()   \
do {                    \
    log_frame_stack();  \
    log_history();      \
    puts("\n");         \
} while (false)

#define ASSERT(X) assert(X)

#else
#define PRINT_TRACE()
#define ASSERT(X)
#endif


frame_solver& frame_solver::operator++() {
    if (invalid(stack_index)) {
        if (!backtrack()) return *this;
        PRINT_TRACE();
    }

    while (!invalid(stack_index)) {
        PRINT_TRACE();
        frame& current_frame = stack[stack_index.raw()];

        switch (current_frame.type) {
        case frame_type::RULE: {
            if (!handle_rule(current_frame)) {
                if (!backtrack()) return *this;
            }
            continue;
        }

        case frame_type::CONJUNCTION: {
            handle_conjunction(current_frame);
            continue;
        }

        case frame_type::DISJUNCTION: {
            handle_disjunction(current_frame);
            continue;
        }

        case frame_type::CUT: {
            handle_cut(current_frame);
            continue;
        }

        default:
            throw std::logic_error{"Not implemented"};
        }
    }
    return *this;
}

step_type frame_solver::step() {
    using enum step_type;
    if (invalid(stack_index)) {
        return backtrack() ? BACKTRACK : FINISH;
    }

    frame& current_frame = stack[stack_index.raw()];

    switch (current_frame.type) {
    case frame_type::RULE: {
        if (!handle_rule(current_frame)) {
            return backtrack() ? BACKTRACK : FINISH;
        }
        return invalid(stack_index) ? SUCCESS_RULE : INVOKE_RULE;
    }

    case frame_type::CUT: {
        handle_cut(current_frame);
        return invalid(stack_index) ? SUCCESS_CUT : CUT;
    }

    case frame_type::CONJUNCTION: {
        handle_conjunction(current_frame);
        return INVOKE_CONJUNCTION;
    }

    case frame_type::DISJUNCTION: {
        handle_disjunction(current_frame);
        return INVOKE_DISJUNCTION;
    }

    default:
        throw std::logic_error{"Not implemented"};
    }
}

#define COMPARISON_CASE(op, cond)\
CASE(op): {                                                                     \
    current_frame.continuation.remains = false;                                 \
    int left_val = env.evaluate_arithmetic_term(structure.left());              \
    int right_val = env.evaluate_arithmetic_term(structure.right());            \
    if (cond) {unwind(); return true;}                                          \
    return false;                                                               \
}

bool frame_solver::handle_rule(frame& current_frame) {
    prolog_struct& structure = env.get_struct(current_frame.index);
    current_frame.continuation.remains = false;

    switch (structure.index.raw()) {
    CASE("halt"):
        throw std::logic_error("EXECUTION HALTED");

    CASE("is"): {
        term_index left = structure.left();
        if (env.term_vector[left.raw()].is_structure()) {
            throw std::logic_error(
                "ERROR: Left hand side of is/2 must be variable or integer");
        }

        term_index right = env.evaluate_and_create_arithmetic_term(
            structure.right());
        env.unify(left, right);
        unwind();
        return true;
    }

    CASE("="): {
        term_index left = structure.left();
        term_index right = structure.right();

        if (env.unify(left, right)) {
            unwind();
        } else {
            return false;
        }
        return true;
    }

    CASE("\\="): {
        prolog_timestamp timestamp = env.get_timestamp();
        term_index left = structure.left();
        term_index right = structure.right();

        if (env.unify(left, right)) {
            env.apply_timestamp(timestamp);
            return false;
        }

        unwind();
        return true;
    }

    COMPARISON_CASE("<", left_val < right_val);
    COMPARISON_CASE(">", left_val > right_val);
    COMPARISON_CASE("=<", left_val <= right_val);
    COMPARISON_CASE(">=", left_val >= right_val);

    default:

        // Get the current state, in case a decision point needs to be recovered:
        prolog_timestamp timestamp = env.get_timestamp();
        size_t stack_size = stack.size();

        continuation_state continuation = current_frame.continuation;
        frame_history_index cut_point = current_frame.cut_point;

        if (structure.index.raw() >= env.identifier_clause_map.size()) {
            return false;
        }

        auto [lower_bound, upper_bound] =
            env.identifier_clause_map[structure.index.raw()];

        if (upper_bound == clause_index{0}) {
            return false;
        }

        size_t decision_range = upper_bound.raw() - lower_bound.raw();

        application_result result = application_result::FAILURE;
        term_index head_index = current_frame.index;
        size_t decision_index = current_frame.decision_index;

        // Tail optimization:
        if (decision_range == 1) {
            result = apply_clause_tail(lower_bound, head_index, current_frame);
            return handle_application_result(result);
        }

        for (size_t i = decision_index; i < decision_range; i++) {
            clause_index clause_idx = lower_bound + i;
            frame_index parent = stack_index;

            // Attempt clause application:
            result = apply_clause(clause_idx, head_index, parent, cut_point,
                                  continuation);

            if (is_success(result)) {
                if (i + 1 < decision_range) {
                    // If possible, place a decision point:
                    history.emplace_back(
                        timestamp, stack_index, stack_size, i + 1);
                }
                break;
            }
        }
        return handle_application_result(result);
    }
}

bool frame_solver::handle_application_result(application_result result) {
    switch (result) {
    case application_result::FACT: {
        unwind();
        return true;
    }
    case application_result::RULE: {
        stack_index = top_index();
        return true;
    }

    case application_result::FAILURE: {
        return false;
    }
    }
    return false;
}


void frame_solver::handle_conjunction(frame& current_frame) {
    int next = current_frame.continuation.next;
    prolog_struct& structure = env.get_struct(current_frame.index);

    ASSERT(current_frame.continuation.next >= 0);
    ASSERT(current_frame.continuation.next <
        static_cast<int>(structure.num_children));

    current_frame.continuation.next++;

    if (next + 1 == static_cast<int>(structure.num_children)) {
        current_frame.continuation.remains = false;
    }

    add_frame(structure[next], stack_index, current_frame.cut_point,
              current_frame.continuation);

    stack_index = top_index();
}

void frame_solver::handle_disjunction(frame& current_frame) {
    prolog_struct& structure = env.get_struct(current_frame.index);
    current_frame.continuation.remains = false;
    size_t decision_index = current_frame.decision_index;

    if (decision_index + 1 < structure.num_children) {
        history.emplace_back(env.get_timestamp(), stack_index,
                             stack.size(),
                             decision_index + 1);
    }

    add_frame(structure[decision_index], stack_index,
              current_frame.cut_point, current_frame.continuation);
    stack_index = top_index();
}

void frame_solver::handle_cut(frame& current_frame) {
    current_frame.continuation.remains = false;
    history.erase(
        history.begin() + static_cast<std::ptrdiff_t>(current_frame.
            cut_point.raw()),
        history.end());
    unwind();
}

void frame_solver::unwind() {
    while (stack_index != frame_index::invalid() &&
           !stack[stack_index.raw()].continuation.remains) {
        stack_index = stack[stack_index.raw()].parent;
    }
}

// Returns whether backtrack was successful
bool frame_solver::backtrack() {
    if (!restore_decision_point()) {
        found_all = true;
        return false;
    }
    return true;
}

bool frame_solver::restore_decision_point() {
    if (history.empty())
        return false;
    decision_point decision_point = history.back();
    history.pop_back();

    ASSERT(decision_point.stack_size <= stack.size());
    ASSERT(decision_point.stack_index.raw() < decision_point.stack_size);

    env.apply_timestamp(decision_point.timestamp);
    stack_index = decision_point.stack_index;
    stack.erase(
        stack.begin() + static_cast<std::ptrdiff_t>(decision_point.stack_size),
        stack.end());

    stack[stack_index.raw()].decision_index = decision_point.decision_index;

    stack[stack_index.raw()].continuation.remains = true;

    for (frame_index i = stack_index;
         stack[i.raw()].parent != frame_index::invalid();
         i = stack[i.raw()].parent) {
        frame_index p = stack[i.raw()].parent;
        stack[p.raw()].continuation = stack[i.raw()].parent_continuation;
    }

    return true;
}

application_result frame_solver::apply_clause(
    c_idx clause_idx, t_idx head_index,
    f_idx parent, fh_idx cut_point,
    cs parent_continuation) {

    prolog_timestamp timestamp = env.get_timestamp();
    clause_index duplicate_index = env.duplicate_clause(clause_idx);
    prolog_clause& duplicate_clause = env.get_clause(duplicate_index);

    // Now, attempt unification:
    if (env.unify(head_index, duplicate_clause.head)) {
        if (duplicate_clause.body != term_index::invalid()) {
            add_frame(
                duplicate_clause.body,
                parent,
                cut_point,
                parent_continuation);
            return application_result::RULE;
        }
        return application_result::FACT;
    }

    env.apply_timestamp(timestamp);
    return application_result::FAILURE;
}

application_result frame_solver::apply_clause_tail(
    c_idx clause_idx, t_idx head_index, frame& current_frame) {

    prolog_timestamp timestamp = env.get_timestamp();
    clause_index duplicate_index = env.duplicate_clause(clause_idx);
    prolog_clause& duplicate_clause = env.get_clause(duplicate_index);

    // Now, attempt unification:
    if (env.unify(head_index, duplicate_clause.head)) {
        if (duplicate_clause.body != term_index::invalid()) {
            current_frame = create_frame(
                duplicate_clause.body,
                current_frame.parent,
                current_frame.cut_point,
                current_frame.parent_continuation);

            return application_result::RULE;
        }
        return application_result::FACT;
    }

    env.apply_timestamp(timestamp);
    return application_result::FAILURE;
}

frame_type identifier_index_to_frame_type(identifier_index i) {
    switch (i.raw()) {
        using enum frame_type;
    CASE(","):
        return CONJUNCTION;
    CASE(";"):
        return DISJUNCTION;
    CASE("!"):
        return CUT;
    default:
        return RULE;
    }
}

frame frame_solver::create_frame(t_idx index, f_idx parent, fh_idx cut_point,
                                 cs parent_continuation) const {
    ASSERT(index.raw() < env.term_vector.size());
    prolog_term& term = env.term_vector[index.raw()];
    switch (term.type) {
        using enum prolog_term_type;
        using enum frame_type;
    case STRUCTURE: {
        prolog_struct& structure = env.get_struct(index);
        frame_type frame_type_ =
            identifier_index_to_frame_type(structure.index);
        if (frame_type_ == RULE) {
            cut_point = frame_history_index{history.size()};
        }
        return {
            frame_type_, index, parent, cut_point,
            parent_continuation
        };
    }

    default: {
        throw std::logic_error{
            "Error: cannot currently create variable frames"
        };
    }
    }
}

void frame_solver::add_frame(t_idx index, f_idx parent, fh_idx cut_point,
                             cs parent_continuation) {

    stack.emplace_back(create_frame(index, parent, cut_point,
                                    parent_continuation));
}

frame_index frame_solver::top_index() {
    return frame_index{stack.size() - 1};
}

void frame_solver::log_frame(frame& frame_) {
    print_green("[");
    using enum frame_type;

    if (frame_.type == RULE) {
        env.log_term(frame_.index);
    } else if (frame_.type == CUT) {
        std::cout << '!';
    } else {
        env.log_bracketed_term(frame_.index);
    }

    // Continuation:

    if constexpr (frame_logger_configuration::log_continuation) {
        std::cout << ", ";
        if (frame_.continuation.remains) {
            if (frame_.type == CONJUNCTION) {
                std::cout << "continuation: " << frame_.continuation.next;
            } else {
                std::cout << "continues";
            }
        } else {
            std::cout << "stops";
        }
    }

    // Parent:

    if constexpr (frame_logger_configuration::log_parent) {
        std::cout << ", parent: " << frame_.parent;
    }

    // Cut point:

    if constexpr (frame_logger_configuration::log_cut_point) {
        std::cout << ", cut_point: " << frame_.cut_point;
    }

    print_green("]");
}

void frame_solver::log_frame_stack() {
    std::cout << "STACK: ";
    if (stack.empty())
        std::cout << "EMPTY";
    else {
        for (frame& frame_ : stack) {
            log_frame(frame_);
        }
    }
    std::cout << '\n';
}

void frame_solver::log_history() {
    std::cout << "HISTORY: ";
    if (history.empty())
        std::cout << "EMPTY";
    else {
        for (decision_point& dp : history) {
            std::cout << "[stack_index: " << dp.stack_index <<
                ", decision_index: " <<
                dp.decision_index << ']';
        }
    }
    std::cout << '\n';
}

bool frame_solver::operator*() const {
    return stack_index == frame_index::invalid();
}

bool frame_solver::at_end() const {
    return found_all;
}

std::vector<decision_point>& frame_solver::get_history() {
    return history;
}

std::vector<frame>& frame_solver::get_stack() {
    return stack;
}
