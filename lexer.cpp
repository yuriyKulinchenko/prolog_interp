#include "lexer.h"
#include <iostream>

std::vector<token> lexer::run() {
    token_vector = {};
    while (!at_end()) {
        char c = advance();
        if (is_whitespace(c)) continue;
        switch (c) {
            using enum token_type;

            case '.': {
                emit_token(DOT);
                break;
            }

            case ',': {
                emit_token(COMMA);
                break;
            }

            case ';': {
                emit_token(SEMI_COLON);
                break;
            }

            case '|': {
                emit_token(PIPE);
            }

            case '(': {
                emit_token(PAREN_OPEN);
                break;
            }

            case ')': {
                emit_token(PAREN_CLOSED);
                break;
            }

            case '[': {
                emit_token(SQUARE_OPEN);
                break;
            }

            case ']': {
                emit_token(SQUARE_CLOSED);
                break;
            }

            case ':': {
                consume('-', "ERROR: Expect '-' to follow ':' in rule operator");
                emit_token(RULE_OPERATOR);
                break;
            }

            default: {
                // Either variable or symbol:
                i--;
                if (is_alpha_lower(c)) {
                    handle_string(SYMBOL);
                } else if (is_alpha_capital(c)) {
                    handle_string(VARIABLE);
                }

                else {
                    std::cerr << "ERROR: Unsupported character: '" << c << "'\n";
                    exit(10);
                }
            }
        }
    }
    return token_vector;
}

char lexer::peek() {
    return source_code[i];
}

char lexer::next() {
    return source_code[i + 1];
}


char lexer::advance() {
    return source_code[i++];
}

char lexer::advance(int n) {
    return source_code[i+=n];
}


bool lexer::check(char c) {
    return source_code[i] == c;
}

bool lexer::match(char c) {
    if (check(c)) {
        i++;
        return true;
    }
    return false;
}

char lexer::consume(char c, const std::string& error_message) {
    if (!match(c)) throw std::logic_error(error_message);
    return c;
}

bool lexer::is_alpha_lower(char c) {
    return c >= 'a' && c <= 'z' || c == '_';
}

bool lexer::is_alpha_capital(char c) {
    return c >= 'A' && c <= 'Z';
}

bool lexer::is_alpha(char c) {
    return is_alpha_lower(c) || is_alpha_capital(c);
}

bool lexer::is_num(char c) {
    return c >= '0' && c <= '9';
}

bool lexer::is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

bool lexer::is_alphanum(char c) {
    return is_alpha(c) || is_num(c);
}

bool lexer::at_end() {
    return source_code.length() == i;
}

void lexer::emit_token(token_type type, const std::string& identifier) {
    token_vector.emplace_back(type, identifier);
}

void lexer::handle_string(token_type type) {
    int start = i;
    while(is_alphanum(peek())) {
        advance();
    }
    token_vector.emplace_back(type, source_code.substr(start, i-start));
}





