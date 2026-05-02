#include <iostream>

#include "frame_solver.h"

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

void frame_solver::solve(term_idx goal_index) {
    stack_index = frame_idx{0};
    found_all = false;
    stack.clear();
    choices.clear();
    add_frame(goal_index);
}

#ifdef FRAME_SOLVER_DEBUG
#define PRINT_TRACE()   \
do {                    \
    log_frame_stack();  \
    log_choices();      \
    puts("\n");         \
} while (false)

#include <cassert>
#define ASSERT(X) assert(X)

#else
#define PRINT_TRACE()
#define ASSERT(X)
#endif


bool frame_solver::next() {
    if (invalid(stack_index)) {
        // Previous traversal was successful:
        if (!backtrack()) return false;
        PRINT_TRACE();
    }

    while (!invalid(stack_index)) {
        PRINT_TRACE();
        frame& current_frame = stack[stack_index];

        switch (current_frame.type) {
        case frame_type::RULE: {
            if (!handle_rule(current_frame)) {
                if (!backtrack()) return false;
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
    return true;
}

step_result frame_solver::step() {
    using enum step_type;
    if (invalid(stack_index)) {
        return backtrack() ? step_result{BACKTRACK} : step_result{FINISH};
    }

    frame& current_frame = stack[stack_index];

    switch (current_frame.type) {
    case frame_type::RULE: {
        frame_idx current_frame_index = stack_index;

        if (!handle_rule(current_frame)) {
            return backtrack()
                       ? step_result{BACKTRACK}
                       : step_result{FINISH};
        }

        clause_idx matched = stack[current_frame_index].original_clause;
        if (invalid(stack_index)) {
            return step_result{SUCCESS_RULE, matched};
        } else {
            return step_result{INVOKE_RULE, matched};
        }
    }

    case frame_type::CUT: {
        handle_cut(current_frame);
        return invalid(stack_index)
                   ? step_result{SUCCESS_CUT}
                   : step_result{CUT};
    }

    case frame_type::CONJUNCTION: {
        handle_conjunction(current_frame);
        return step_result{INVOKE_CONJUNCTION};
    }

    case frame_type::DISJUNCTION: {
        handle_disjunction(current_frame);
        return step_result{INVOKE_DISJUNCTION};
    }

    default:
        throw std::logic_error{"Not implemented"};
    }
}

#define COMPARISON_CASE(op, cond)\
CASE(op): {                                                                         \
    int left_val, right_val;                                                        \
    try {                                                                           \
        left_val = env.evaluate_arithmetic_term(structure.left());                  \
        right_val = env.evaluate_arithmetic_term(structure.right());                \
    } catch (std::exception& e) {                                                   \
        throw prolog_user_error(e.what());                                          \
    }                                                                               \
    if (cond) {                                                                     \
        unwind(current_frame);                                                      \
        return true;                                                                \
    }                                                                               \
    return false;                                                                   \
}

bool frame_solver::handle_rule(frame& current_frame) {
    prolog_struct& structure = env.get_struct(current_frame.index);

    switch (structure.index.raw()) {
    CASE("halt"): {
        if (structure.num_children != 0) break;
        throw prolog_user_error("halt/0: execution halted by user");
    }

    CASE("is"): {
        term_idx left = structure.left();
        if (env.term_vector[left].is_structure()) {
            throw prolog_user_error(
                "is/2: left-hand side must be an unbound variable or integer");
        }

        term_idx right = env.add_integer(eval_arith(structure.right()));
        env.unify(left, right);
        unwind(current_frame);
        return true;
    }

    CASE("="): {
        term_idx left = structure.left();
        term_idx right = structure.right();

        if (env.unify(left, right)) {
            unwind(current_frame);
            return true;
        }

        return false;
    }

    CASE("\\="): {
        prolog_timestamp timestamp = env.get_timestamp();
        term_idx left = structure.left();
        term_idx right = structure.right();

        if (env.unify(left, right)) {
            env.apply_timestamp(timestamp);
            return false;
        }

        unwind(current_frame);
        return true;
    }

    CASE("not"):
    CASE("\\+"): {
        if (structure.num_children != 1) break;

        if (current_frame.decision_index == 1) {
            // 'not' has been invoked as a result of a backtrack:
            unwind(current_frame);
            return true;
        }

        if (current_frame.continuation.next == 0) {
            // 'not' has been invoked for the first time:
            current_frame.continuation.next = 1;
            choices.emplace_back(env.get_timestamp(), stack_index,
                                 stack.size(), 1,
                                 choice_point_type::NEGATION);

            add_frame(structure[0], stack_index, current_frame.cut_point,
                      current_frame.continuation);
            stack_index = top_index();
            return true;
        }
        // Second entry into 'not' (propagate failure):
        erase_choices(current_frame.cut_point);
        return false;
    }

    COMPARISON_CASE("<", left_val < right_val);
    COMPARISON_CASE(">", left_val > right_val);
    COMPARISON_CASE("=<", left_val <= right_val);
    COMPARISON_CASE(">=", left_val >= right_val);
    COMPARISON_CASE("=:=", left_val == right_val);

    default:


    }

    current_frame.continuation.remains = false;

    // Get the current state, in case a decision point needs to be recovered:
    prolog_timestamp timestamp = env.get_timestamp();
    size_t stack_size = stack.size();

    continuation_state continuation = current_frame.continuation;
    choice_idx cut_point = current_frame.cut_point;

    if (structure.index.raw() >= env.identifier_clause_map.size()) {
        return false;
    }

    // This lookup is not always necessary:

    if (invalid(current_frame.clause_lower_bound)) {
        auto [lower_bound, upper_bound] =
            env.identifier_clause_map[structure.index];
        current_frame.clause_lower_bound = lower_bound;
        current_frame.clause_upper_bound = upper_bound;
    }

    clause_idx lower_bound = current_frame.clause_lower_bound;
    clause_idx upper_bound = current_frame.clause_upper_bound;

    if (upper_bound == clause_idx{0}) {
        return false;
    }

    size_t decision_range = upper_bound.raw() - lower_bound.raw();

    application_result result = application_result::FAILURE;
    term_idx head_index = current_frame.index;
    size_t decision_index = current_frame.decision_index;

    // Tail optimization:
    if (config.enable_tail_optimisation && decision_range == 1) {
        result = apply_clause_tail(lower_bound, head_index, current_frame);
        if (result == application_result::FACT) {
            current_frame.original_clause = lower_bound;
        }
        return handle_application_result(result, current_frame);
    }

    for (size_t i = decision_index; i < decision_range; i++) {
        clause_idx c = lower_bound + i;
        frame_idx parent = stack_index;

        // Attempt clause application:
        result = apply_clause(c, head_index, parent, cut_point,
                              continuation);

        if (is_success(result)) {
            stack[stack_index].original_clause = c;
            if (i + 1 < decision_range) {
                // If possible, place a decision point:
                choices.emplace_back(
                    timestamp, stack_index, stack_size, i + 1);
            }
            break;
        }
    }
    return handle_application_result(result, current_frame);
}


bool frame_solver::handle_application_result(application_result result,
                                             frame& current_frame) {
    switch (result) {
    case application_result::FACT: {
        unwind(current_frame);
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
        choices.emplace_back(env.get_timestamp(), stack_index,
                             stack.size(),
                             decision_index + 1);
    }

    add_frame(structure[decision_index], stack_index,
              current_frame.cut_point, current_frame.continuation);
    stack_index = top_index();
}

void frame_solver::handle_cut(frame& current_frame) {
    erase_choices(current_frame.cut_point);
    unwind(current_frame);
}

void frame_solver::erase_choices(choice_idx cut_point) {
    choices.erase(
        choices.begin() + static_cast<std::ptrdiff_t>(cut_point.raw()),
        choices.end());
}


void frame_solver::unwind(frame& current_frame) {
    current_frame.continuation.remains = false;
    while (stack_index != frame_idx::invalid() &&
           !stack[stack_index].continuation.remains) {
        stack_index = stack[stack_index].parent;
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
    if (choices.empty())
        return false;
    choice_point cp = choices.back();
    choices.pop_back();

    ASSERT(cp.stack_size <= stack.size());
    ASSERT(cp.stack_index.raw() < cp.stack_size);

    env.apply_timestamp(cp.timestamp);
    stack_index = cp.stack_index;
    stack.erase(
        stack.begin() + static_cast<std::ptrdiff_t>(cp.stack_size),
        stack.end());

    stack[stack_index].decision_index = cp.decision_index;

    stack[stack_index].continuation.remains = true;

    for (frame_idx i = stack_index;
         stack[i].parent != frame_idx::invalid();
         i = stack[i].parent) {
        frame_idx p = stack[i].parent;
        stack[p].continuation = stack[i].parent_continuation;
    }

    return true;
}

application_result frame_solver::apply_clause(
    clause_idx clause_index, term_idx head_index,
    frame_idx parent, choice_idx cut_point,
    continuation_state parent_continuation) {

    prolog_timestamp timestamp = env.get_timestamp();
    clause_idx duplicate_index = env.duplicate_clause(clause_index);
    prolog_clause& duplicate_clause = env.get_clause(duplicate_index);

    // Now, attempt unification:
    if (env.unify(head_index, duplicate_clause.head)) {
        if (duplicate_clause.body != term_idx::invalid()) {
            add_frame(
                duplicate_clause.body,
                parent,
                cut_point,
                parent_continuation);
            stack.back().original_clause = clause_index;
            return application_result::RULE;
        }
        return application_result::FACT;
    }

    env.apply_timestamp(timestamp);
    return application_result::FAILURE;
}

application_result frame_solver::apply_clause_tail(
    clause_idx clause_index, term_idx head_index, frame& current_frame) {

    prolog_timestamp timestamp = env.get_timestamp();
    clause_idx duplicate_index = env.duplicate_clause(clause_index);
    prolog_clause& duplicate_clause = env.get_clause(duplicate_index);

    // Now, attempt unification:
    if (env.unify(head_index, duplicate_clause.head)) {
        if (duplicate_clause.body != term_idx::invalid()) {
            current_frame = create_frame(
                duplicate_clause.body,
                current_frame.parent,
                current_frame.cut_point,
                current_frame.parent_continuation);
            current_frame.original_clause = clause_index;
            return application_result::RULE;
        }
        return application_result::FACT;
    }

    env.apply_timestamp(timestamp);
    return application_result::FAILURE;
}

frame_type identifier_index_to_frame_type(name_idx i) {
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

frame frame_solver::create_frame(term_idx index, frame_idx parent,
                                 choice_idx cut_point,
                                 continuation_state parent_continuation) const {
    ASSERT(index.raw() < env.term_vector.size());
    term_idx resolved_idx = env.resolve_bound_variable(index);
    prolog_term& term = env.term_vector[resolved_idx];

    if (term.type == prolog_term_type::INTEGER) {
        throw prolog_user_error(
            "call/1: cannot invoke an integer as a goal");
    }
    if (term.type == prolog_term_type::VARIABLE) {
        throw prolog_user_error(
            "call/1: cannot invoke an unbound variable as a goal");
    }

    prolog_struct& structure = env.get_struct(resolved_idx);
    frame_type frame_type_ =
        identifier_index_to_frame_type(structure.index);
    if (frame_type_ == frame_type::RULE) {
        cut_point = choice_idx{choices.size()};
    }
    return {
        frame_type_, resolved_idx, parent, cut_point,
        parent_continuation
    };
}

void frame_solver::add_frame(term_idx index, frame_idx parent,
                             choice_idx cut_point,
                             continuation_state parent_continuation) {

    stack.emplace_back(create_frame(index, parent, cut_point,
                                    parent_continuation));
}

frame_idx frame_solver::top_index() {
    return frame_idx{stack.size() - 1};
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

void frame_solver::log_choices() {
    std::cout << "HISTORY: ";
    if (choices.empty())
        std::cout << "EMPTY";
    else {
        for (choice_point& dp : choices) {
            std::cout << "[stack_index: " << dp.stack_index <<
                ", decision_index: " <<
                dp.decision_index << ']';
        }
    }
    std::cout << '\n';
}

int frame_solver::eval_arith(term_idx i) const {
    try {
        return env.evaluate_arithmetic_term(i);
    } catch (std::exception& e) {
        throw prolog_user_error(e.what());
    }
}

bool frame_solver::at_end() const {
    return found_all;
}

choice_vec& frame_solver::get_choices() {
    return choices;
}

frame_vec& frame_solver::get_stack() {
    return stack;
}
