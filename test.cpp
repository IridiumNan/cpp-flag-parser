// test_flag_parser.cpp
#include "FlagParser.hpp"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_eq(const std::string &actual, const std::string &expected,
              const std::string &name) {
    if (actual == expected) {
        std::cout << "[ OK ] " << name << "\n";
    } else {
        std::cerr << "[FAIL] " << name << "\n"
                  << "       expected: [" << expected << "]\n"
                  << "       actual:   [" << actual << "]\n";
        ++g_failures;
    }
}

void check_true(bool cond, const std::string &name) {
    if (cond) {
        std::cout << "[ OK ] " << name << "\n";
    } else {
        std::cerr << "[FAIL] " << name << "\n";
        ++g_failures;
    }
}

// At(0) 不应包含 argv[0] 程序名
void test_at_should_not_include_argv0() {
    char prog[] = "prog";
    char input[] = "input.txt";
    char flag[] = "--port";
    char value[] = "8080";
    char *argv[] = {prog, input, flag, value};

    fpr::FlagParser parser(4, argv);

    check_eq(parser.At(0), "input.txt", "At(0) 不应是程序名，应是 input.txt");
    check_eq(parser.At(1), fpr::invalid_arg, "At(1) 越界应返回 invalid_arg");
}

// --port 8080
void test_long_flag_space_value() {
    char prog[] = "prog";
    char flag[] = "--port";
    char value[] = "8080";
    char *argv[] = {prog, flag, value};

    fpr::FlagParser parser(3, argv);

    check_eq(parser.Flag("port", "p", "default"), "8080",
             "Flag(port,p) 解析 --port 8080");
    check_eq(parser.Flag("--port", "-p", "default"), "8080",
             "Flag(--port,-p) 解析 --port 8080（按注释）");
}

// -p 8080
void test_short_flag_space_value() {
    char prog[] = "prog";
    char flag[] = "-p";
    char value[] = "8080";
    char *argv[] = {prog, flag, value};

    fpr::FlagParser parser(3, argv);

    check_eq(parser.Flag("port", "p", "default"), "8080",
             "Flag(port,p) 解析 -p 8080");
    check_eq(parser.Flag("--port", "-p", "default"), "8080",
             "Flag(--port,-p) 解析 -p 8080（按注释）");
}

// --port=8080
void test_long_flag_equals_value() {
    char prog[] = "prog";
    char arg[] = "--port=8080";
    char *argv[] = {prog, arg};

    fpr::FlagParser parser(2, argv);

    check_eq(parser.Flag("port", "p", "default"), "8080",
             "Flag(port,p) 解析 --port=8080");
    check_eq(parser.Flag("--port", "-p", "default"), "8080",
             "Flag(--port,-p) 解析 --port=8080（按注释）");
}

// -p=8080
void test_short_flag_equals_value() {
    char prog[] = "prog";
    char arg[] = "-p=8080";
    char *argv[] = {prog, arg};

    fpr::FlagParser parser(2, argv);

    check_eq(parser.Flag("port", "p", "default"), "8080",
             "Flag(port,p) 解析 -p=8080");
    check_eq(parser.Flag("--port", "-p", "default"), "8080",
             "Flag(--port,-p) 解析 -p=8080（按注释）");
}

// 非 flag 参数顺序
void test_no_flag_args_order() {
    char prog[] = "prog";
    char f1[] = "file1.txt";
    char flag[] = "--port";
    char value[] = "8080";
    char f2[] = "file2.txt";
    char *argv[] = {prog, f1, flag, value, f2};

    fpr::FlagParser parser(5, argv);

    check_eq(parser.At(0), "file1.txt", "At(0) 非 flag 参数顺序");
    check_eq(parser.At(1), "file2.txt", "At(1) 非 flag 参数顺序");
    check_eq(parser.Flag("port", "p", "default"), "8080",
             "Flag(port,p) 与非 flag 参数混合");
}

// 负索引
void test_negative_index() {
    char prog[] = "prog";
    char f1[] = "file1.txt";
    char *argv[] = {prog, f1};

    fpr::FlagParser parser(2, argv);

    check_eq(parser.At(-1), fpr::invalid_arg, "At(-1) 应返回 invalid_arg");
}

// --port= 应抛异常
void test_invalid_equals_throws() {
    char prog[] = "prog";
    char arg[] = "--port=";
    char *argv[] = {prog, arg};

    bool thrown = false;
    try {
        fpr::FlagParser parser(2, argv);
    } catch (const std::invalid_argument &) {
        thrown = true;
    } catch (...) {
        // 其他异常也说明测试失败
    }

    check_true(thrown, "--port= 应抛出 std::invalid_argument");
}

void test_empty_arg() {
    char prog[] = "prog";
    char empty[] = "";
    char value[] = "8080";
    char *argv[] = {prog, empty, value};
    fpr::FlagParser parser(3, argv);
    // 期望：空字符串应被忽略或放入 no_flag_args，而不是作为 flag
    check_eq(parser.At(0), "8080", "空字符串应被忽略？");
    check_eq(parser.Flag("", "", "default"), "default", "空 flag 不应存在");
}

} // namespace

int main() {
    test_at_should_not_include_argv0();
    test_long_flag_space_value();
    test_short_flag_space_value();
    test_long_flag_equals_value();
    test_short_flag_equals_value();
    test_no_flag_args_order();
    test_negative_index();
    test_invalid_equals_throws();
    test_empty_arg();

    if (g_failures == 0) {
        std::cout << "\nAll tests passed.\n";
        return 0;
    }

    std::cerr << "\n" << g_failures << " test(s) failed.\n";
    return 1;
}
