#ifndef HELPER_H
#define HELPER_H

#include <ostream>
#include <vector>

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

#endif //HELPER_H
