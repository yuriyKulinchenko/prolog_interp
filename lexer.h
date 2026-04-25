#ifndef LEXER_H
#define LEXER_H

// #define LEXER_DEBUG

#include <vector>
#include "frontend_types.h"

class lexer {
public:
    explicit lexer(std::string&& source_code):
        source_code(std::move(source_code)),
        i(0), line_number(1), current_line_index(0) {
    }

    std::vector<token> run();
    void reset(std::string&& source_code);
    std::string_view fetch_line_at(int index);

private:
    char peek();
    char next();
    char advance();
    char advance(int n);
    char consume(char c, const std::string& error_message);
    bool check(char c);
    bool match(char c);
    bool at_end();

    void emit_token(token_type type, const std::string& identifier);
    void emit_token(token_type type, int number);
    void emit_token(token_type type);

    void emit_string();
    void emit_integer();

    static bool is_alpha_lower(char c);
    static bool is_alpha_capital(char c);
    static bool is_alpha(char c);
    static bool is_num(char c);
    static bool is_alphanum(char c);
    static bool is_whitespace(char c);

    std::logic_error lexer_error(const std::string& error_message);
    std::string_view fetch_current_line();
    [[nodiscard]] text_position get_text_position() const;

    std::vector<token> token_vector;
    std::string source_code;
    int i;
    int line_number;
    int current_line_index;
};
#endif //LEXER_H
