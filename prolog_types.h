#ifndef PROLOG_TYPES_H
#define PROLOG_TYPES_H

#include <vector>
#include <variant>

#include "frontend_types.h"
#include "helper.h"
#include "prolog_types.h"
#include "strong_indices.h"

struct term_tag {
};

struct clause_tag {
};

struct name_tag {
};

struct var_name_tag {
};

struct trail_tag {
};

using term_idx = strong_index<term_tag>;
using clause_idx = strong_index<clause_tag>;
using name_idx = strong_index<name_tag>;
using var_name_idx = strong_index<var_name_tag>;
using trail_idx = strong_index<trail_tag>;

enum class prolog_var_type {
    BOUND, UNBOUND
};

struct prolog_var {
    explicit prolog_var(var_name_idx identifier);

    prolog_var(var_name_idx identifier, int version);

    prolog_var(prolog_var_type type, term_idx index,
               var_name_idx identifier, int version);

    prolog_var_type type = prolog_var_type::UNBOUND;
    term_idx index = term_idx::invalid();
    var_name_idx identifier = var_name_idx::invalid();
    int version = 0;
};


struct prolog_struct {
    prolog_struct();

    explicit prolog_struct(size_t num_children);

    prolog_struct(size_t num_children, name_idx index);
    prolog_struct(prolog_struct&& other) noexcept;
    prolog_struct& operator=(prolog_struct&& other) noexcept;
    prolog_struct(const prolog_struct& other);
    prolog_struct& operator=(const prolog_struct& other);
    ~prolog_struct();

    term_idx& operator[](size_t i);
    const term_idx& operator[](size_t i) const;

    term_idx& at(size_t i);
    [[nodiscard]] const term_idx& at(size_t i) const;

    term_idx left();
    term_idx right();

    name_idx index;
    size_t num_children;
    static constexpr size_t inline_capacity = 2;

private:
    union {
        term_idx* children{};
        term_idx inline_children[inline_capacity];
    };
};

enum class prolog_term_type {
    VARIABLE, STRUCTURE, INTEGER
};

std::string prolog_term_type_to_string(prolog_term_type type);

struct prolog_term {
    explicit prolog_term(prolog_term_type type);
    explicit prolog_term(int integer);
    explicit prolog_term(prolog_var);

    prolog_struct& structure();
    prolog_var& variable();
    int integer();

    [[nodiscard]] bool is_variable() const;
    [[nodiscard]] bool is_unbound_variable();
    [[nodiscard]] bool is_bound_variable();
    [[nodiscard]] bool is_structure() const;
    [[nodiscard]] bool is_integer() const;
    [[nodiscard]] bool is_ground() const;

    prolog_term_type type;
    std::variant<int, prolog_struct, prolog_var> tagged_union;
};

struct prolog_clause {
    prolog_clause(term_idx head, term_idx body)
    : head(head), body(body) {
    }

    explicit prolog_clause(term_idx head)
    : head(head) {
    }

    prolog_clause(term_idx head, term_idx body, position_range range)
    : head(head), body(body), range(std::make_unique<position_range>(range)) {
    }

    prolog_clause(term_idx head, position_range range)
    : head(head), range(std::make_unique<position_range>(range)) {
    }

    term_idx head;
    term_idx body = term_idx::invalid(); // term_idx::invalid() if body not present
    std::unique_ptr<position_range> range {}; // Range of the clause: nullptr if not present
};

struct prolog_timestamp {
    prolog_timestamp(
        term_idx term,
        clause_idx clause,
        trail_idx trail
        ): term(term), clause(clause),
           trail(trail) {
    }

    term_idx term;
    clause_idx clause;
    trail_idx trail;
};

using term_vec = strong_vector<term_idx, prolog_term>;
using clause_vec = strong_vector<clause_idx, prolog_clause>;
using name_vec = strong_vector<name_idx, std::string>;
using var_name_vec = strong_vector<var_name_idx, std::string>;
using trail_vec = strong_vector<trail_idx, term_idx>;

#endif // PROLOG_TYPES_H
