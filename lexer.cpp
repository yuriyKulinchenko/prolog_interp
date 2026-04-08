#include "lexer.h"
#include "helper.h"
#include <iostream>

std::vector<token> lexer::run() {
    token_vector = {};
    while (!at_end()) {
        char c = advance();

        if (c == '\n') {
            line_number++;
            current_line_index = i;
        }

        if (is_whitespace(c)) continue;
        if (c == '%') {
            while (!at_end() && peek() != '\n') {
                advance();
            }
            continue;
        }

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
                break;
            }

            case '!': {
                emit_token(EXCLAMATION_MARK);
                break;
            }

            case '?': {
                emit_token(QUESTION_MARK);
                break;
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

            case '+': {
                emit_token(PLUS);
                break;
            }

            case '-': {
                emit_token(MINUS);
                break;
            }

            case '*': {
                emit_token(STAR);
                break;
            }

            case '/': {
                emit_token(SLASH);
                break;
            }

            case '=': {
                emit_token(EQUAL);
                break;
            }

            case '\\': {
                switch (advance()) {
                    case '=': emit_token(NOT_EQUAL); break;
                    case '+': emit_token(NOT); break;
                    default:
                        throw lexer_error("Expect '\\' to be followed by '=' or '+'");
                }
                break;
            }


            case ':': {
                consume('-', "Expect '-' to follow ':' in rule operator");
                emit_token(RULE_OPERATOR);
                break;
            }

            default: {
                // Either variable or symbol:
                i--;
                if (is_alpha_lower(c) || is_num(c)) {
                    token& t = handle_string(SYMBOL);
                    if (t.identifier == "_") t.type = VARIABLE;
                } else if (is_alpha_capital(c)) {
                    handle_string(VARIABLE);
                }

                else {
                    throw lexer_error(std::format("Unsupported character: '{}'", c));
                }
            }
        }
    }
    return token_vector;
}

void lexer::reset(std::string&& source_code) {
    source_code = std::move(source_code);
    token_vector.clear();
    i = 0;
    current_line_index = 0;
    line_number = 0;
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
    if (!match(c)) throw lexer_error(error_message);
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
    token_vector.emplace_back(type, identifier, get_text_position());
}

token& lexer::handle_string(token_type type) {
    int start = i;
    while(is_alphanum(peek())) {
        advance();
    }
    emit_token(type, source_code.substr(start, i - start));
    return token_vector[token_vector.size() - 1];
}

std::logic_error lexer::lexer_error(const std::string& error_message) {
    std::string current_line {fetch_current_line()};

    std::string position_error = generate_position_error_string(
        current_line,
        i - current_line_index,
        line_number);

    return std::logic_error(
        std::format("\nLEXER ERROR: {}\n{}",
        error_message, position_error));
}

std::string_view lexer::fetch_current_line() {
   return fetch_line_at(current_line_index);
}

std::string_view lexer::fetch_line_at(int index) {
    size_t start = index;
    size_t j = start;

    while (j < source_code.size() && source_code[j] != '\n') {
        j++;
    }

    return {source_code.data() + start, j - start};
}

text_position lexer::get_text_position() const {
    return {current_line_index, i - 1, line_number};
}






