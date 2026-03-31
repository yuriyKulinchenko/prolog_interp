#include <iostream>
#include <fstream>
#include "helper.h"
#include  "lexer.h"
#include "parser.h"

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open file");

    std::streamsize size = file.tellg();
    file.seekg(0);

    std::string buffer(size, '\0');
    file.read(buffer.data(), size);

    return buffer;
}

int main() {
    std::string input = read_file("../source.txt");
    lexer lexer {input};
    auto vec = lexer.run();
    parser parser {vec};
    auto clauses = parser.run();
    std::cout << clauses;
}