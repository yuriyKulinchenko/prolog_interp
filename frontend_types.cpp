#include <new>
#include <utility>
#include "frontend_types.h"

bool is_identifier_token(token& token) {
    return token.type == token_type::SYMBOL || token.type ==
           token_type::VARIABLE;
}

bool is_integral_token(token& token) {
    return token.type == token_type::INTEGER;
}

std::ostream& operator<<(std::ostream& stream, token& token) {
    stream << '{' << token_type_to_string(token.type);
    if (is_identifier_token(token)) {
        stream << ", '" << token.identifier() << "'";
    } else if (is_integral_token(token)) {
        stream << ", " << token.integer();
    }
    return stream << '}';
}

std::string node_type_to_string(node_type type) {
    switch (type) {
#define X(name) case node_type::name: return #name;
    NODE_TYPE_LIST(X)
#undef X
    }
    return "UNKNOWN";
}

std::string token_type_to_string(token_type type) {
    switch (type) {
#define X(name) case token_type::name: return #name;
    TOKEN_TYPE_LIST(X)
#undef X
    }
    return "UNKNOWN";
}

std::string node_type_to_short_string(node_type type) {
    switch (type) {
        using enum node_type;
    case DISJUNCTION:
        return "OR";
    case CONJUNCTION:
        return "AND";
    default:
        return node_type_to_string(type);

    }
}

bool token_has_identifier(token_type type) {
    return type == token_type::SYMBOL ||
           type == token_type::VARIABLE;
}

bool node_has_name(node_type type) {
    return type == node_type::TERM ||
           type == node_type::VARIABLE;
}

bool node_has_integer(node_type type) {
    return type == node_type::INTEGER_TERM;
}

// text_position

text_position::text_position(
    int line_start_index,
    int pointer_index,
    int line_number)
    : line_start_index(line_start_index),
      pointer_index(pointer_index),
      line_number(line_number) {
}


// token

token::token(token_type type, position_range range)
    : type(type), tagged_union(0), range(range) {
}

token::token(token_type type, std::string identifier, position_range range)
    : type(type), tagged_union(std::move(identifier)), range(range) {
}

token::token(token_type type, int integer, position_range range)
    : type(type), tagged_union(integer), range(range) {
}

std::string& token::identifier() {
    return std::get<std::string>(tagged_union);
}

int token::integer() {
    return std::get<int>(tagged_union);
}

// node

node::node(const position_range& range)
    : type(node_type::CUT), tagged_union(0), range(range) {
}

node::node(node_type type, const position_range& range)
    : type(type), tagged_union(0), range(range) {
}

node::node(node_type type, std::string name, const position_range& range)
    : type(type), tagged_union(std::move(name)), range(range) {
}

node::node(node_type type, int integer, const position_range& range)
    : type(type), tagged_union(integer), range(range) {
}

node::node(node_type type, std::vector<node> children,
           const position_range& range)
    : type(type), tagged_union(0), children(std::move(children)), range(range) {
}

node::node(node_type type, std::string name, std::vector<node> children,
           const position_range& range)
    : type(type), tagged_union(std::move(name)),
      children(std::move(children)), range(range) {
}

std::string& node::name() {
    return std::get<std::string>(tagged_union);
}

int node::integer() {
    return std::get<int>(tagged_union);
}