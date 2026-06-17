# Print Logging
Telkin provides wrapper functions for the OS logging API which automatically include the source file and line number of the call. The formatting is the same as the standard `printf` function.

Example:
```cpp
#include <telkin/Print.h>

void main() {
    tk::println("Hello world!");
}
```
Results in:
```
[Main.cpp:4] Hello world!
```

> [!NOTE]
> The file name is automatically truncated at compile-time so build paths are not leaked in the binary.

A @ref tk::fatal function is also provided for fatal errors, which will immediately halt the application and display an error message on-screen.

See @ref include/telkin/Print.h for more information.
