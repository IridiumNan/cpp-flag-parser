// test_flag_parser.cpp
#include "FlagParser.hpp"

#include <cstring>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

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

// ---------------------------------------------------------------------------
// enable 状态
//
// 用例分两类：
//   [契约] 按 FlagParser.hpp 的注释与 API 名字应当成立的行为；
//   [现状] characterization，锁定当前实现，避免改动时无意改变行为。
//          它们记录的是已知的取舍与限制，不一定是期望结果。
// ---------------------------------------------------------------------------

// 用 initializer_list 构造可写 argv，省去每个用例手写 char 数组。
// argv[0] 自动补一个占位程序名，FlagParser 会跳过它。
class ArgvBuilder {
  public:
    explicit ArgvBuilder(std::initializer_list<const char *> items) {
        add("prog");
        for (const char *item : items) {
            add(item);
        }
    }

    int argc() const { return static_cast<int>(ptrs_.size()); }
    char **argv() { return ptrs_.data(); }

  private:
    void add(const char *text) {
        storage_.emplace_back(text, text + std::strlen(text) + 1);
        ptrs_.push_back(storage_.back().data());
    }

    std::vector<std::vector<char>> storage_;
    std::vector<char *> ptrs_;
};

// [契约] 头文件示例：连续的 flag 全部 enable，其后的带值选项仍正常取值
void test_enable_consecutive_flags() {
    ArgvBuilder args{"--verbose", "--debug", "--output", "file.txt"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] 连续 flag：--verbose 被 enable");
    check_eq(parser.Flag("debug", "", "<unset>"), fpr::enable_value,
             "[契约] 连续 flag：--debug 被 enable");
    check_eq(parser.Flag("output", "o", "<unset>"), "file.txt",
             "[契约] 连续 flag：--output 仍取到值");
    check_true(parser.IsEnableFlag("verbose", ""),
               "[契约] 连续 flag：IsEnableFlag(verbose) 为真");
}

// [契约] 单独一个 flag 位于末尾时也应 enable
void test_enable_single_last_flag() {
    ArgvBuilder args{"--verbose"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] 单独且位于末尾的 --verbose 应被 enable");
    check_true(parser.IsEnableFlag("verbose", ""),
               "[契约] 末尾 flag：IsEnableFlag(verbose) 为真");
}

// [契约] 末尾的 flag 出现在位置参数之后
void test_enable_last_flag_after_positional() {
    ArgvBuilder args{"photo.jpg", "--verbose"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.At(0), "photo.jpg", "[契约] 末尾 flag：位置参数不受影响");
    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] 位于末尾的 --verbose（在位置参数之后）应被 enable");
}

// [契约] 连续两个 flag，最后一个位于末尾
void test_enable_last_of_consecutive_flags() {
    ArgvBuilder args{"--verbose", "--debug"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] 连续两个 flag：第一个被 enable");
    check_eq(parser.Flag("debug", "", "<unset>"), fpr::enable_value,
             "[契约] 连续两个 flag：位于末尾的第二个同样应被 enable");
}

// [契约] 显式写法与裸 flag 等价（向后兼容）
void test_enable_explicit_true_equivalent() {
    ArgvBuilder args{"--verbose=true"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] --verbose=true 与 --verbose 等价");
    check_true(parser.IsEnableFlag("verbose", ""),
               "[契约] 显式 true：IsEnableFlag(verbose) 为真");
}

// [契约] IsEnableFlag 与 Flag(name)==enable_value 等价（见头文件注释）
void test_isenable_agrees_with_value() {
    ArgvBuilder enabled{"--verbose"};
    fpr::FlagParser a(enabled.argc(), enabled.argv());
    check_true(a.IsEnableFlag("verbose", "") ==
                   (a.Flag("verbose", "", "") == fpr::enable_value),
               "[契约] --verbose 时 IsEnableFlag 与 Flag()==true 一致");

    ArgvBuilder absent{"--columns", "80"};
    fpr::FlagParser b(absent.argc(), absent.argv());
    check_true(b.IsEnableFlag("verbose", "") ==
                   (b.Flag("verbose", "", "") == fpr::enable_value),
               "[契约] 未传入时两者一致（都应为 false）");
}

// [契约] 显式关闭不应被报成 enable
void test_isenable_respects_explicit_false() {
    ArgvBuilder args{"--verbose=false"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), "false",
             "[契约] --verbose=false 的值为 false");
    check_true(!parser.IsEnableFlag("verbose", ""),
               "[契约] --verbose=false 不应被 IsEnableFlag 报成 enable");
}

// [契约] 带值选项不应被 IsEnableFlag 报成 enable
void test_isenable_for_value_option() {
    ArgvBuilder args{"--columns", "80"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_true(!parser.IsEnableFlag("columns", ""),
               "[契约] 带值的 --columns 不应被 IsEnableFlag 报成 enable");
}

// [契约] '--' 作为选项结束标记，不应污染其他键的查询
void test_double_dash_does_not_poison_lookup() {
    ArgvBuilder args{"--verbose", "--", "foo.txt"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.At(0), "foo.txt", "[契约] -- 之后的 token 应作为位置参数");
    check_eq(parser.Flag("font", "", "<unset>"), "<unset>",
             "[契约] -- 之后的 token 不应成为 --font 的值");
    check_eq(parser.Flag("columns", "", "<unset>"), "<unset>",
             "[契约] -- 之后的 token 不应成为 --columns 的值");
    check_true(!parser.IsEnableFlag("debug", ""),
               "[契约] 未传入的 --debug 不应因 -- 而报 enable");
}

// [现状] 值型选项后面紧跟另一个 flag 时，会被当成 enable
// 原因：parser 不知道哪个选项该取值，只能靠“下一个 token 像不像 flag”猜测。
void test_characterization_value_option_followed_by_flag() {
    ArgvBuilder args{"--output", "--font", "a.ttf"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("output", "o", "<unset>"), fpr::enable_value,
             "[现状] --output 后紧跟 --font 时被当作 enable，值变成 \"true\"");
    check_eq(parser.Flag("font", "", "<unset>"), "a.ttf",
             "[现状] --font 正常取到值");
}

// [现状] 前一个 flag 会吃掉紧随其后的位置参数
void test_characterization_flag_swallows_positional() {
    ArgvBuilder args{"--verbose", "photo.jpg"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("verbose", "", "<unset>"), "photo.jpg",
             "[现状] 前一个 flag 会吃掉紧随其后的位置参数");
    check_true(parser.size() == 0u, "[现状] 位置参数列表为空");
}

// [契约] 键值对跟在无值 flag 之后时，两者都要落到各自的位置。
// 回归：trySingleFlagValuePair 成功时已经自己存表，调用方必须 continue，
// 否则 pending flag 会被写成 "--host=example.com" 这一串原文，
// 于是 --verbose 既不再是 enable、也拿不到正确的值。
void test_pair_after_enable_flag() {
    ArgvBuilder args{"--verbose", "--host=example.com"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("host", "h", "<unset>"), "example.com",
             "[契约] 键值对紧跟无值 flag：--host 取到值");
    check_eq(parser.Flag("verbose", "", "<unset>"), fpr::enable_value,
             "[契约] 键值对紧跟无值 flag：--verbose 仍为 enable");
    check_true(parser.IsEnableFlag("verbose", ""),
               "[契约] 键值对紧跟无值 flag：IsEnableFlag(verbose) 为真");
}

// [契约] 同上，但前面有两个连续的无值 flag
void test_pair_after_two_enable_flags() {
    ArgvBuilder args{"--verbose", "--debug", "--columns=80"};
    fpr::FlagParser parser(args.argc(), args.argv());

    check_eq(parser.Flag("columns", "", "<unset>"), "80",
             "[契约] 键值对跟在两个无值 flag 后：--columns 取到值");
    check_true(parser.IsEnableFlag("verbose", ""), "[契约] …--verbose 为 enable");
    check_eq(parser.Flag("debug", "", "<unset>"), fpr::enable_value,
             "[契约] …--debug 的值是 enable_value，而非 --columns=80");
    check_true(parser.IsEnableFlag("debug", ""), "[契约] …--debug 为 enable");
}

// [契约] 空 flag 名不得进入查找表，否则会污染所有查询
void test_empty_flag_name_not_stored() {
    ArgvBuilder dash{"-", "photo.jpg"};
    fpr::FlagParser p1(dash.argc(), dash.argv());
    check_eq(p1.Flag("font", "", "<unset>"), "<unset>",
             "[契约] '-' 不应写入空 flag 名");
    check_true(!p1.IsEnableFlag("debug", ""),
               "[契约] '-' 不应让未传入的 --debug 报 enable");

    ArgvBuilder long_dash{"--=x"};
    fpr::FlagParser p2(long_dash.argc(), long_dash.argv());
    check_eq(p2.Flag("font", "", "<unset>"), "<unset>",
             "[契约] '--=x' 不应写入空 flag 名");

    ArgvBuilder short_dash{"-=x"};
    fpr::FlagParser p3(short_dash.argc(), short_dash.argv());
    check_eq(p3.Flag("font", "", "<unset>"), "<unset>",
             "[契约] '-=x' 不应写入空 flag 名");
}

// [现状] '-' 目前被当成 end-of-options 标记（与 '--' 同义），且该 token
// 自身不会进入位置参数。POSIX 里 '-' 是普通操作数，本条锁定的是当前设计，
// 若改成“仅 '--' 结束选项”应改写此用例。
void test_characterization_single_dash_ends_options() {
    ArgvBuilder out{"--output", "-"};
    fpr::FlagParser p1(out.argc(), out.argv());
    check_eq(p1.Flag("output", "o", "<unset>"), fpr::enable_value,
             "[现状] --output - ：'-' 未成为 output 的值，output 反被 enable");

    ArgvBuilder mid{"photo.jpg", "-", "photo2.jpg"};
    fpr::FlagParser p2(mid.argc(), mid.argv());
    check_true(p2.size() == 2u, "[现状] 中间的 '-' 被丢弃，不进入位置参数");
    check_eq(p2.At(1), "photo2.jpg", "[现状] '-' 之后的 token 作为位置参数");
}

// [现状] 以 '-' 开头且不带 '=' 的值无法用空格形式传入，需写成 --name=value。
// 根源是没有选项表，parser 分不出“以 - 开头的值”和“新的 flag”。
void test_characterization_dash_value_needs_equals() {
    ArgvBuilder spaced{"--font", "-myfont.ttf"};
    fpr::FlagParser p1(spaced.argc(), spaced.argv());
    check_eq(p1.Flag("font", "", "<unset>"), fpr::enable_value,
             "[现状] --font -myfont.ttf ：值被当成 flag，--font 变成 enable");

    ArgvBuilder joined{"--font=-myfont.ttf"};
    fpr::FlagParser p2(joined.argc(), joined.argv());
    check_eq(p2.Flag("font", "", "<unset>"), "-myfont.ttf",
             "[现状] 用 --font=-myfont.ttf 才能传入以 '-' 开头的值");
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

    // enable 状态
    test_enable_consecutive_flags();
    test_enable_single_last_flag();
    test_enable_last_flag_after_positional();
    test_enable_last_of_consecutive_flags();
    test_enable_explicit_true_equivalent();
    test_isenable_agrees_with_value();
    test_isenable_respects_explicit_false();
    test_isenable_for_value_option();
    test_double_dash_does_not_poison_lookup();
    test_characterization_value_option_followed_by_flag();
    test_characterization_flag_swallows_positional();
    test_pair_after_enable_flag();
    test_pair_after_two_enable_flags();
    test_empty_flag_name_not_stored();
    test_characterization_single_dash_ends_options();
    test_characterization_dash_value_needs_equals();

    if (g_failures == 0) {
        std::cout << "\nAll tests passed.\n";
        return 0;
    }

    std::cerr << "\n" << g_failures << " test(s) failed.\n";
    return 1;
}
