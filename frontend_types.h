#ifndef FRONTEND_TYPES_H
#define FRONTEND_TYPES_H

#include <string>
#include <vector>
#include <ostream>

#define TOKEN_TYPE_LIST(X) \
X(SYMBOL)              \
X(VARIABLE)            \
X(PAREN_OPEN)          \
X(PAREN_CLOSED)        \
X(SQUARE_OPEN)         \
X(SQUARE_CLOSED)       \
X(DOT)                 \
X(COMMA)               \
X(SEMI_COLON)          \
X(RULE_OPERATOR)       \
X(PIPE)                \
X(EXCLAMATION_MARK)    \
X(QUESTION_MARK)       \
X(PLUS)                \
X(MINUS)               \
X(STAR)                \
X(SLASH)               \
X(NOT)                 \
X(EQUAL)               \
X(NOT_EQUAL)           \
X(IS)                  \
X(INTEGER)             \
X(LESS_THAN)           \
X(MORE_THAN)

#define NODE_TYPE_LIST(X) \
X(CLAUSE)              \
X(GOAL)                \
X(TERM)                \
X(INTEGER_TERM)        \
X(VARIABLE)            \
X(CONJUNCTION)         \
X(DISJUNCTION)         \
X(CUT)

// TOKENS

enum class token_type {
#define X(name) name,
    TOKEN_TYPE_LIST(X)
#undef X
};

std::string token_type_to_string(token_type type);

struct text_position {
    text_position(int line_start_index, int pointer_index, int line_number);

    int line_start_index;
    int pointer_index;
    int line_number;
};

struct token {
    token(token_type type, text_position position);
    token(token_type type, std::string identifier, text_position position);
    token(token_type type, int integer, text_position position);

    token(const token& other);
    token(token&& other) noexcept;
    token& operator=(const token& other);
    token& operator=(token&& other) noexcept;
    ~token();

    token_type type;

    union {
        std::string identifier;
        int integer;
    };

    text_position position;
};

// NODES

enum class node_type {
#define X(name) name,
    NODE_TYPE_LIST(X)
#undef X
};

struct node {
    node();
    explicit node(node_type type);
    node(node_type type, std::string name);
    node(node_type type, int integer);
    node(node_type type, std::vector<node> children);
    node(node_type type, std::string name, std::vector<node> children);

    node(const node& other);
    node(node&& other) noexcept;
    node& operator=(const node& other);
    node& operator=(node&& other) noexcept;
    ~node();

    node_type type;

    union {
        std::string name;
        int integer;
    };

    std::vector<node> children;
};

// HELPER FUNCTIONS:

std::ostream& operator<<(std::ostream& stream, token& token);
bool is_identifier_token(token& token);

std::string node_type_to_string(node_type type);
std::string node_type_to_short_string(node_type type);



#endif //FRONTEND_TYPES_H
