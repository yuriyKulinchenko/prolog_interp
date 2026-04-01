#ifndef PROLOG_TERM_H
#define PROLOG_TERM_H

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
    VARIABLE, STRUCTURE
};

struct prolog_term {
    explicit prolog_term(prolog_term_type type);

    prolog_term(const prolog_term&) = delete;
    prolog_term& operator=(const prolog_term&) = delete;

    prolog_term(prolog_term&& other) noexcept;
    prolog_term& operator=(prolog_term&& other) noexcept;

    ~prolog_term();

    [[nodiscard]] bool is_variable() const;
    [[nodiscard]] bool is_structure() const;

    prolog_term_type type;

    union storage {
        prolog_structure structure;
        prolog_variable variable;

        storage();
        ~storage();
    } as;

private:
    void destroy_active();
};

#endif // PROLOG_TERM_H