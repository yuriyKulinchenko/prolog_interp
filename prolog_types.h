#ifndef PROLOG_TYPES_H
#define PROLOG_TYPES_H

#include <vector>
#include <variant>

enum class prolog_var_type {
    BOUND, UNBOUND
};

struct prolog_var {
    prolog_var();
    prolog_var(prolog_var_type type, int index);

    prolog_var_type type;
    int index;
};

struct prolog_struct {
    prolog_struct();
    explicit prolog_struct(int index);

    int identifier_index;
    std::vector<int> children;
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
    [[nodiscard]] bool is_structure() const;
    [[nodiscard]] bool is_integer() const;
    [[nodiscard]] bool is_ground() const;

    prolog_term_type type;
    std::variant<int, prolog_struct, prolog_var> tagged_union;
};

struct prolog_clause {
    prolog_clause(int head, int body):
        head(head), body(body) {}

    explicit prolog_clause(int head):
        head(head), body(-1) {}

    int head; // Term
    int body; // Goal: -1 if not present
};

struct prolog_timestamp {
    prolog_timestamp(
        int term_index,
        int clause_index,
        int trail_index
        ):
    term_index(term_index), clause_index(clause_index),
    trail_index(trail_index) {}

    int term_index;
    int clause_index;
    int trail_index;
};

#endif // PROLOG_TYPES_H