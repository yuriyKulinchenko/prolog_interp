#ifndef HELPER_H
#define HELPER_H

#include <ranges>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <format>
#include <iostream>
#include <functional>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define RESET   "\033[0m"

template <typename T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& vector) {
    stream << '[';
    if (vector.size() != 0) {
        stream << vector[0];
        for (int i = 1; i < vector.size(); i++) {
            stream << ", " << vector[i];
        }
    }
    return stream << ']';
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& stream, std::unordered_map<T, U>& map) {
    stream << '{';

    auto it = map.begin();
    if (it != map.end()) {
        stream << it->first << " -> " << it->second;
        ++it;
    }

    for (; it != map.end(); ++it) {
        stream << ", " << it->first << " -> " << it->second;
    }

    return stream << '}';
}

inline std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Failed to open file");

    std::streamsize size = file.tellg();
    file.seekg(0);

    std::string buffer(size, '\0');
    file.read(buffer.data(), size);

    return buffer;
}

inline std::string generate_select_pointer_string(std::string& s, int i) {
    int visual = 0;
    for (int j = 0; j <= i; j++) {
        if (s[j] == '\t') {
            visual += 4;
        } else {
            visual += 1;
        }
    }
    return std::format("{:>{}}", '^', visual);
}

inline std::string generate_position_error_string(
    std::string& s, int i, int line_number) {
    std::string pointer = generate_select_pointer_string(s, i);
    std::string line_number_string = std::to_string(line_number);
    std::string padding = std::string(line_number_string.size(), ' ');
    return std::format("{} │{}\n{} │{}",
                       line_number_string, s, padding, pointer);
}

template <typename... Args>
std::logic_error formatted_error(std::format_string<Args...> fmt,
                                 Args&&... args) {
    return std::logic_error(std::format(fmt, std::forward<Args>(args)...));
}

inline void print_green(const std::string& s) {
    std::cout << GREEN << s << RESET;
}

inline void print_red(const std::string& s) {
    std::cout << RED << s << RESET;
}

inline constexpr char subscript_digits[] = "₀₁₂₃₄₅₆₇₈₉";

inline std::string subscript_number(int n) {
    static constexpr const char* digits[] = {
        "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"
    };
    std::vector<const char*> parts;
    while (n != 0) {
        parts.push_back(digits[n % 10]);
        n /= 10;
    }
    std::string out;
    for (auto& part : std::ranges::reverse_view(parts))
        out += part;
    return out;
}

#endif //HELPER_H
