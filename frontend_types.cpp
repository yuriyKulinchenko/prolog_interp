#include <new>
#include <utility>
#include "frontend_types.h"

bool is_identifier_token(token& token) {
    return token.type == token_type::SYMBOL || token.type == token_type::VARIABLE;
}

bool is_integral_token(token& token) {
    return token.type == token_type::INTEGER;
}

std::ostream& operator<<(std::ostream& stream, token& token) {
    stream << '{' << token_type_to_string(token.type);
    if (is_identifier_token(token)) {
        stream << ", '" << token.identifier << "'";
    } else if (is_integral_token(token)) {
        stream << ", " << token.integer;
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
        case DISJUNCTION: return "OR";
        case CONJUNCTION: return "AND";
        default: return node_type_to_string(type);

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
      line_number(line_number) {}


// token

token::token(token_type type, text_position position)
    : type(type), integer(0), position(position) {}

token::token(token_type type, std::string identifier, text_position position)
    : type(type), identifier(std::move(identifier)), position(position) {}

token::token(token_type type, int integer, text_position position)
    : type(type), integer(integer), position(position) {}

token::token(const token& other)
    : type(other.type), position(other.position) {
    if (token_has_identifier(type)) {
        new (&identifier) std::string(other.identifier);
    } else {
        integer = other.integer;
    }
}

token::token(token&& other) noexcept
    : type(other.type), position(other.position) {
    if (token_has_identifier(type)) {
        new (&identifier) std::string(std::move(other.identifier));
    } else {
        integer = other.integer;
    }
}

token& token::operator=(const token& other) {
    if (this == &other) {
        return *this;
    }

    this->~token();

    type = other.type;
    position = other.position;

    if (token_has_identifier(type)) {
        new (&identifier) std::string(other.identifier);
    } else {
        integer = other.integer;
    }

    return *this;
}

token& token::operator=(token&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->~token();

    type = other.type;
    position = other.position;

    if (token_has_identifier(type)) {
        new (&identifier) std::string(std::move(other.identifier));
    } else {
        integer = other.integer;
    }

    return *this;
}

token::~token() {
    if (token_has_identifier(type)) {
        identifier.~basic_string();
    }
}


// node

node::node()
    : type(node_type::CUT), integer(0) {}

node::node(node_type type)
    : type(type), integer(0) {}

node::node(node_type type, std::string name)
    : type(type), name(std::move(name)) {}

node::node(node_type type, int integer)
    : type(type), integer(integer) {}

node::node(node_type type, std::vector<node> children)
    : type(type), integer(0), children(std::move(children)) {}

node::node(node_type type, std::string name, std::vector<node> children)
    : type(type), name(std::move(name)), children(std::move(children)) {}

node::node(const node& other)
    : type(other.type), children(other.children) {
    if (node_has_name(type)) {
        new (&name) std::string(other.name);
    } else if (node_has_integer(type)) {
        integer = other.integer;
    } else {
        integer = 0;
    }
}

node::node(node&& other) noexcept
    : type(other.type), children(std::move(other.children)) {
    if (node_has_name(type)) {
        new (&name) std::string(std::move(other.name));
    } else if (node_has_integer(type)) {
        integer = other.integer;
    } else {
        integer = 0;
    }
}

node& node::operator=(const node& other) {
    if (this == &other) {
        return *this;
    }

    this->~node();

    type = other.type;
    children = other.children;

    if (node_has_name(type)) {
        new (&name) std::string(other.name);
    } else if (node_has_integer(type)) {
        integer = other.integer;
    } else {
        integer = 0;
    }

    return *this;
}

node& node::operator=(node&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->~node();

    type = other.type;
    children = std::move(other.children);

    if (node_has_name(type)) {
        new (&name) std::string(std::move(other.name));
    } else if (node_has_integer(type)) {
        integer = other.integer;
    } else {
        integer = 0;
    }

    return *this;
}

node::~node() {
    if (node_has_name(type)) {
        name.~basic_string();
    }
}