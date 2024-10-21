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
extern ostream cout;

bool isspace(char symbol);
}  // namespace stdlike