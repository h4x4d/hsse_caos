#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "implementation/rec_dir_it.hpp"

namespace fs = std::filesystem;

using namespace stdlike;

class RecursiveDirectoryIteratorTest {
protected:
    RecursiveDirectoryIteratorTest() {
        fs::create_directory(test_dir);
        fs::create_directory(test_dir / "subdir1");
        fs::create_directory(test_dir / "subdir2");
        std::ofstream(test_dir / "file.txt");
        std::ofstream(test_dir / "subdir1/file1.txt");
        std::ofstream(test_dir / "subdir2/file2.txt");
    }

    ~RecursiveDirectoryIteratorTest() {
        fs::remove_all(test_dir);
    }

    fs::path test_dir = fs::temp_directory_path() / "test_dir";
};

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "EmptyDirectoryHandling") {
    fs::create_directory(test_dir / "empty_dir");
    std::vector<fs::path> actual_paths;

    for (const auto& entry :
         stdlike::recursive_directory_iterator((test_dir / "empty_dir").c_str())) {
        actual_paths.emplace_back(entry.path());
    }

    REQUIRE(actual_paths.empty());
}

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "PlainTraversal") {
    std::vector<fs::path> expectedPaths = {
        test_dir / "file.txt", test_dir / "subdir1",           test_dir / "subdir1/file1.txt",
        test_dir / "subdir2",  test_dir / "subdir2/file2.txt",
    };

    std::vector<fs::path> actual_paths;
    for (const auto& entry : stdlike::recursive_directory_iterator(test_dir.c_str())) {
        actual_paths.emplace_back(entry.path());
    }

    REQUIRE(actual_paths.size() == expectedPaths.size());
    for (const auto& expected : expectedPaths) {
        REQUIRE(std::ranges::find(actual_paths, expected) != actual_paths.end());
    }
}

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "AccessOptions") {
    fs::permissions(test_dir / "subdir1", fs::perms::none);

    auto dangerous_traversal = [this]() {
        for (const auto& entry : stdlike::recursive_directory_iterator(test_dir.c_str())) {
            std::ignore = entry;
        }
    };
    REQUIRE_THROWS(dangerous_traversal());

    std::vector<fs::path> possiblePaths = {
        test_dir / "file.txt",
        test_dir / "subdir2",
        test_dir / "subdir2/file2.txt",
    };
    std::vector<fs::path> actual_paths;

    auto safe_iter = stdlike::recursive_directory_iterator(
        test_dir.c_str(), stdlike::directory_options::skip_permission_denied);
    for (const auto& entry : safe_iter) {
        actual_paths.emplace_back(entry.path());
    }

    fs::permissions(test_dir / "subdir1", fs::perms::owner_all);

    for (const auto& path : possiblePaths) {
        REQUIRE(std::ranges::find(actual_paths, path) != actual_paths.end());
    }
}

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "SymLinksProcessing") {
    fs::path redirector_dir = fs::temp_directory_path() / "other_dir";
    fs::create_directory(redirector_dir);
    fs::create_symlink(test_dir, redirector_dir / "symlink_to_test_dir");
    fs::create_symlink(test_dir / "subdir1", test_dir / "symlink_to_subdir1");

    fs::path path_with_link = redirector_dir / "symlink_to_test_dir";
    std::vector<fs::path> control_paths = {
        path_with_link,
        path_with_link / "symlink_to_subdir1",
        path_with_link / "subdir1",
    };
    std::vector<fs::path> actual_paths;

    constexpr int expected_cnt_symlinks = 2;
    int cnt_symlinks = 0;

    for (const auto& entry : stdlike::recursive_directory_iterator(
             redirector_dir.c_str(), stdlike::directory_options::follow_directory_symlink)) {
        actual_paths.emplace_back(entry.path());
        cnt_symlinks += static_cast<int>(entry.is_symlink());
    }

    REQUIRE(cnt_symlinks == expected_cnt_symlinks);

    for (const auto& path : control_paths) {
        REQUIRE(std::ranges::find(actual_paths, path) != actual_paths.end());
    }

    fs::remove_all(redirector_dir);
}

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "CheckDepthAndPop") {
    fs::path deep_path = test_dir / "subdir1" / "subsubdir1";

    fs::create_directory(deep_path);

    std::ofstream(deep_path / "file.txt");

    std::vector<std::pair<fs::path, int>> control_depths = {
        {test_dir / "file.txt", 0},
        {test_dir / "subdir1/file1.txt", 1},
        {test_dir / "subdir2/file2.txt", 1},
        {deep_path / "file.txt", 2},
    };

    std::vector<std::pair<fs::path, int>> actual_paths;
    for (auto iter = stdlike::recursive_directory_iterator(test_dir.c_str());
         iter != stdlike::end(iter); ++iter) {
        actual_paths.emplace_back(iter->path(), iter.depth());
    }

    for (auto entry : control_depths) {
        REQUIRE(std::ranges::find(actual_paths, entry) != actual_paths.end());
    }

    for (auto it = stdlike::recursive_directory_iterator(test_dir.c_str()); it != stdlike::end(it);
         ++it) {
        if (it->path() == (deep_path / "file.txt")) {
            it.pop();
            REQUIRE(it->path() == test_dir / "subdir1" / "file1.txt");
            break;
        }
    }
}

TEST_CASE_METHOD(RecursiveDirectoryIteratorTest, "CheckStats") {
    constexpr int kFileSize = 64;
    std::ofstream fout{test_dir / "file_sz_small.txt"};
    fs::resize_file(test_dir / "file_sz_small.txt", kFileSize);

    constexpr int expected_directories_cnt = 2;
    constexpr int expected_regular_cnt = 4;

    int directories_cnt = 0;
    int regular_cnt = 0;
    for (auto it = stdlike::recursive_directory_iterator(test_dir.c_str()); it != stdlike::end(it);
         ++it) {
        if (it->is_directory()) {
            REQUIRE(it->hard_link_count() == 2);
            ++directories_cnt;
        } else {
            REQUIRE(it->hard_link_count() == 1);
            if (it->is_regular_file()) {
                ++regular_cnt;
            }
            if (it->path() == (test_dir / "file_sz_small.txt")) {
                REQUIRE(it->file_size() == kFileSize);
            }
        }
    }

    REQUIRE(directories_cnt == expected_directories_cnt);
    REQUIRE(regular_cnt == expected_regular_cnt);
}