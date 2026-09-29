#include "FlagParser.hpp"
#include <iostream>
#include <string>

// A short tour of the parser. Supported forms:
//
//   --host example.com   long flag, value in the next argument
//   --host=example.com   long flag, value joined by '='
//   -h example.com       short flag, value in the next argument
//   -h=example.com       short flag, value joined by '='
//   --verbose            flag with no value: stored as fpr::enable_value
//   --                   end of flags (any token made only of dashes works);
//                        later arguments become positional, the marker itself
//                        is not kept
//
// Try:
//   main --host=example.com -p 9090 --verbose --debug
//   main file1.txt file2.txt --verbose
//   main -- --weird-name.png
int main(int argc, char **argv) {
    fpr::FlagParser parser(argc, argv);

    // Flags that carry a value: pass the long name, the short name and the
    // default used when the flag is absent. Leading dashes are optional, so
    // "host" and "--host" find the same entry.
    const std::string host = parser.Flag("host", "h", "127.0.0.1");
    const std::string port = parser.Flag("port", "p", "8080");

    // Flags without a value are stored as fpr::enable_value ("true"), whether
    // they sit at the end of the command line or directly before another flag.
    const bool verbose = parser.IsEnableFlag("verbose", "v");
    const bool debug = parser.IsEnableFlag("debug", ""); // no short name

    std::cout << "Host:    " << host << "\n";
    std::cout << "Port:    " << port << "\n";
    std::cout << "Verbose: " << (verbose ? "true" : "false") << "\n";
    std::cout << "Debug:   " << (debug ? "true" : "false") << "\n";

    // IsEnableFlag(name, short) is equivalent to comparing Flag() against
    // fpr::enable_value; pick whichever reads better at the call site.
    // This also means --verbose=false reports as not enabled.
    const bool verbose_via_value =
        parser.Flag("verbose", "v", "false") == fpr::enable_value;
    std::cout << "Verbose via value comparison: "
              << (verbose_via_value ? "true" : "false") << "\n";

    // Arguments that are not flags are kept in their original order. At(i)
    // returns fpr::invalid_arg when i is out of range, but size() is the
    // simpler way to walk them.
    std::cout << "Non-flag arguments (" << parser.size() << "):\n";
    for (int i = 0; i < static_cast<int>(parser.size()); ++i) {
        std::cout << "  [" << i << "] " << parser.At(i) << "\n";
    }

    return 0;
}
