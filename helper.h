#ifndef HELPER_H
#define HELPER_H

#include <ostream>
#include <vector>
#include <fstream>

template<typename T>
std::ostream& operator<<(std::ostream& stream, std::vector<T> vector) {
    stream << '[';
    if (vector.size() != 0) {
        stream << vector[0];
        for (int i = 1; i < vector.size(); i++) {
            stream << ", " << vector[i];
        }
    }
    return stream << ']';
}

inline std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open file");

    std::streamsize size = file.tellg();
    file.seekg(0);

    std::string buffer(size, '\0');
    file.read(buffer.data(), size);

    return buffer;
}

#endif //HELPER_H
