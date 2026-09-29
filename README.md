# Flag Parser

A single-header C++17 argument parser.

It splits `argv` into flag/value pairs plus an ordered list of positional
arguments. It never needs to know which flags your program has: you declare
them at the call site with `Flag()` and `IsEnableFlag()`, and anything you never
ask about is simply never looked up.

---

## Quick Start

- download `FlagParser.hpp` then include it.

> [!NOTE]
> All you need is the hpp file.
> There is no cpp source file for this class.

- basic usage:

The runnable tour lives in `main.cpp`, reproduced below.

```cpp
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
```

- Compile

```bash
mkdir bin
g++ -std=c++17 main.cpp -o bin/main
```

- Run

```bash
bin/main --host=example.com -p 9090 --verbose --debug

# output
Host:    example.com
Port:    9090
Verbose: true
Debug:   true
Verbose via value comparison: true
Non-flag arguments (0):
```

```bash
bin/main file1.txt file2.txt --verbose

# output
Host:    127.0.0.1
Port:    8080
Verbose: true
Debug:   false
Verbose via value comparison: true
Non-flag arguments (2):
  [0] file1.txt
  [1] file2.txt
```

```bash
bin/main -- --weird-name.png

# output
Host:    127.0.0.1
Port:    8080
Verbose: false
Debug:   false
Verbose via value comparison: false
Non-flag arguments (1):
  [0] --weird-name.png
```

```bash
bin/main hello world

# output
Host:    127.0.0.1
Port:    8080
Verbose: false
Debug:   false
Verbose via value comparison: false
Non-flag arguments (2):
  [0] hello
  [1] world
```

---

## How arguments are parsed

### Flag forms

A flag that carries a value can be written in four equivalent shapes:

```bash
--host example.com   # long name, value in the next argument
--host=example.com   # long name, value joined by '='
-h example.com       # short name, value in the next argument
-h=example.com       # short name, value joined by '='
```

Leading dashes are stripped before lookup, and the name is compared with or
without them, so `Flag("host", "h", ...)` and `Flag("--host", "-h", ...)` find
the same entry.

### Reading a value

```cpp
std::string host = parser.Flag("host", "h", "127.0.0.1");
```

`Flag(name, short_name, default_value)` returns the value stored under
`name`, then the value stored under `short_name`, and finally
`default_value` when neither was given. Values are returned as raw strings;
type conversion is the caller's job.

### Flags without a value

A flag written on its own is stored as `fpr::enable_value` (`"true"`):

```bash
--verbose --debug --output file.txt
```

Here `--verbose` and `--debug` are enabled and `--output` still takes
`file.txt`. The rule is:

> A flag takes the next argument as its value, unless that argument starts
> with `-` or is a dash-only token. In that case the flag itself is enabled.

Check it with either of these equivalent forms:

```cpp
bool verbose = parser.IsEnableFlag("verbose", "v");
bool same    = parser.Flag("verbose", "v", "false") == fpr::enable_value;
```

Because both compare against `fpr::enable_value`, an explicit
`--verbose=false` reports as *not* enabled.

### Ending flags

A token made only of dashes (`--`, `-`, `---`) ends flag parsing. Every later
argument is positional, and the marker itself is not kept:

```bash
bin/main -- --weird-name.png   # one positional: --weird-name.png
bin/main - photo.jpg           # one positional: photo.jpg
```

### Positional arguments

Everything that is not a flag is kept in its original order:

```cpp
for (int i = 0; i < static_cast<int>(parser.size()); ++i) {
    std::string arg = parser.At(i);
    // ...
}
```

`size()` returns how many there are; `At(i)` returns `fpr::invalid_arg` when
`i` is out of range.

---

## Notes

- The parser does not know which flags exist. A misspelled flag is stored under
  its misspelled name and simply never looked up, so `Flag()` returns the
  default value instead of reporting an error.
- A value that starts with `-` cannot be passed as the following argument; use
  the joined form: `--font=-myfont.ttf`, not `--font -myfont.ttf`.
- `--name=` with nothing after the `=` throws `std::invalid_argument`, since a
  flag written that way is neither a value pair nor a bare flag.
- Empty arguments (`""`) are skipped.
