#pragma once

#include <unistd.h>

namespace stdlike {

class istream {
public:
    istream& operator>>(int& value);

    istream& operator>>(unsigned int& value);

    istream& operator>>(long long& value);

    istream& operator>>(unsigned long long& value);

    istream& operator>>(bool& value);

    istream& operator>>(float& value);

    istream& operator>>(double& value);

    istream& operator>>(char& value);

    istream& get(char& symbol);

    int peek();

private:
    void get_new_buffer();

    void skip_spaces();

    static constexpr size_t max_buffer_size_ = 128;
    char buffer_[max_buffer_size_];
    size_t buffer_size_ = 0;
    size_t offset_ = 0;
    long long absolute_offset_ = 0;
};

class ostream {
public:
    ~ostream() {
        write_buffer();
    }

    ostream& operator<<(int value);

    ostream& operator<<(long long value);

    ostream& operator<<(double value);

    ostream& operator<<(char value);

    ostream& operator<<(const void* value);

    ostream& operator<<(const char* value);

    ostream& put(char symbol);

    ostream& flush();

private:
    void write_buffer();

    static constexpr size_t max_buffer_size_ = 128;
    char buffer_[max_buffer_size_];
    size_t offset_ = 0;

    unsigned double_precision = 16;
};

extern istream cin;
extern ostream cout;

bool isspace(char symbol);
}  // namespace stdlike