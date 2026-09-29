
#include "FlagParser.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    fpr::FlagParser parser(argc, argv);

    // get the value by flag
    // if flag not found, return the default value
    std::string host = parser.Flag("host", "h", "127.0.0.1");
    std::string port = parser.Flag("port", "p", "8080");

    // You can also begin with --
    std::string verbose = parser.Flag("--verbose", "-v", "false");

    std::cout << "Host:    " << host << "\n";
    std::cout << "Port:    " << port << "\n";
    std::cout << "Verbose: " << verbose << "\n";

    // get all non-flag arguments
    // visit them by index
    std::cout << "Non-flag arguments:\n";
    for (int i = 0;; ++i) {
        std::string arg = parser.At(i);
        if (arg == fpr::invalid_arg) {
            break;
        }
        std::cout << "  [" << i << "] " << arg << "\n";
    }

    return 0;
}
