#ifndef FRONTEND_TYPES_H
#define FRONTEND_TYPES_H

#include <string>
#include <vector>
#include <ostream>
#include <variant>

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
X(MORE_THAN)           \
X(LESS_THAN_EQUAL)     \
X(MORE_THAN_EQUAL)     \
X(EQUAL_COLON_EQUAL)   \
X(END_OF_FILE)

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

struct position_range {
    text_position start; // Inclusive
    text_position end; // Exclusive
};

struct token {
    token(token_type type, position_range range);
    token(token_type type, std::string identifier, position_range range);
    token(token_type type, int integer, position_range range);

    std::string& identifier();
    int integer();

    token_type type;
    std::variant<int, std::string> tagged_union;
    position_range range;
};

// NODES

enum class node_type {
#define X(name) name,
    NODE_TYPE_LIST(X)
#undef X
};

struct node {
    explicit node(const position_range& range);
    node(node_type type, const position_range& range);
    node(node_type type, std::string name, const position_range& range);
    node(node_type type, int integer, const position_range& range);
    node(node_type type, std::vector<node> children,
         const position_range& range);
    node(node_type type, std::string name, std::vector<node> children,
         const position_range& range);

    std::string& name();
    int integer();

    node_type type;
    std::variant<int, std::string> tagged_union;
    std::vector<node> children;
    position_range range;
};

// HELPER FUNCTIONS:

std::ostream& operator<<(std::ostream& stream, token& token);
bool is_identifier_token(token& token);

std::string node_type_to_string(node_type type);
std::string node_type_to_short_string(node_type type);


#endif //FRONTEND_TYPES_H
