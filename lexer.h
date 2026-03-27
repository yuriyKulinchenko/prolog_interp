#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <ostream>



// Tokens:
// SYMBOL, VARIABLE, PAREN_OPEN, PAREN_CLOSED, SQUARE_OPEN, SQUARE_CLOSED,

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
X(PIPE)

enum class token_type {
#define X(name) name,
    TOKEN_TYPE_LIST(X)
#undef X
};

inline std::string token_type_to_string(token_type type) {
    switch (type) {
#define X(name) case token_type::name: return #name;
        TOKEN_TYPE_LIST(X)
#undef X
    }
    return "UNKNOWN";
}

struct token {
    token_type type;
    std::string identifier; // Optionally present
};

inline bool is_identifier_token(token& token) {
    return token.type == token_type::SYMBOL || token.type == token_type::VARIABLE;
}

inline std::ostream& operator<<(std::ostream& stream, token& token) {
    stream << "{type: " << token_type_to_string(token.type);
    if (is_identifier_token(token)) {
        stream << ", identifier: " << token.identifier;
    }
    stream << '}';
    return stream;
}

class lexer {
public:
    explicit lexer(std::string source_code):
    source_code(std::move(source_code)), i(0) {}

    std::vector<token> run();


private:
    char peek();
    char next();
    char advance();
    char advance(int n);
    char consume(char c, const std::string& error_message);
    bool check(char c);
    bool match(char c);
    bool at_end();
    void emit_token(token_type type, const std::string& identifier = "");
    void handle_string(token_type type);
    static bool is_alpha_lower(char c);
    static bool is_alpha_capital(char c);
    static bool is_alpha(char c);
    static bool is_num(char c);
    static bool is_alphanum(char c);
    static bool is_whitespace(char c);

    std::vector<token> token_vector;
    std::string source_code;
    int i;
};

#endif //LEXER_H
