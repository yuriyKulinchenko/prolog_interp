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

// prolog_term

prolog_term::prolog_term(prolog_term_type type): type(type) {
    switch (type) {
        case prolog_term_type::STRUCTURE:
            tagged_union = prolog_structure();
            break;

        case prolog_term_type::VARIABLE:
            tagged_union = prolog_variable();
            break;

        case prolog_term_type::INTEGER:
            tagged_union = 0;
            break;
    }
}

int prolog_term::integer() {
    return std::get<int>(tagged_union);
}

prolog_structure &prolog_term::structure() {
    return std::get<prolog_structure>(tagged_union);
}

prolog_variable &prolog_term::variable() {
    return std::get<prolog_variable>(tagged_union);
}




prolog_term::prolog_term(int integer)
    : type(prolog_term_type::INTEGER) {
    tagged_union = integer;
}

bool prolog_term::is_variable() const {
    return type == prolog_term_type::VARIABLE;
}

bool prolog_term::is_unbound_variable() {
    return is_variable() &&
        variable().type == prolog_variable_type::UNBOUND;
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
