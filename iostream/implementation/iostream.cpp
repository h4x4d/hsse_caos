#include "iostream.hpp"
#include <limits>

namespace stdlike {

bool isspace(char symbol) {
    char spaces[6] = {' ', '\f', '\n', '\r', '\t', '\v'};

    for (char space : spaces) {
        if (space == symbol) {
            return true;
        }
    }

    return false;
}

istream& istream::operator>>(int& value) {
    long long number;
    *this >> number;
    value = static_cast<int>(number);
    return *this;
}

istream& istream::operator>>(long long& value) {
    cout.flush();
    skip_spaces();
    long long minus = 1;
    if (peek() == '-') {
        ++offset_;
        minus = -1;
    }
    unsigned long long number;
    *this >> number;
    if (number > std::numeric_limits<long long>::max()) {
        value = minus > 0 ? std::numeric_limits<long long>::max()
                          : std::numeric_limits<long long>::min();
    } else {
        value = minus * static_cast<long long>(number);
    }
    return *this;
}

istream& istream::operator>>(unsigned int& value) {
    unsigned long long number;
    *this >> number;
    value = static_cast<unsigned int>(number);
    return *this;
}

istream& istream::operator>>(unsigned long long& value) {
    cout.flush();
    skip_spaces();
    value = 0;
    if (peek() == '-') {
        ++offset_;
    }
    while ('0' <= peek() && peek() <= '9') {
        value = value * 10 + peek() - '0';
        ++offset_;
    }
    return *this;
}

istream& istream::operator>>(bool& value) {
    cout.flush();
    int number = 0;
    *this >> number;
    value = number != 0;
    return *this;
}

istream& istream::operator>>(float& value) {
    double number;
    *this >> number;
    value = static_cast<float>(number);
    return *this;
}

istream& istream::operator>>(double& value) {
    cout.flush();
    skip_spaces();
    double minus = 1;
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

istream& istream::operator>>(char& value) {
    get(value);
    return *this;
}

istream& istream::get(char& symbol) {
    cout.flush();
    skip_spaces();
    symbol = buffer_[offset_++];
    return *this;
}

int istream::peek() {
    get_new_buffer();
    return buffer_[offset_];
}

void istream::get_new_buffer() {
    if (offset_ >= buffer_size_) {
        absolute_offset_ += static_cast<long long>(buffer_size_);
        buffer_size_ = pread(0, buffer_, max_buffer_size_, absolute_offset_);
        offset_ = 0;
    }
}

void istream::skip_spaces() {
    while (isspace(peek())) {
        ++offset_;
    }
}

ostream& ostream::operator<<(int value) {
    return *this << static_cast<long long>(value);
}

ostream& ostream::operator<<(long long value) {
    if (max_buffer_size_ - offset_ < 20) {
        write_buffer();
    }
    if (value < 0) {
        buffer_[offset_++] = '-';
        value *= -1;
    } else if (value == 0) {
        buffer_[offset_++] = '0';
    }

    size_t start_offset = offset_;
    while (value != 0) {
        buffer_[offset_++] = static_cast<char>('0' + value % 10);
        value /= 10;
    }
    for (size_t i = 0; i < (offset_ - start_offset) / 2; ++i) {
        char temp = buffer_[start_offset + i];
        buffer_[start_offset + i] = buffer_[offset_ - 1 - i];
        buffer_[offset_ - 1 - i] = temp;
    }
    return *this;
}

ostream& ostream::operator<<(double value) {
    *this << static_cast<long long>(value);
    *this << '.';
    value -= static_cast<double>(static_cast<long long>(value));
    unsigned index = 0;
    while (index++ < double_precision && value > 0) {
        value *= 10;
        int digit = static_cast<int>(value);
        *this << digit;
        value -= digit;
    }

    return *this;
}

ostream& ostream::operator<<(char value) {
    if (offset_ >= max_buffer_size_) {
        write_buffer();
    }
    buffer_[offset_++] = value;
    return *this;
}

ostream& ostream::operator<<(const void* value) {
    return *this << '0' << 'x' << reinterpret_cast<long long>(value);
}

ostream& ostream::operator<<(const char* value) {
    while (*value != '\0') {
        *this << *(value++);
    }

    return *this;
}

ostream& ostream::put(char symbol) {
    buffer_[offset_++] = symbol;
    return *this;
}

ostream& ostream::flush() {
    write_buffer();
    return *this;
}

void ostream::write_buffer() {
    size_t written = 0;
    while (written != offset_) {
        written += write(1, buffer_ + written, offset_ - written);
    }
    offset_ = 0;
}

istream cin;
ostream cout;
}  // namespace stdlike
