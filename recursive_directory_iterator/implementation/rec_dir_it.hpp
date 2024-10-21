#pragma once

#include <iostream>
#include <iterator>
#include <cstdint>
#include <stack>
#include <dirent.h>
#include <cstring>
#include <utility>
#include <sys/stat.h>

namespace stdlike {

enum class directory_options { none = 0, follow_directory_symlink = 1, skip_permission_denied = 2 };

class PermissionDeniedException : public std::exception {
public:
    PermissionDeniedException(const char* filename);

    const char* what() const noexcept;

private:
    std::string error_text_;
};

class directory_entry {
public:
    directory_entry() = default;

    directory_entry(dirent* entry, const std::string& path);

    directory_entry& operator=(const directory_entry& other);

    [[nodiscard]] const char* path() const;

    [[nodiscard]] bool is_symlink() const;
    [[nodiscard]] bool is_directory() const;
    [[nodiscard]] bool is_regular_file() const;
    [[nodiscard]] bool is_block_file() const;
    [[nodiscard]] bool is_character_file() const;
    [[nodiscard]] bool is_socket() const;
    [[nodiscard]] bool is_fifo() const;

    [[nodiscard]] std::uintmax_t file_size() const;
    [[nodiscard]] unsigned hard_link_count() const;
    [[nodiscard]] uint16_t permissions() const;

    [[nodiscard]] time_t last_time_write() const;

private:
    std::string name_{};
    struct stat stats_;
    bool is_symlink_ = false;
};

class directory_iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = directory_entry;
    using pointer = value_type*;
    using reference = value_type&;

    explicit directory_iterator(const char* dir_name);
    directory_iterator(const char* dir_name, directory_options options);
    directory_iterator(const directory_iterator& other);

    reference operator*();
    pointer operator->();

    directory_iterator& operator++();

    ~directory_iterator();

private:
    directory_options dir_options_ = directory_options::none;
    std::string root_ = "/";
    DIR* current_dir_ = nullptr;
    directory_entry current_file_;
};

class recursive_directory_iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = directory_entry;
    using pointer = value_type*;
    using reference = value_type&;

    explicit recursive_directory_iterator(const char* dir_name);
    recursive_directory_iterator(const char* dir_name, directory_options options);

    reference operator*();
    pointer operator->();

    recursive_directory_iterator operator++();

    [[nodiscard]] size_t depth() const;

    void pop();

    bool operator==(const recursive_directory_iterator& other);
    bool operator!=(const recursive_directory_iterator& other);

private:
    directory_options dir_options_ = directory_options::none;
    std::string root_ = "/";
    std::stack<directory_iterator> rec_{};
    directory_entry* current_file_;

    friend recursive_directory_iterator end(recursive_directory_iterator iter);
};

recursive_directory_iterator begin(recursive_directory_iterator iter);
recursive_directory_iterator end(recursive_directory_iterator iter);

}  // namespace stdlike