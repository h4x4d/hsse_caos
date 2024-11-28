#pragma once

#include <unistd.h>
#include <type_traits>

namespace concepts {
template <typename T>
concept IsIntegrallyProcessed = requires(T value) {
    T(0);
    value % value;
    value* T(1);
    value - T(1);
    value + T(1);
};

template <typename T>
concept IsFloatingProcessed = requires(T value) {
    T(0);
    T(3.14);
    value* T(1);
    value - T(1);
    value + T(1);
};

namespace detail {
template <typename T, bool = std::is_arithmetic<T>::value>
struct IsSignedType : std::integral_constant<bool, T(-1) < T(0)> {};

template <typename T>
struct IsSignedType<T, false> : std::false_type {};
}  // namespace detail

template <typename T>
concept IsSigned = requires(T value) { detail::IsSignedType<T>::value; };
}  // namespace concepts

namespace stdlike {

namespace utils {
bool isspace(char symbol);

template <typename T>
T abs(T value);

template <typename T>
void reverse(T* start, T* end);
}  // namespace utils

class ostream;
extern ostream cout;

class istream {
public:
    template <concepts::IsIntegrallyProcessed T>
    istream& operator>>(T& value);

    istream& operator>>(bool& value);

    template <concepts::IsFloatingProcessed T>
    istream& operator>>(T& value)
        requires(!concepts::IsIntegrallyProcessed<T>);

    istream& operator>>(char& value);

    istream& get(char& symbol);

    int peek();

    bool fail();

private:
    void get_new_buffer();

    void skip_spaces();

    static constexpr size_t kMaxBufferSize = 128;
    char buffer_[kMaxBufferSize];
    ssize_t buffer_size_ = 0;
    ssize_t offset_ = 0;
    long long absolute_offset_ = 0;
    bool error_{false};
    ostream* cout_ptr = &cout;
};

class ostream {
public:
    ~ostream() {
        write_buffer();
    }

    template <concepts::IsIntegrallyProcessed T>
    ostream& operator<<(T value);

    ostream& operator<<(bool value);

    template <concepts::IsFloatingProcessed T>
    ostream& operator<<(T value)
        requires(!concepts::IsIntegrallyProcessed<T>);

    ostream& operator<<(char value);

    ostream& operator<<(const void* value);

    ostream& operator<<(const char* value);

    ostream& put(char symbol);

    ostream& flush();

    bool fail();

private:
    void write_buffer();

    static constexpr ssize_t kMaxBufferSize = 128;
    char buffer_[kMaxBufferSize];
    ssize_t offset_ = 0;

    static const unsigned kDoublePrecision = 16;
    bool error_{false};
};

extern istream cin;

template <concepts::IsIntegrallyProcessed T>
istream& istream::operator>>(T& value) {
    cout_ptr->flush();
    skip_spaces();
    error_ = true;
    value = 0;

    short minus = 1;
    if (peek() == '-') {
        ++offset_;
        if constexpr (concepts::IsSigned<T>) {
            minus = -1;
        }
    }

    while ('0' <= peek() && peek() <= '9') {
        value = value * 10 + minus * (peek() - '0');
        ++offset_;
        error_ = false;
    }

    return *this;
}

template <concepts::IsFloatingProcessed T>
istream& istream::operator>>(T& value)
    requires(!concepts::IsIntegrallyProcessed<T>)
{
    cout_ptr->flush();
    skip_spaces();
    T minus = 1;
    if (peek() == '-') {
        ++offset_;
        minus = -1;
    }
    value = 0;
    while ('0' <= peek() && peek() <= '9') {
        value = value * 10 + peek() - '0';
        ++offset_;
    }
    if (peek() == '.') {
        ++offset_;
        double index = 10;

        while ('0' <= peek() && peek() <= '9') {
            value += static_cast<double>(peek() - '0') / (index);
            index *= 10;
            ++offset_;
        }
    }
    value *= minus;
    return *this;
}

template <concepts::IsIntegrallyProcessed T>
ostream& ostream::operator<<(T value) {
    char* buffer = new char[30];
    size_t offset = 0;

    if (value < 0) {
        buffer[offset++] = '-';
    } else if (value == 0) {
        buffer[offset++] = '0';
    }

    size_t start_offset = offset;
    while (value != 0) {
        buffer[offset++] = static_cast<char>('0' + utils::abs(value % 10));
        value /= 10;
    }
    utils::reverse(buffer + start_offset, buffer + offset);
    *(buffer + offset) = '\0';
    operator<<(buffer);
    delete[] buffer;
    return *this;
}

template <concepts::IsFloatingProcessed T>
ostream& ostream::operator<<(T value)
    requires(!concepts::IsIntegrallyProcessed<T>)
{
    *this << static_cast<long long>(value);
    *this << '.';
    value -= static_cast<double>(static_cast<long long>(value));
    unsigned index = 0;
    while (index++ < kDoublePrecision && value > 0) {
        value *= 10;
        int digit = static_cast<int>(value);
        *this << digit;
        value -= digit;
    }

    return *this;
}
}  // namespace stdlike