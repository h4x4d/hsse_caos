#include <errno.h>  // for debug
#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>  // for debug
#include <regex>
#include <string_view>
// #include <numbers>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "implementation/iostream.hpp"

// NOLINTBEGIN
using namespace stdlike;

TEST_CASE("Cheater") {
    REQUIRE(!std::is_base_of_v<decltype(std::cin), decltype(cin)>);
    REQUIRE(!std::is_base_of_v<decltype(std::cout), decltype(cout)>);
}

class InputTest {
public:
    InputTest() : stdin_d(dup(STDIN_FILENO)) {
        int fd = open(kInputFilePath.data(), O_RDONLY);
        if (dup2(fd, STDIN_FILENO) == -1) {
            close(fd);
            throw std::runtime_error("couldn't open input-source file");
        }
        close(fd);
    }

    ~InputTest() {
        dup2(stdin_d, STDIN_FILENO);
        close(stdin_d);
    }

private:
    static constexpr std::string_view kInputFilePath = "../iostream/io_files/input.txt";
    int stdin_d{-1};
};

TEST_CASE_METHOD(InputTest, "FundamentalTypes") {
    char ch_num, ch_classic;
    cin >> ch_num >> ch_classic;
    REQUIRE(ch_num == '1');
    REQUIRE(ch_classic == 'c');

    int num;
    long long big_num;
    double floating_num;

    cin >> num >> big_num >> floating_num;
    REQUIRE(num == 32);
    REQUIRE(big_num == 8589934588);
    REQUIRE(floating_num == 3.1415926535);

    cin >> num >> big_num >> floating_num;
    REQUIRE(num == -32);
    REQUIRE(big_num == -8589934588);
    REQUIRE(floating_num == -3.1415926535);

    bool boolean(false);
    cin >> boolean;
    REQUIRE(boolean);
    cin >> boolean;
    REQUIRE_FALSE(boolean);
}

TEST_CASE_METHOD(InputTest, "BoundaryValues") {
    {
        int positive_num, negative_num;

        cin >> positive_num >> negative_num;
        REQUIRE(positive_num == std::numeric_limits<int>::max());
        REQUIRE(negative_num == std::numeric_limits<int>::min());
    }

    {
        long long positive_num, negative_num;

        cin >> positive_num >> negative_num;
        REQUIRE(positive_num == std::numeric_limits<long long>::max());
        REQUIRE(negative_num == std::numeric_limits<long long>::min());
    }
}

TEST_CASE_METHOD(InputTest, "LeadingZeroes") {
    int num;

    cin >> num;
    REQUIRE(num == 101);

    cin >> num;
    REQUIRE(num == -101);
}

TEST_CASE_METHOD(InputTest, "Unsigned") {
    unsigned int num;
    cin >> num;
    REQUIRE(num == std::numeric_limits<unsigned int>::max());

    cin >> num;
    REQUIRE(num == 2147483648);
}

TEST_CASE_METHOD(InputTest, "FloatingComplexInput") {
    double a, b;

    cin >> a >> b;

    REQUIRE(a == 10.0);
    REQUIRE(b == 0.01);
}

TEST_CASE_METHOD(InputTest, "InputVectorWithWrongInput") {
    std::vector<int> v(8);
    for (auto &elem : v) {
        cin >> elem;
    }

    std::vector<int> expected_v = {1, 0, -1, 0, 0, 0, 0, 0};

    REQUIRE(v == expected_v);
}

TEST_CASE_METHOD(InputTest, "ReadCharIntoInt") {
    int a;
    REQUIRE_NOTHROW(cin >> a);

    REQUIRE(a == 0);
    REQUIRE(cin.fail());
}

TEST_CASE_METHOD(InputTest, "NoReadPermissions") {
    int real_stdin_fd = dup(STDIN_FILENO);
    close(STDIN_FILENO);

    auto ReadMillionTimes = []() {
        char a;
        for (int i = 0; i < 1000000; ++i) {
            cin >> a;
        }
    };

    REQUIRE_NOTHROW(ReadMillionTimes());
    REQUIRE(cin.fail());

    dup2(real_stdin_fd, STDIN_FILENO);
    close(real_stdin_fd);
}

class OutputTest {
public:
    OutputTest() : stdout_d(dup(STDOUT_FILENO)), stdin_d(dup(STDIN_FILENO)), buf(32, '\0') {
        int fd = open(kOutputFilePath.data(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (dup2(fd, STDOUT_FILENO) == -1) {
            close(fd);
            throw std::runtime_error("couldn't open file for output");
        }
        fin.open(kOutputFilePath.data());
        close(fd);

        fd = open(kInputFilePath.data(), O_RDONLY);
        if (dup2(fd, STDIN_FILENO) == -1) {
            close(fd);
            throw std::runtime_error("couldn't open input-source file");
        }
        close(fd);
    }

    char *ExportOutput() {
        fin.sync();
        fin.getline(buf.data(), buf.size(), '\n');
        return buf.data();
    }

    void RestoreIfstream() {
        int pos = fin.tellg();
        if (pos == -1) {
            if (fin.is_open()) {
                fin.close();
            }
            fin.open(kOutputFilePath.data());
        }
    }

    void ProhibitWritingToSTDOUT() {
        stdout_with_write_permissions_d = dup(STDOUT_FILENO);

        // open file with only read permissions
        int fd = open(kOutputFilePath.data(), O_RDONLY, S_IRUSR);

        if (dup2(fd, STDOUT_FILENO) == -1) {
            close(fd);
            throw std::runtime_error("couldn't open file for output");
        }
        fin.open(kOutputFilePath.data());
        close(fd);
    }

    void AllowWritingToSTDOUT() {
        dup2(stdout_with_write_permissions_d, STDOUT_FILENO);
        close(stdout_with_write_permissions_d);
    }

    ~OutputTest() {
        dup2(stdout_d, STDOUT_FILENO);
        close(stdout_d);
        fin.close();

        dup2(stdin_d, STDIN_FILENO);
        close(stdin_d);
    }

private:
    static constexpr std::string_view kOutputFilePath = "../iostream/io_files/output.txt";
    int stdout_d{-1};
    int stdout_with_write_permissions_d{-1};  // uses only in
                                              // ProhibitWritingToSTDOUT and
                                              // AllowWritingToSTDOUT methods

    static constexpr std::string_view kInputFilePath = "../iostream/io_files/input.txt";
    int stdin_d{-1};

    std::ifstream fin;
    std::string buf;
};

TEST_CASE_METHOD(OutputTest, "FundamentalTypes") {
    constexpr char ch_num('1'), ch_classic('c');
    cout << ch_num << ' ' << ch_classic << '\n';
    cout.flush();
    REQUIRE(strcmp("1 c", ExportOutput()) == 0);

    constexpr int num(32);
    constexpr long long big_num(8589934588LL);
    cout << num << ' ' << big_num << '\n';
    cout.flush();
    REQUIRE(strcmp("32 8589934588", ExportOutput()) == 0);

    constexpr double floating_num(3.14159);
    constexpr double floating_expected(3.14159);
    cout << floating_num << '\n';
    cout.flush();
    REQUIRE_THAT(std::stod(ExportOutput()), Catch::Matchers::WithinRel(floating_expected, 0.0001));

    cout << false << ' ' << true << '\n';
    cout.flush();
    REQUIRE(strcmp("0 1", ExportOutput()) == 0);
}

TEST_CASE_METHOD(OutputTest, "BorderValues") {
  constexpr int max_int(std::numeric_limits<int>::max());
  constexpr int min_int(std::numeric_limits<int>::min());
  cout << max_int << ' ' << min_int << '\n';
  cout.flush();
  REQUIRE(strcmp((std::to_string(max_int) + ' ' + std::to_string(min_int)).c_str(), ExportOutput()) == 0);
}

TEST_CASE_METHOD(OutputTest, "Pointers") {
    constexpr std::string_view str = "Should be string!";
    cout << str.data() << '\n';
    cout.flush();
    REQUIRE_THAT(ExportOutput(), Catch::Matchers::Matches(str.data()));

    void *ptr = &cout;
    cout << ptr << '\n';
    cout.flush();
    REQUIRE_THAT(ExportOutput(), Catch::Matchers::Matches("0x[0-9a-f]+"));
}

TEST_CASE_METHOD(OutputTest, "FlushingOutputBeforeInput") {
    cout.flush();
    static constexpr std::string_view output_before_input = "Some Output!";
    cout << output_before_input.data() << '\n';
    REQUIRE_THAT(ExportOutput(), Catch::Matchers::Matches("\0"));

    bool trigger;
    cin >> trigger;
    RestoreIfstream();
    REQUIRE_THAT(ExportOutput(), Catch::Matchers::Matches(output_before_input.data()));
}

TEST_CASE_METHOD(OutputTest, "NoWritePermissions") {
    ProhibitWritingToSTDOUT();

    cout.flush();

    auto CoutWithFlush = []() {
        cout << "Some string!";
        cout.flush();
    };

    REQUIRE_NOTHROW(CoutWithFlush());
    REQUIRE(cout.fail());

    AllowWritingToSTDOUT();
    cout.flush();  // clear buffer
}
// NOLINTEND
