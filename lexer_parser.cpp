//
// Created by Yuriy Kulinchenko on 26/04/2026.
//

#include "lexer_parser.h"
#include "helper.h"

#include <iostream>

std::vector<node> lexer_parser::program(const std::string& source) {
    initialize(source);
    return parser_->program();
}

node lexer_parser::query(const std::string& source) {
    initialize(source);
    return parser_->query();
}

void lexer_parser::initialize(std::string source) {
    if (lexer_ == nullptr) {
        lexer_ = std::make_unique<lexer>(std::move(source));
        parser_ = std::make_unique<parser>(lexer_->run(), *lexer_);
    } else {
        lexer_->reset(std::move(source));
        parser_->reset(lexer_->run());
    }
}


