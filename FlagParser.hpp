#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fpr {

using std::string;

using flag_type = string;
using arg_type = string;

const string invalid_arg = "";
const string enable_value = "true";

// not_found_idx is the max limit of size_t
// The [string.find](char) will return it if not found
constexpr size_t not_found_idx = (size_t(0) - 1);

class FlagParser {

    // no_flag_args store all args without leading flags
    // Visit it by index
    std::vector<arg_type> no_flag_args;

    // flag_args store all flag and its value
    std::unordered_map<flag_type, arg_type> flag_pairs;

    // isFlagStr check if this arg is a flag with leading char '-'
    bool isFlagStr(const string arg) { return arg[0] == '-'; }

    // pushNewFlagValuePair will stript the leading '-' on the flag for parsing
    // then store safe
    void pushNewFlagValuePair(const flag_type &raw_flag,
                              const arg_type &value) {

        string flag = stripFlagNameLeading(raw_flag);

        if (flag.empty())
            return;
        flag_pairs[flag] = value;
    }

    // trySingleFlagValuePair try to parse this arg as flag=value then push it
    // You should ensure this arg is leading with '-' to make sure it's a flag
    // or flag-value pair
    // If it can't be parsed as flag value pair with '=' concate
    // return false
    //
    // If parse success, it will push it into map then return true
    bool trySingleFlagValuePair(string pair) {
        // empty args
        if (pair.empty())
            return false;

        size_t equal_pos = pair.find('=');

        // This means '=' not found on this string
        if (equal_pos == not_found_idx) {
            return false;
        }

        // encounter the pair like "--port=", throw error
        if (equal_pos == pair.size() - 1) {
            throw std::invalid_argument(
                "Invalid args, format flag with --key=value or -short=value");
        }

        string flag = pair.substr(0, equal_pos);
        string value = pair.substr(equal_pos + 1, pair.size());
        pushNewFlagValuePair(flag, value);
        return true;
    }

    // enableFlag set the value of this flag as [enable_value]
    void enableFlag(const flag_type &name) {
        pushNewFlagValuePair(name, enable_value);
    }

    // stripFlagNameLeading, cut the leading '-' on the front of raw flag
    arg_type stripFlagNameLeading(const arg_type &name) {
        size_t start = 0;
        while (name[start] == '-') {
            start++;
        }

        return name.substr(start, name.size());
    }

  public:
    // FlagParser initilize with args from main function
    // it can parse flags like
    // --port 8080
    // --port=8080
    // -p 8080
    // -p=8080
    //
    //
    // for flag without any value, it set this flag as enable
    // check by Flag(name...) == [enable_value] or IsEnableFlag(name...)
    //
    // Other no flag paired args can be visit by [At](index) function
    //
    // NOTE:
    // If args[i] is empty str, it will be omit
    FlagParser(int argv, char **args) {

        arg_type curr_arg;
        // expecte_value is the flag mark if next argv should be treated as
        // value of flag if false, try if next arg is flag str, else push it
        // into no_flag_args
        bool expecte_value = false;

        // store current flag
        flag_type curr_flag;

        bool flag_end = false;

        for (int i = 1; i < argv; i++) {

            curr_arg = args[i];
            if (curr_arg.empty())
                continue;

            if (stripFlagNameLeading(curr_arg).empty()) {
                flag_end = true;
                continue;
            }

            // to mark that flag pair ended, use '--' or '-'
            // all arguments will be treated as positioinal args
            if (flag_end) {
                no_flag_args.push_back(curr_arg);
                continue;
            }

            // add new pair then reset status
            if (expecte_value) {

                // A flag-looking arg while a value is pending means the
                // pending flag carries no value of its own.
                if (isFlagStr(curr_arg)) {

                    // consecious flags, set the first flag's value as "true"
                    enableFlag(curr_flag);

                    // trySingleFlagValuePair stores the pair itself and
                    // reports success. Without this continue the same text
                    // would also be stored as the pending flag's value.
                    if (trySingleFlagValuePair(curr_arg)) {
                        expecte_value = false;
                        continue;
                    }

                    curr_flag = curr_arg;
                    continue;
                }

                // flag_map[curr_flag] = curr_arg;
                pushNewFlagValuePair(curr_flag, curr_arg);
                expecte_value = false;
                continue;
            }

            // if not flag string, push into no_flag_args
            if (!isFlagStr(curr_arg)) {
                no_flag_args.push_back(curr_arg);
                continue;
            }

            // treat this as flag string, trySingleFlagValuePair
            // if not single string pair, set expected value as true
            if (trySingleFlagValuePair(curr_arg)) {
                // if parse as single value pair success
                // set expecte_value as false
                expecte_value = false;
            } else {
                // parse as single flag failed, set curr_flag
                expecte_value = true;
                curr_flag = curr_arg;
            }
        }

        // treat the last flag as enable
        if (expecte_value == true) {
            enableFlag(curr_flag);
        }
    }

    // At function visit the all no_flag_args with original sequence
    // return [invalid_arg] if index out of range
    // You should use if arg == invalid_arg to verify it
    arg_type At(int index) const {
        if (index >= no_flag_args.size()) {
            return invalid_arg;
        }

        return no_flag_args[index];
    }

    // size return the non-flag args count
    size_t size() const { return no_flag_args.size(); }

    // Flag find the value by flag string
    // it will search both
    // name (e.g. --port or port)
    // and
    // short_name (e.g. -p or p)
    //
    // If not found, it will return default value
    arg_type Flag(const flag_type &name, const flag_type &short_name,
                  const arg_type &default_value) {

        string flag_name = stripFlagNameLeading(name);
        string flag_short_name = stripFlagNameLeading(short_name);

        // this mean name of flag found, return this vlaue directly
        if (flag_pairs.find(flag_name) != flag_pairs.end()) {

            return flag_pairs[flag_name];
        } else if (flag_pairs.find(flag_short_name) != flag_pairs.end()) {
            return flag_pairs[flag_short_name];
        }

        return default_value;
    }

    // IsEnableFlag return if this flag is enable
    // to enable a flag, use
    // --flag-name without any arguments after that or
    // --falg-name --next-flag
    // e.g.  --verbose --debug --output file.txt
    // The verbose and debug will be enabled
    //
    // if enabled, return true else false
    bool IsEnableFlag(const flag_type &name, const flag_type &short_name) {
        bool res = false;

        string flag_name = stripFlagNameLeading(name);
        string flag_short_name = stripFlagNameLeading(short_name);

        if (flag_pairs.find(flag_name) != flag_pairs.end() &&
            flag_pairs[flag_name] == enable_value) {
            res = true;
        } else if (flag_pairs.find(flag_short_name) != flag_pairs.end() &&
                   flag_pairs[flag_short_name] == enable_value) {
            res = true;
        }

        return res;
    }
};

} // namespace fpr
