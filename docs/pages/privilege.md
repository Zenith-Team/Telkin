# Privileged Operations
Telkin provides the @ref tk::privilegedWrite function-pointer to perform memory copy operations with kernel permissions.

Example:
```cpp
#include <telkin/Privilege.h>
#include <telkin/Assembly.h>

using namespace tk::ppc;

void OverwriteSyscall() {
    const u32 instruction = li(GPR::r3, 0xCAFE);
    tk::privilegedWrite((const void*)0x02000000, &instruction, sizeof(u32));
}

```

> [!NOTE]
> The `.rodata` section of RPX files is already writable, so a privileged write is unnecessary for modifying it.

See [Inline Assembly](./assembly.md) for more information on the constexpr assembler used.
