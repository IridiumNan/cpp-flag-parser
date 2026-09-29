# Flag Parser

This is a class that parse cpp args as flag-value pair and no flag value (or main command).

You should not use it to assign the no value flag like `--verbose` or `-v`, use `--verbose=true` instead.

---

## Quick Start

- download `FlagParser.hpp` then include。

> [!NOTE]
> All you need is the hpp file
> There is no cpp source file for this class

- basic usage：

This content already written on `main.cpp`
You can compile then run it directly.

```cpp
#include "FlagParser.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    fpr::FlagParser parser(argc, argv);

    // get the value by flag
    // if flag not found, return the default value
    std::string host    = parser.Flag("host", "h", "127.0.0.1");
    std::string port    = parser.Flag("port", "p", "8080");

    // You can also begin with --
    std::string verbose = parser.Flag("--verbose", "-v", "false");

    std::cout << "Host:    " << host << "\n";
    std::cout << "Port:    " << port << "\n";
    std::cout << "Verbose: " << verbose << "\n";

    // get all non-flag arguments
    // visit them by index
    std::cout << "Non-flag arguments:\n";
    for (int i = 0; ; ++i) {
        std::string arg = parser.At(i);
        if (arg == fpr::invalid_arg) {
            break;
        }
        std::cout << "  [" << i << "] " << arg << "\n";
    }

    return 0;
}
```

- Compile

```bash
mkdir bin
g++ -std=c++17 main.cpp -o bin/main
```

- Run

```bash
bin/main --host=example.com -p 9090 --verbose=true file1.txt file2.txt

# output
Host:    example.com
Port:    9090
Verbose: true
Non-flag arguments:
  [0] file1.txt
  [1] file2.txt
```

```bash
bin/main hello world

# output
Host:    127.0.0.1
Port:    8080
Verbose: false
Non-flag arguments:
  [0] hello
  [1] world
```
