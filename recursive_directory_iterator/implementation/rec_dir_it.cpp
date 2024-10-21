#include "rec_dir_it.hpp"

namespace stdlike {
PermissionDeniedException::PermissionDeniedException(const char *filename)
    : std::exception(), error_text_(std::string("Permission to folder ") + filename + " denied") {
}

const char *PermissionDeniedException::what() const noexcept {
    return error_text_.data();
}

directory_entry::directory_entry(dirent *entry, const std::string &path) {
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

directory_entry &directory_entry::operator=(const directory_entry &other) {
    if (this != &other) {
        name_ = other.name_;
        stats_ = other.stats_;
        is_symlink_ = other.is_symlink_;
    }
    return *this;
}

const char *directory_entry::path() const {
    return name_.data();
}
bool directory_entry::is_symlink() const {
    return is_symlink_;
}
bool directory_entry::is_directory() const {
    return S_ISDIR(stats_.st_mode);
}
bool directory_entry::is_regular_file() const {
    return S_ISREG(stats_.st_mode);
}
bool directory_entry::is_character_file() const {
    return S_ISCHR(stats_.st_mode);
}
bool directory_entry::is_block_file() const {
    return S_ISBLK(stats_.st_mode);
}
bool directory_entry::is_socket() const {
    return S_ISSOCK(stats_.st_mode);
}
bool directory_entry::is_fifo() const {
    return S_ISFIFO(stats_.st_mode);
}
std::uintmax_t directory_entry::file_size() const {
    return stats_.st_size;
}
unsigned directory_entry::hard_link_count() const {
    return stats_.st_nlink;
}
time_t directory_entry::last_time_write() const {
    return stats_.st_mtime;
}
uint16_t directory_entry::permissions() const {
    return stats_.st_mode % 511;
}

directory_iterator::directory_iterator(const char *dir_name)
    : root_(dir_name), current_dir_(opendir(dir_name)) {
    if (current_dir_ == nullptr) {
        throw PermissionDeniedException(root_.data());
    }
    operator++();
}

directory_iterator::directory_iterator(const char *dir_name, directory_options options)
    : dir_options_(options), root_(dir_name), current_dir_(opendir(root_.data())) {
    if (current_dir_ == nullptr) {
        throw PermissionDeniedException(root_.data());
    }
    operator++();
}

directory_iterator::directory_iterator(const directory_iterator &other)
    : directory_iterator(other.root_.data(), other.dir_options_) {
}

directory_iterator::value_type &directory_iterator::operator*() {
    return current_file_;
}

directory_iterator &directory_iterator::operator++() {
    dirent *new_entry = readdir(current_dir_);
    while (new_entry != nullptr &&
           (strcmp(new_entry->d_name, ".") == 0 || strcmp(new_entry->d_name, "..") == 0)) {
        new_entry = readdir(current_dir_);
    }
    current_file_ = directory_entry{new_entry, root_};
    return *this;
}

directory_iterator::pointer directory_iterator::operator->() {
    return &current_file_;
}
recursive_directory_iterator::value_type &recursive_directory_iterator::operator*() {
    return *current_file_;
}
recursive_directory_iterator::pointer recursive_directory_iterator::operator->() {
    if (strlen(current_file_->path()) == 0) {
        return nullptr;
    }
    return current_file_;
}

recursive_directory_iterator recursive_directory_iterator::operator++() {
    if (current_file_ != nullptr && current_file_->is_directory() &&
        (!current_file_->is_symlink() ||
         dir_options_ == directory_options::follow_directory_symlink)) {
        try {
            rec_.emplace(current_file_->path());
        } catch (PermissionDeniedException &) {
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
}

size_t recursive_directory_iterator::depth() const {
    return rec_.size() - 1;
}

void recursive_directory_iterator::pop() {
    rec_.pop();
    if (!rec_.empty()) {
        current_file_ = rec_.top().operator->();
    } else {
        current_file_ = nullptr;
    }
}

bool recursive_directory_iterator::operator==(const recursive_directory_iterator &other) {
    return current_file_ == other.current_file_ && root_ == other.root_;
}
bool recursive_directory_iterator::operator!=(const recursive_directory_iterator &other) {
    return !(*this == other);
}

directory_iterator::~directory_iterator() {
    closedir(current_dir_);
    current_dir_ = nullptr;
}

recursive_directory_iterator::recursive_directory_iterator(const char *dir_name) : root_(dir_name) {
    rec_.emplace(dir_name, dir_options_);
    current_file_ = rec_.top().operator->();
    if (strlen(current_file_->path()) == 0) {
        pop();
    }
}

recursive_directory_iterator::recursive_directory_iterator(const char *dir_name,
                                                           directory_options options)
    : dir_options_(options), root_(dir_name) {
    rec_.emplace(dir_name);
    current_file_ = rec_.top().operator->();
    if (strlen(current_file_->path()) == 0) {
        pop();
    }
}

recursive_directory_iterator begin(recursive_directory_iterator iter) {
    return iter;
}
recursive_directory_iterator end(recursive_directory_iterator iter) {
    auto end_iter = std::move(iter);
    end_iter.current_file_ = nullptr;
    return end_iter;
}
}  // namespace stdlike
