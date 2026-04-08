#include "prolog_types.h"
#include "helper.h"
#include <utility>
// prolog_variable

prolog_variable::prolog_variable()
    : type(prolog_variable_type::UNBOUND), index(0) {}

prolog_variable::prolog_variable(prolog_variable_type type, int index)
    : type(type), index(index) {}


// prolog_structure

prolog_structure::prolog_structure()
    : identifier_index(-1) {}

prolog_structure::prolog_structure(int index)
    : identifier_index(index) {}

// storage

prolog_term::storage::storage() {}
prolog_term::storage::~storage() {}

void prolog_term::destroy_active() {
    switch (type) {
        case prolog_term_type::STRUCTURE:
            as.structure.~prolog_structure();
            break;

        case prolog_term_type::VARIABLE:
            as.variable.~prolog_variable();
            break;

        case prolog_term_type::INTEGER:
            break;
    }
}

// constructors

prolog_term::prolog_term(prolog_term_type type)
    : type(type) {
    switch (type) {
        case prolog_term_type::STRUCTURE:
            new (&as.structure) prolog_structure();
            break;

        case prolog_term_type::VARIABLE:
            new (&as.variable) prolog_variable();
            break;

        case prolog_term_type::INTEGER:
            as.integer = 0;
            break;
    }
}

prolog_term::prolog_term(int integer)
    : type(prolog_term_type::INTEGER) {
    as.integer = integer;
}


// copy constructor

prolog_term::prolog_term(const prolog_term& other)
    : type(other.type) {
    switch (type) {
        case prolog_term_type::STRUCTURE:
            new (&as.structure) prolog_structure(other.as.structure);
            break;

        case prolog_term_type::VARIABLE:
            new (&as.variable) prolog_variable(other.as.variable);
            break;

        case prolog_term_type::INTEGER:
            as.integer = other.as.integer;
            break;
    }
}


// move constructor

prolog_term::prolog_term(prolog_term&& other) noexcept
    : type(other.type) {
    switch (type) {
        case prolog_term_type::STRUCTURE:
            new (&as.structure) prolog_structure(std::move(other.as.structure));
            break;

        case prolog_term_type::VARIABLE:
            new (&as.variable) prolog_variable(std::move(other.as.variable));
            break;

        case prolog_term_type::INTEGER:
            as.integer = other.as.integer;
            break;
    }
}

// copy assignment

prolog_term& prolog_term::operator=(const prolog_term& other) {
    if (this == &other) {
        return *this;
    }

    destroy_active();
    type = other.type;

    switch (type) {
        case prolog_term_type::STRUCTURE:
            new (&as.structure) prolog_structure(other.as.structure);
            break;

        case prolog_term_type::VARIABLE:
            new (&as.variable) prolog_variable(other.as.variable);
            break;

        case prolog_term_type::INTEGER:
            as.integer = other.as.integer;
            break;
    }

    return *this;
}

// move assignment

prolog_term& prolog_term::operator=(prolog_term&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_active();
    type = other.type;

    switch (type) {
        case prolog_term_type::STRUCTURE:
            new (&as.structure) prolog_structure(std::move(other.as.structure));
            break;

        case prolog_term_type::VARIABLE:
            new (&as.variable) prolog_variable(std::move(other.as.variable));
            break;

        case prolog_term_type::INTEGER:
            as.integer = other.as.integer;
            break;
    }

    return *this;
}

// destructor

prolog_term::~prolog_term() {
    destroy_active();
}

// helpers

bool prolog_term::is_variable() const {
    return type == prolog_term_type::VARIABLE;
}

bool prolog_term::is_unbound_variable() const {
    return is_variable() &&
        as.variable.type == prolog_variable_type::UNBOUND;
}

bool prolog_term::is_structure() const {
    return type == prolog_term_type::STRUCTURE;
}

bool prolog_term::is_integer() const {
    return type == prolog_term_type::INTEGER;
}

bool prolog_term::is_ground() const {
    return is_structure() || is_integer();
}

std::string prolog_term_type_to_string(prolog_term_type type) {
    switch (type) {
        using enum prolog_term_type;
        case STRUCTURE: return "Structure";
        case VARIABLE: return "Variable";
        case INTEGER: return "Integer";
    }
    throw std::logic_error("ERROR: unrecognized prolog_term_type");
}
