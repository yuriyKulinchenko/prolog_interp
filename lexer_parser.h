#ifndef LEXER_PARSER_H
#define LEXER_PARSER_H

#include "lexer.h"
#include "parser.h"

class lexer_parser {
public:
    lexer_parser() = default;

    std::vector<node> program(const std::string& source);
    node query(const std::string& source);

    // Not copyable:
    lexer_parser(const lexer_parser&) = delete;
    void operator=(const lexer_parser&) = delete;

private:
    void initialize(std::string source);

    std::unique_ptr<lexer> lexer_;
    std::unique_ptr<parser> parser_;
};

#endif //LEXER_PARSER_H
