#ifndef PROLOG_TYPES_H
#define PROLOG_TYPES_H

#include <vector>
#include <variant>
#include "helper.h"

struct term_index_base {};
struct clause_index_base {};
struct identifier_index_base {};
struct trail_index_base {};

using term_index = strong_index<term_index_base>;
using clause_index = strong_index<clause_index_base>;
using identifier_index = strong_index<identifier_index_base>;
using trail_index = strong_index<trail_index_base>;

enum class prolog_var_type {
    BOUND, UNBOUND
};

struct prolog_var {
    prolog_var();
    prolog_var(prolog_var_type type, term_index index);

    prolog_var_type type;
    term_index index;
};

struct prolog_struct {
    prolog_struct();
    explicit prolog_struct(identifier_index index);

    identifier_index index;
    std::vector<term_index> children;
};

enum class prolog_term_type {
    VARIABLE, STRUCTURE, INTEGER
};

std::string prolog_term_type_to_string(prolog_term_type type);

struct prolog_term {
    explicit prolog_term(prolog_term_type type);
    explicit prolog_term(int integer);

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
    prolog_clause(term_index head, term_index body):
        head(head), body(body) {}

    explicit prolog_clause(term_index head):
        head(head), body(term_index::invalid()) {}

    term_index head; // Term
    term_index body; // Goal: -1 if not present
};

struct prolog_timestamp {
    prolog_timestamp(
        term_index term_index_,
        clause_index clause_index_,
        trail_index trail_index_
        ):
    term_index_(term_index_), clause_index_(clause_index_),
    trail_index_(trail_index_) {}

    term_index term_index_;
    clause_index clause_index_;
    trail_index trail_index_;
};

#endif // PROLOG_TYPES_H