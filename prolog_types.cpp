#include "prolog_types.h"
#include "helper.h"
#include <utility>
// prolog_variable

prolog_var::prolog_var(identifier_index identifier):
    identifier(identifier) {
}

prolog_var::prolog_var(identifier_index identifier, int version):
    identifier(identifier), version(version) {
}

prolog_var::prolog_var(prolog_var_type type, term_index index,
                       identifier_index identifier, int version)
    : type(type),
      index(index),
      identifier(identifier),
      version(version) {
}


// prolog_structure

prolog_struct::prolog_struct(): num_children(0) {
}

prolog_struct::prolog_struct(size_t num_children):
    num_children(num_children),
    children(num_children > inline_capacity
                 ? new term_index[num_children]
                 : nullptr) {
}

prolog_struct::prolog_struct(size_t num_children, identifier_index index):
    index(index),
    num_children(num_children),
    children(num_children > inline_capacity
                 ? new term_index[num_children]
                 : nullptr) {
}

prolog_struct::~prolog_struct() {
    if (num_children > inline_capacity) {
        delete[] children;
    }
}

term_index& prolog_struct::operator[](size_t i) {
    if (num_children > inline_capacity) {
        return children[i];
    }
    return inline_children[i];
}

const term_index& prolog_struct::operator[](size_t i) const {
    if (num_children > inline_capacity) {
        return children[i];
    }
    return inline_children[i];
}

term_index& prolog_struct::at(size_t i) {
    return operator[](i);
}

const term_index& prolog_struct::at(size_t i) const {
    return operator[](i);
}

term_index prolog_struct::left() {
    return inline_children[0];
}

term_index prolog_struct::right() {
    return inline_children[1];
}

prolog_struct::prolog_struct(prolog_struct&& other) noexcept
    : index(other.index), num_children(other.num_children) {
    if (num_children > inline_capacity) {
        children = other.children;
        other.children = nullptr;
        other.num_children = 0;
    } else {
        for (size_t i = 0; i < num_children; i++) {
            inline_children[i] = other.inline_children[i];
        }
    }
}

prolog_struct::prolog_struct(const prolog_struct& other)
    : index(other.index), num_children(other.num_children) {
    if (num_children > inline_capacity) {
        children = new term_index[num_children];
        for (size_t i = 0; i < num_children; i++) {
            children[i] = other.children[i];
        }
    } else {
        for (size_t i = 0; i < num_children; i++) {
            inline_children[i] = other.inline_children[i];
        }
    }
}

prolog_struct& prolog_struct::operator=(const prolog_struct& other) {
    if (this != &other) {
        if (num_children > inline_capacity) {
            delete[] children;
        }

        index = other.index;
        num_children = other.num_children;

        if (num_children > inline_capacity) {
            children = new term_index[num_children];
            for (size_t i = 0; i < num_children; i++) {
                children[i] = other.children[i];
            }
        } else {
            for (size_t i = 0; i < num_children; i++) {
                inline_children[i] = other.inline_children[i];
            }
        }
    }
    return *this;
}

prolog_struct& prolog_struct::operator=(prolog_struct&& other) noexcept {
    if (this != &other) {
        if (num_children > inline_capacity) {
            delete[] children;
        }

        index = other.index;
        num_children = other.num_children;

        if (num_children > inline_capacity) {
            children = other.children;
            other.children = nullptr;
            other.num_children = 0;
        } else {
            for (size_t i = 0; i < num_children; i++) {
                inline_children[i] = other.inline_children[i];
            }
        }
    }
    return *this;
}

// prolog_term

prolog_term::prolog_term(prolog_term_type type): type(type) {
    switch (type) {
    case prolog_term_type::STRUCTURE:
        tagged_union = prolog_struct();
        break;

    case prolog_term_type::INTEGER:
        tagged_union = 0;
        break;

    case prolog_term_type::VARIABLE:
        throw std::logic_error(
            "Error: enum VARIABLE cannot be passed to constructor of prolog_term");
    }
}

prolog_term::prolog_term(prolog_var variable):
    type(prolog_term_type::VARIABLE),
    tagged_union(variable) {
}

prolog_term::prolog_term(int integer)
    : type(prolog_term_type::INTEGER),
      tagged_union(integer) {
}

int prolog_term::integer() {
    return std::get<int>(tagged_union);
}

prolog_struct& prolog_term::structure() {
    return std::get<prolog_struct>(tagged_union);
}

prolog_var& prolog_term::variable() {
    return std::get<prolog_var>(tagged_union);
}

bool prolog_term::is_variable() const {
    return type == prolog_term_type::VARIABLE;
}

bool prolog_term::is_unbound_variable() {
    return is_variable() &&
           variable().type == prolog_var_type::UNBOUND;
}

bool prolog_term::is_bound_variable() {
    return is_variable() &&
           variable().type == prolog_var_type::BOUND;
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
    case STRUCTURE:
        return "Structure";
    case VARIABLE:
        return "Variable";
    case INTEGER:
        return "Integer";
    }
    throw std::logic_error("ERROR: unrecognized prolog_term_type");
}