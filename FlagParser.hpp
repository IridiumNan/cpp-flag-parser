#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fpr {

using std::string;

using flag_type = string;
using arg_type = string;

const string invalid_arg = "";

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
    bool isFlagStr(string arg) { return arg[0] == '-'; }

    // pushNewFlagValuePair will stript the leading '-' on the flag for parsing
    // then store safe
    void pushNewFlagValuePair(const flag_type &raw_flag,
                              const arg_type &value) {

        string flag = stripFlagNameLeading(raw_flag);
        flag_pairs[flag] = value;

        // DEBUG information
        // std::cout << "push new flag pair value, flag: " << flag
        //           << "\tvalue: " << value << '\n';
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

        // below cdoe is deprecated
        // Use [pushNewFlagValuePair] to push new pair safe
        // while (pair[key_start] == '-') {
        //     key_start++;
        // }
        //
        // string key = pair.substr(key_start, equal_pos - 1);
        // string value = pair.substr(equal_pos - 1, pair.size() - 1);

        // push this pair into map then return true for skip this arg
        // flag_pairs[key] = value;

        string flag = pair.substr(0, equal_pos);
        string value = pair.substr(equal_pos + 1, pair.size());
        pushNewFlagValuePair(flag, value);
        return true;
    }

    // stripFlagNameLeading, cut the leaading '-' on the front of raw flag
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
    // Other no flag paired args can be visit by [At](index) function
    //
    // NOTE:
    // If args[i] is empty str, it will be omit
    //
    // WARN: This class not parse the no value flag like --verbose
    // It just parse flag-value pair
    FlagParser(int argv, char **args) {

        arg_type curr_arg;
        // expecte_value is the flag mark if next argv should be treated as
        // value of flag if false, try if next arg is flag str, else push it
        // into no_flag_args
        bool expecte_value = false;

        // store current flag
        flag_type curr_flag;

        for (int i = 1; i < argv; i++) {

            curr_arg = args[i];
            if (curr_arg.empty())
                continue;

            // add new pair then reset status
            if (expecte_value) {
                // flag_map[curr_flag] = curr_arg;
                pushNewFlagValuePair(curr_flag, curr_arg);
                expecte_value = false;
                continue;
            }

            // if not flag string, push into no_flag_args
            if (!isFlagStr(curr_arg)) {
                // DEBUG information
                // std::cout << "push new no flag value: " << curr_arg << '\n';
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
                // DEBUG information
                // std::cout << "expecte_value = true" << "curr_flag " <<
                // curr_flag
                // << '\n';
            }
        }
    }

    // At function visit the all no_flag_args with original sequence
    // return [invalid_arg] if index out of range
    arg_type At(int index) {
        if (index >= no_flag_args.size()) {
            return invalid_arg;
        }

        return no_flag_args[index];
    }

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

        // DEBUG information
        // std::cout << "seach for flag, name: " << flag_name
        //           << "\t short_name: " << flag_short_name << '\n';

        // this mean name of flag found, return this vlaue directly
        if (flag_pairs.find(flag_name) != flag_pairs.end()) {

            return flag_pairs[flag_name];
        } else if (flag_pairs.find(flag_short_name) != flag_pairs.end()) {
            return flag_pairs[flag_short_name];
        }

        return default_value;
    }
};

} // namespace fpr
