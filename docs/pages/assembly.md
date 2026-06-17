# Inline Assembly
The @ref include/telkin/Assembly.h file includes helpers for working with inline assembly in C++ source files.

## Assembly Function Declaration
The `tAssembly` macro can be used to define the body of a function using only PowerPC assembly instructions.
```cpp
#define TELKIN_REGISTERS
#include <telkin/Assembly.h>

void func() tAssembly(
    li r3, 0xFF;
    blr;
)
```

Defining the `TELKIN_REGISTERS` macro will generate lots of macros that map register names such as `r3` to the underlying format that is expected by the compiler `%r3`. Since these identifiers are very short and may cause conflicts, you may globally opt-out of the macro by defining `TELKIN_NO_REGISTERS` through the compiler flags.

If a conflict is caused within a file that also needs the register definitions, you may include `UndefineRegisters.h` and `DefineRegisters.h` an unlimited number of times.

## Register Preservation
Applying the `tRegSave` attribute onto a C++ function definition will cause every single volatile register to be saved and restored at the end of the function, so that mid-function hooks do not corrupt any state. If the function does return a value (which corresponds to `r3`) then `r3` will be omitted from the restorations and thus will be allowed to change.

There are also macros available for use in assembly functions:
- `tSaveVolatileRegisters`: Saves r0, r3-r12, CR, CTR, and LR to the stack
- `tRestoreVolatileRegisters`: Restores r0, r3-12, CR, CTR, and LR from the stack
- `tSaveLR`: Saves LR to the stack (through clobbering r2)
- `tRestoreLR`: Restores LR from the stack (through clobbering r2)

## Compile-time Assembler
The @ref tk::ppc namespace provides a set of `consteval` functions which return assembled machine code `u32`s for various PowerPC instructions. This is useful alongside [patch hooks](./hooks.md) (specifically `tPatch32u`) to replace an instruction in-line within a function.
