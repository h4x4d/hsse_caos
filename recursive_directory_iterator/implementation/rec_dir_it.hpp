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
    PermissionDeniedException(const char* filename)
        : std::exception(),
          error_text_(std::string("Permission to folder ") + filename + " denied") {
    }

    const char* what() const noexcept {
        return error_text_.data();
    }

private:
    std::string error_text_;
};

class directory_entry {
public:
    directory_entry() = default;

    directory_entry(dirent* entry, const std::string& path) {
        if (entry != nullptr) {
            name_ = path + "/" + entry->d_name;
            int error = lstat(name_.data(), &stats_);
            if (error) {
                throw PermissionDeniedException(name_.data());
            }
            if (DT_LNK == entry->d_type) {
                is_symlink_ = true;
                stat(name_.data(), &stats_);
            }
        }
    }

    directory_entry& operator=(const directory_entry& other) {
        if (this != &other) {
            name_ = other.name_;
            stats_ = other.stats_;
            is_symlink_ = other.is_symlink_;
        }
        return *this;
    }

    [[nodiscard]] const char* path() const {
        return name_.data();
    };

    [[nodiscard]] bool is_symlink() const {
        return is_symlink_;
    };

    [[nodiscard]] bool is_directory() const {
        return S_ISDIR(stats_.st_mode);
    };

    [[nodiscard]] bool is_regular_file() const {
        return S_ISREG(stats_.st_mode);
    };

    [[nodiscard]] bool is_block_file() const {
        return S_ISBLK(stats_.st_mode);
    };

    [[nodiscard]] bool is_character_file() const {
        return S_ISCHR(stats_.st_mode);
    };

    [[nodiscard]] bool is_socket() const {
        return S_ISSOCK(stats_.st_mode);
    };

    [[nodiscard]] bool is_fifo() const {
        return S_ISFIFO(stats_.st_mode);
    };

    [[nodiscard]] std::uintmax_t file_size() const {
        return stats_.st_size;
    };

    [[nodiscard]] unsigned hard_link_count() const {
        return stats_.st_nlink;
    };

    [[nodiscard]] time_t last_time_write() const {
        return stats_.st_mtime;
    };

    [[nodiscard]] uint16_t permissions() const {
        return stats_.st_mode % 511;
    };

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

    explicit directory_iterator(const char* dir_name)
        : root_(dir_name), current_dir_(opendir(dir_name)) {
        if (current_dir_ == nullptr) {
            throw PermissionDeniedException(root_.data());
        }
        operator++();
    };

    directory_iterator(const char* dir_name, directory_options options)
        : dir_options_(options), root_(dir_name), current_dir_(opendir(root_.data())) {
        if (current_dir_ == nullptr) {
            throw PermissionDeniedException(root_.data());
        }
        operator++();
    };

    directory_iterator(const directory_iterator& other)
        : directory_iterator(other.root_.data(), other.dir_options_) {};

    reference operator*() {
        return current_file_;
    };

    pointer operator->() {
        return &current_file_;
    };

    directory_iterator& operator++() {
        dirent* new_entry = readdir(current_dir_);
        while (new_entry != nullptr &&
               (strcmp(new_entry->d_name, ".") == 0 || strcmp(new_entry->d_name, "..") == 0)) {
            new_entry = readdir(current_dir_);
        }
        current_file_ = directory_entry{new_entry, root_};
        return *this;
    };

    ~directory_iterator() {
        closedir(current_dir_);
        current_dir_ = nullptr;
    }

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

    explicit recursive_directory_iterator(const char* dir_name) : root_(dir_name) {
        rec_.emplace(dir_name, dir_options_);
        current_file_ = rec_.top().operator->();
        if (strlen(current_file_->path()) == 0) {
            pop();
        }
    };

    recursive_directory_iterator(const char* dir_name, directory_options options)
        : dir_options_(options), root_(dir_name) {
        rec_.emplace(dir_name);
        current_file_ = rec_.top().operator->();
        if (strlen(current_file_->path()) == 0) {
            pop();
        }
    };

    reference operator*() {
        return *current_file_;
    };

    pointer operator->() {
        if (strlen(current_file_->path()) == 0) {
            return nullptr;
        }
        return current_file_;
    };

    recursive_directory_iterator operator++() {
        if (current_file_ != nullptr && current_file_->is_directory() &&
            (!current_file_->is_symlink() ||
             dir_options_ == directory_options::follow_directory_symlink)) {
            try {
                rec_.emplace(current_file_->path());
            } catch (PermissionDeniedException&) {
                if (dir_options_ == directory_options::skip_permission_denied) {
                    ++rec_.top();
                    while (strlen(current_file_->path()) == 0) {
                        pop();
                        if (!rec_.empty()) {
                            ++rec_.top();
                        } else {
                            return *this;
                        }
                    }
                } else {
                    throw;
                }
            }
        } else {
            ++rec_.top();
            while (strlen(current_file_->path()) == 0) {
                pop();
                if (!rec_.empty()) {
                    ++rec_.top();
                } else {
                    return *this;
                }
            }
        }
        current_file_ = rec_.top().operator->();
        return *this;
    };

    [[nodiscard]] size_t depth() const {
        return rec_.size() - 1;
    };

    void pop() {
        rec_.pop();
        if (!rec_.empty()) {
            current_file_ = rec_.top().operator->();
        } else {
            current_file_ = nullptr;
        }
    };

    bool operator==(const recursive_directory_iterator& other) {
        return current_file_ == other.current_file_ && root_ == other.root_;
    }

    bool operator!=(const recursive_directory_iterator& other) {
        return !(*this == other);
    }

private:
    directory_options dir_options_ = directory_options::none;
    std::string root_ = "/";
    std::stack<directory_iterator> rec_{};
    directory_entry* current_file_;

    friend recursive_directory_iterator end(recursive_directory_iterator iter);
};

recursive_directory_iterator begin(recursive_directory_iterator iter) {
    return iter;
}

recursive_directory_iterator end(recursive_directory_iterator iter) {
    auto end_iter = std::move(iter);
    end_iter.current_file_ = nullptr;
    return end_iter;
}

}  // namespace stdlike