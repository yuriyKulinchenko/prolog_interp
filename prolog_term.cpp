#include "prolog_term.h"

// prolog_variable

prolog_variable::prolog_variable()
    : type(prolog_variable_type::UNBOUND), index(0) {}

prolog_variable::prolog_variable(prolog_variable_type type, int index)
    : type(type), index(index) {}


// prolog_structure

prolog_structure::prolog_structure()
    : identifier_index(0) {}

prolog_structure::prolog_structure(int index)
    : identifier_index(index) {}


// storage

prolog_term::storage::storage() {}
prolog_term::storage::~storage() {}


// prolog_term

prolog_term::prolog_term(prolog_term_type type)
    : type(type) {

    if (type == prolog_term_type::STRUCTURE) {
        new (&as.structure) prolog_structure(0);
    } else {
        new (&as.variable) prolog_variable();
    }
}

prolog_term::prolog_term(prolog_term&& other) noexcept
    : type(other.type) {

    if (type == prolog_term_type::STRUCTURE) {
        new (&as.structure) prolog_structure(std::move(other.as.structure));
    } else {
        new (&as.variable) prolog_variable(other.as.variable);
    }
}

prolog_term& prolog_term::operator=(prolog_term&& other) noexcept {
    if (this == &other) return *this;

    destroy_active();
    type = other.type;

    if (type == prolog_term_type::STRUCTURE) {
        new (&as.structure) prolog_structure(std::move(other.as.structure));
    } else {
        new (&as.variable) prolog_variable(other.as.variable);
    }

    return *this;
}

prolog_term::~prolog_term() {
    destroy_active();
}


// helpers

bool prolog_term::is_variable() const {
    return type == prolog_term_type::VARIABLE;
}

bool prolog_term::is_anonymous_variable() const {
    return is_variable() &&
        as.variable.type == prolog_variable_type::ANONYMOUS;
}

bool prolog_term::is_unbound_variable() const {
    return is_variable() &&
        as.variable.type == prolog_variable_type::UNBOUND;
}

bool prolog_term::is_structure() const {
    return type == prolog_term_type::STRUCTURE;
}

void prolog_term::destroy_active() {
    if (type == prolog_term_type::STRUCTURE) {
        as.structure.~prolog_structure();
    } else {
        as.variable.~prolog_variable();
    }
}