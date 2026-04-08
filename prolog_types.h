#ifndef PROLOG_TYPES_H
#define PROLOG_TYPES_H

#include <vector>

enum class prolog_variable_type {
    BOUND, UNBOUND
};

struct prolog_variable {
    prolog_variable();
    prolog_variable(prolog_variable_type type, int index);

    prolog_variable_type type;
    int index;
};

struct prolog_structure {
    prolog_structure();
    explicit prolog_structure(int index);

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

    prolog_term(const prolog_term&);
    prolog_term& operator=(const prolog_term&);

    prolog_term(prolog_term&& other) noexcept;
    prolog_term& operator=(prolog_term&& other) noexcept;

    ~prolog_term();

    [[nodiscard]] bool is_variable() const;
    [[nodiscard]] bool is_unbound_variable() const;
    [[nodiscard]] bool is_structure() const;
    [[nodiscard]] bool is_integer() const;
    [[nodiscard]] bool is_ground() const;

    prolog_term_type type;

    union storage {
        prolog_structure structure;
        prolog_variable variable;
        int integer {};

        storage();
        ~storage();
    } as;

private:
    void destroy_active();
};

struct prolog_clause {
    prolog_clause(int head, int body):
        head(head), body(body) {}

    explicit prolog_clause(int head):
        head(head), body(-1) {}

    int head; // Term
    int body; // Goal: -1 if not present
};

#endif // PROLOG_TYPES_H