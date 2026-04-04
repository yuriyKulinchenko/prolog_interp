#include <iostream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"
#include "unification_environment.h"

int main() {
    unification_environment environment {};
    environment.test_clauses("../source.txt");
}