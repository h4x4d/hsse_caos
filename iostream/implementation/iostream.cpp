#include "iostream.hpp"
#include <limits>
#include <iostream>

namespace stdlike {

bool utils::isspace(char symbol) {
    char spaces[6] = {' ', '\f', '\n', '\r', '\t', '\v'};

    for (char space : spaces) {
        if (space == symbol) {
            return true;
        }
    }

    return false;
}
template <typename T>
void utils::reverse(T* start, T* end) {
    for (int i = 0; i < (end - start) / 2; ++i) {
        T tmp = *(start + i);
        *(start + i) = *(end - i - 1);
        *(end - i - 1) = tmp;
    }
}

template <typename T>
T utils::abs(T value) {
    return value > 0 ? value : -value;
}

istream& istream::operator>>(bool& value) {
    cout_ptr->flush();
    int number;
    *this >> number;
    if (error_ != 0) {
        value = false;
    } else {
        value = number != 0;
        error_ = (number > 1) || (number < 0);
    }
    return *this;
}

istream& istream::operator>>(char& value) {
    get(value);
    return *this;
}

istream& istream::get(char& symbol) {
    skip_spaces();
    if (buffer_size_ == -1) {
        symbol = '\0';
        error_ = true;
        return *this;
    }
    cout_ptr->flush();
    symbol = peek();
    ++offset_;
    return *this;
}

int istream::peek() {
    get_new_buffer();
    if (buffer_size_ <= 0) {
        return '\0';
    }
    return buffer_[offset_];
}

void istream::get_new_buffer() {
    if (offset_ >= buffer_size_) {
        if (buffer_size_ > 0) {
            absolute_offset_ += static_cast<long long>(buffer_size_);
        }
        buffer_size_ = pread(0, buffer_, kMaxBufferSize, absolute_offset_);
        if (buffer_size_ <= 0) {
            error_ = 1;
        }
        offset_ = 0;
    }
}

void istream::skip_spaces() {
    while (!error_ && utils::isspace(peek())) {
        ++offset_;
    }
}

ostream& ostream::operator<<(char value) {
    if (offset_ >= kMaxBufferSize) {
        write_buffer();
    }
    buffer_[offset_++] = value;
    return *this;
}

ostream& ostream::operator<<(const void* value) {
    return *this << '0' << 'x' << reinterpret_cast<long long>(value);
}

ostream& ostream::operator<<(bool value) {
    return *this << static_cast<int>(value);
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
    error_ = false;
    write_buffer();
    return *this;
}

void ostream::write_buffer() {
    ssize_t written = 0;
    while (written != offset_) {
        ssize_t last_written = write(1, buffer_ + written, offset_ - written);
        if (last_written == -1) {
            error_ = 1;
            return;
        }
        written += last_written;
    }
    offset_ = 0;
}

bool ostream::fail() {
    return error_;
}

bool istream::fail() {
    return error_;
}

istream cin;
ostream cout;
}  // namespace stdlike
