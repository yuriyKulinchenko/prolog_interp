#ifndef STRONG_INDICES_H
#define STRONG_INDICES_H

#include <cstddef>
#include <functional>
#include <limits>

#include "helper.h"

/*

The purpose of strong_index is to provide a zero-overhead alias for size_t
which has the property that strong_index<T> is only convertible to
strong_index<U> if T = U. This prevents misuse of index variables, as you
cannot mistakenly use one strong index in place of another.

strong_vector is a zero-overhead wrapper around std::vector which takes
2 template parameters: a strong_index type, and an element type.
strong_vector<strong_index<T>, U> can only be accessed with indices of
the type strong_index<T>.

*/

template <typename Tag>
struct strong_index {
    explicit constexpr strong_index(size_t v) noexcept : value(v) {
    }

    strong_index() = default;

    template <typename OtherTag>
    strong_index(strong_index<OtherTag>) = delete;

    [[nodiscard]] constexpr size_t raw() const noexcept { return value; }

    constexpr bool operator==(strong_index other) const noexcept {
        return value == other.value;
    }

    constexpr bool operator!=(strong_index other) const noexcept {
        return value != other.value;
    }

    constexpr bool operator<(strong_index other) const noexcept {
        return value < other.value;
    }

    constexpr bool operator<=(strong_index other) const noexcept {
        return value <= other.value;
    }

    constexpr bool operator>(strong_index other) const noexcept {
        return value > other.value;
    }

    constexpr bool operator>=(strong_index other) const noexcept {
        return value >= other.value;
    }

    constexpr strong_index operator+(size_t n) const noexcept {
        return strong_index(value + n);
    }

    constexpr strong_index operator-(size_t n) const noexcept {
        return strong_index(value - n);
    }

    constexpr strong_index& operator++() noexcept {
        ++value;
        return *this;
    }

    constexpr strong_index operator++(int) noexcept {
        auto tmp = *this;
        ++value;
        return tmp;
    }

    constexpr strong_index& operator--() noexcept {
        --value;
        return *this;
    }

    constexpr strong_index operator--(int) noexcept {
        auto tmp = *this;
        --value;
        return tmp;
    }

    static consteval strong_index invalid() noexcept {
        return strong_index{std::numeric_limits<size_t>::max()};
    }

    size_t value{};
};

template <typename Tag>
struct std::hash<strong_index<Tag>> {
    size_t operator()(strong_index<Tag> idx) const noexcept {
        return std::hash<size_t>{}(idx.value);
    }
};

template <typename Tag>
std::ostream& operator<<(std::ostream& stream, strong_index<Tag> index) {
    if (index == strong_index<Tag>::invalid()) {
        return stream << "null";
    }
    return stream << index.raw();
}

template<typename IndexType, typename ValueType>
class strong_vector;

template<typename Tag, typename ValueType>
class strong_vector<strong_index<Tag>, ValueType> {
public:
    template<typename... Args>
    explicit strong_vector(Args&&... args):
        vector_(std::forward<Args>(args)...) {
    }

    ValueType& operator[](strong_index<Tag> n) {
        return vector_[n.raw()];
    }

    const ValueType& operator[](strong_index<Tag> n) const {
        return vector_[n.raw()];
    }

    [[nodiscard]] size_t size() const {
        return vector_.size();
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        vector_.emplace_back(std::forward<Args>(args)...);
    }

    void push_back(const ValueType& val) {
        vector_.push_back(val);
    }

    void push_back(ValueType&& val) {
        vector_.push_back(std::move(val));
    }

    bool empty() {
        return vector_.empty();
    }

    auto clear() {
        return vector_.clear();
    }

    auto& back() {
        return vector_.back();
    }

    auto pop_back() {
        return vector_.pop_back();
    }

    template<typename... Args>
    auto erase(Args&&... args) {
        return vector_.erase(std::forward<Args>(args)...);
    }

    auto begin() {
        return vector_.begin();
    }

    auto end() {
        return vector_.end();
    }

    auto begin() const {
        return vector_.begin();
    }

    auto end() const {
        return vector_.end();
    }

    // Escape hatch for backwards compatibility:
    const std::vector<ValueType>& underlying_() const {
        return vector_;
    }

private:
    std::vector<ValueType> vector_;
};

template<typename StrongIndex, typename ValueType>
std::ostream& operator<<(std::ostream& stream, strong_vector<StrongIndex, ValueType>& vector) {
    return stream << vector.underlying_();
}

#endif //STRONG_INDICES_H
