#ifndef PROLOG_TYPES_H
#define PROLOG_TYPES_H

#include <vector>
#include <variant>
#include "helper.h"

struct term_index_base {
};

struct clause_index_base {
};

struct identifier_index_base {
};

struct trail_index_base {
};

using term_index = strong_index<term_index_base>;
using clause_index = strong_index<clause_index_base>;
using identifier_index = strong_index<identifier_index_base>;
using trail_index = strong_index<trail_index_base>;

enum class prolog_var_type {
    BOUND, UNBOUND
};

struct prolog_var {
    explicit prolog_var(identifier_index identifier);

    prolog_var(prolog_var_type type, term_index index,
               identifier_index identifier, int version);

    prolog_var_type type = prolog_var_type::UNBOUND;
    term_index index = term_index::invalid();
    identifier_index identifier = identifier_index::invalid();
    int version = 0;
};


struct prolog_struct {
    prolog_struct();

    explicit prolog_struct(size_t num_children);

    prolog_struct(size_t num_children, identifier_index index);
    prolog_struct(prolog_struct&& other) noexcept;
    prolog_struct& operator=(prolog_struct&& other) noexcept;
    prolog_struct(const prolog_struct& other);
    prolog_struct& operator=(const prolog_struct& other);
    ~prolog_struct();

    term_index& operator[](size_t i);
    const term_index& operator[](size_t i) const;

    term_index& at(size_t i);
    [[nodiscard]] const term_index& at(size_t i) const;

    term_index left();
    term_index right();

    identifier_index index;
    size_t num_children;
    static constexpr size_t inline_capacity = 2;

private:
    union {
        term_index* children{};
        term_index inline_children[inline_capacity];
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
    prolog_clause(term_index head, term_index body): head(head), body(body) {
    }

    explicit prolog_clause(term_index head): head(head),
                                             body(term_index::invalid()) {
    }

    term_index head; // Term
    term_index body; // Goal: -1 if not present
};

struct prolog_timestamp {
    prolog_timestamp(
        term_index term_index_,
        clause_index clause_index_,
        trail_index trail_index_
        ): term_index_(term_index_), clause_index_(clause_index_),
           trail_index_(trail_index_) {
    }

    term_index term_index_;
    clause_index clause_index_;
    trail_index trail_index_;
};

#endif // PROLOG_TYPES_H