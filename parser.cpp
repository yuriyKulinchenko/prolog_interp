#include "parser.h"
#include "helper.h"
#include <optional>

// TODO: std::logic_error is deprecated here

node create_transparent_list(node_type type, std::vector<node> nodes,
                             const char* name = "") {
    if (nodes.size() == 1) {
        return nodes[0];
    }
    return node{type, name, nodes};
}

std::vector<node> parser::run() {
    // There could be more to this later
    return program();
}

void parser::reset(std::vector<token>&& token_vector) {
    this->token_vector = std::move(token_vector);
    goal_vector.clear();
    i = 0;
}

std::vector<node> parser::program() {
    std::vector<node> clauses{};
    while (!at_end()) {
        clauses.push_back(clause());
    }
    return clauses;
}

// Clause ::= Term ':-' GoalExpr '.' | Term '.'

node parser::clause() {
    node term_instance = term();
    if (term_instance.type != node_type::TERM) {
        throw parser_error(
            "Left side of clause must be an atom or compound term");
    }

    if (term_instance.name() == "fail") {
        throw parser_error("'fail' cannot be used as the head of a clause");
    }

    std::optional<node> goal_instance;
    if (match(token_type::RULE_OPERATOR))
        goal_instance = goalExpr();
    consume(token_type::DOT, "Expect '.' after clause", previous());

    node clause_instance = {node_type::CLAUSE, {term_instance}};
    if (goal_instance)
        clause_instance.children.push_back(*goal_instance);
    return clause_instance;
}

// GoalExpr ::= Disjunction

node parser::goalExpr() {
    return {node_type::GOAL, {disjunction()}};
}

// Disjunction ::= Conjunction (';' Conjunction)*

node parser::disjunction() {
    std::vector nodes{conjunction()};
    while (match(token_type::SEMI_COLON)) {
        nodes.push_back(conjunction());
    }

    return create_transparent_list(node_type::DISJUNCTION,
                                   std::move(nodes), ";");
}

// Conjunction ::= SimpleGoal (',' SimpleGoal)*

node parser::conjunction() {
    std::vector nodes{simpleGoal()};
    while (match(token_type::COMMA)) {
        nodes.push_back(simpleGoal());
    }

    return create_transparent_list(node_type::CONJUNCTION,
                                   std::move(nodes), ",");
}

// SimpleGoal ::= Term | '(' GoalExpr ')' | '!'

node parser::simpleGoal() {
    if (match(token_type::EXCLAMATION_MARK)) {
        return node{node_type::CUT, "!"};
    }

    if (match(token_type::PAREN_OPEN)) {
        token& open_paren = previous();
        node inner = goalExpr();
        consume(token_type::PAREN_CLOSED, "Unmatched bracket", open_paren);
        return inner;
    }

    return term();
}

// Term ::= Sum (('='|'is'|'<'|'>'|'=<'|'>=') Sum)?

node parser::term() {
    node left = sum();
    using enum token_type;
    if (match(EQUAL) || match(IS)
        || match(LESS_THAN) || match(MORE_THAN)
        || match(LESS_THAN_EQUAL) || match(MORE_THAN_EQUAL)
    ) {
        std::string identifier_string;
        switch (previous().type) {
        case EQUAL: {
            identifier_string = "=";
            break;
        }
        case IS: {
            identifier_string = "is";
            break;
        }
        case LESS_THAN: {
            identifier_string = "<";
            break;
        }
        case MORE_THAN: {
            identifier_string = ">";
            break;
        }
        case MORE_THAN_EQUAL: {
            identifier_string = ">=";
            break;
        }
        case LESS_THAN_EQUAL: {
            identifier_string = "=<";
            break;
        }
        default: {
            identifier_string = "";
        }
        }
        node right = term();
        return {node_type::TERM, identifier_string, {left, right}};
    }
    return left;
}

// Sum ::= Product (('+'|'-') Sum)?

node parser::sum() {
    node left = product();
    if (match(token_type::PLUS) || match(token_type::MINUS)) {
        std::string identifier_string;
        if (previous().type == token_type::PLUS) {
            identifier_string = "+";
        } else {
            identifier_string = "-";
        }
        node right = sum();
        return {node_type::TERM, identifier_string, {left, right}};
    }
    return left;
}

// Product ::= SimpleTerm (('*'|'/') Product)?

node parser::product() {
    node left = simple_term();
    if (match(token_type::STAR) || match(token_type::SLASH)) {
        std::string identifier_string;
        if (previous().type == token_type::STAR) {
            identifier_string = "*";
        } else {
            identifier_string = "/";
        }
        node right = product();
        return {node_type::TERM, identifier_string, {left, right}};
    }
    return left;
}

// SimpleTerm ::= variable | ('+'|'-')? integer
// | identifier | identifier '(' Elements ')' | List

node parser::simple_term() {
    if (check(token_type::VARIABLE)) {
        return node{node_type::VARIABLE, advance().identifier()};
    }

    // Unary '+':
    if (check(token_type::PLUS)) {
        token& erroneous_token = advance();
        int value = consume(
            token_type::INTEGER,
            "Expect integer to follow unary '+'",
            erroneous_token).integer();
        return {node_type::INTEGER_TERM, value};
    }

    // Unary '-':
    if (check(token_type::MINUS)) {
        token& erroneous_token = advance();
        int value = consume(
            token_type::INTEGER,
            "Expect integer to follow unary '-'",
            erroneous_token).integer();
        return {node_type::INTEGER_TERM, -value};
    }

    // Regular integer:
    if (check(token_type::INTEGER)) {
        return {node_type::INTEGER_TERM, advance().integer()};
    }

    // Parsing identifier:
    if (check(token_type::SYMBOL)) {
        std::string identifier_name = advance().identifier();
        std::optional<std::vector<node>> nodes;
        if (match(token_type::PAREN_OPEN)) {
            token& open_paren = previous();
            nodes = elements();
            consume(token_type::PAREN_CLOSED, "Unmatched bracket", open_paren);
        }

        node term_instance = {node_type::TERM, identifier_name};
        term_instance.children = nodes ? *nodes : std::vector<node>{};
        return term_instance;
    }

    // Parsing list:

    if (match(token_type::SQUARE_OPEN)) {
        return list(previous());
    }

    throw parser_error(
        "Unrecognized term: term must be a variable, identifier or list");
}

node parser::list(token& token_instance) {
    // Empty list:
    if (match(token_type::SQUARE_CLOSED)) {
        return {node_type::TERM, "[]"};
    }

    std::vector<node> nodes = elements();
    // Elements have to be converted into a linked list:
    node head = {node_type::TERM, ".", {nodes[0]}};
    std::vector<node>* tail = &head.children;
    for (int i = 1; i < nodes.size(); i++) {
        tail->push_back({node_type::TERM, ".", {nodes[i]}});
        tail = &(*tail)[1].children;
    }

    if (match(token_type::PIPE)) {
        tail->push_back(term());
    } else {
        tail->emplace_back(node_type::TERM, "[]");
    }

    consume(token_type::SQUARE_CLOSED, "unmatched square bracket",
            token_instance);
    return head;
}

std::vector<node> parser::elements() {
    std::vector nodes{term()};
    while (match(token_type::COMMA)) {
        nodes.push_back(term());
    }
    return nodes;
}

token& parser::peek() {
    return token_vector[i];
};

token& parser::previous() {
    return token_vector[i - 1];
}


token& parser::next() {
    return token_vector[i + 1];
};

token& parser::advance() {
    return token_vector[i++];
};

token& parser::advance(int n) {
    return token_vector[i += n];
};

token& parser::consume(token_type type, const std::string& error_message,
                       token& erroneous_token) {
    if (peek().type != type) {
        throw parser_error(error_message, erroneous_token);
    }
    return advance();
};

bool parser::check(token_type type) {
    return peek().type == type;
}

bool parser::match(token_type type) {
    if (check(type)) {
        i++;
        return true;
    }
    return false;
}

bool parser::at_end() {
    return token_vector.size() == i;
}

std::ostream& bracketed_elements_log(std::ostream& stream,
                                     std::vector<node>& nodes) {
    stream << '(';
    if (!nodes.empty()) {
        stream << nodes[0];
        for (int i = 1; i < nodes.size(); i++) {
            stream << ", " << nodes[i];
        }
    }
    return stream << ')';
}

std::ostream& lisp_list_log(std::ostream& stream, std::string& identifier,
                            std::vector<node>& nodes) {
    stream << '(' << identifier;
    for (auto& node : nodes)
        stream << ", " << node;
    return stream << ')';
}

std::ostream& operator<<(std::ostream& stream, node& node) {
    if (node.type == node_type::VARIABLE) {
        return stream << node.name();
    }

    if (node.type == node_type::TERM) {
        stream << node.name();
        if (node.children.size() > 1) {
            bracketed_elements_log(stream, node.children);
        }
        return stream;
    }

    if (node.type == node_type::CUT) {
        return stream << '!';
    }

    std::string identifier = node_type_to_short_string(node.type);
    lisp_list_log(stream, identifier, node.children);

    return stream;
}

std::logic_error parser::parser_error(const std::string& error_message) {
    return parser_error(error_message, previous());
}

std::logic_error parser::parser_error(const std::string& error_message,
                                      const token& token_instance) {
    std::string current_line{
        lexer_instance.fetch_line_at(token_instance.position.line_start_index)};

    std::string position_error = generate_position_error_string(
        current_line,
        token_instance.position.pointer_index - token_instance.position.
        line_start_index,
        token_instance.position.line_number);

    return std::logic_error(
        std::format("\nPARSER ERROR: {}\n{}",
                    error_message, position_error));
}

node parser::query() {
    node goal = goalExpr();
    consume(token_type::DOT, "Expect '.' following query", previous());
    if (!at_end()) {
        throw parser_error("Expect nothing following query");
    }
    return goal;
}
