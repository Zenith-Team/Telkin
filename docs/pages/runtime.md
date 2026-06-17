# Runtime
A minimal runtime is available for modules to link to.

## Cafe OS
Telkin exports function pointers for Cafe OS system modules via [DynamicLibs](https://github.com/Zenith-Team/DynamicLibs) which are automatically available for dependers to use. Both a module-based (`dynamic_libs/`) and SDK-style (`cafe/`) path interface is available.

## LibC
To implement functions from libc when necessary due to a module being freestanding or the function not existing in the target application, a limited set of basic definitions is available in @ref include/telkin/Runtime.h for usage. Simply define the corresponding macro before including the header to define it.

Telkin already defines the following:
```cpp
#define TK_IMPL_FREE
#define TK_IMPL_ABORT
#define TK_IMPL_CALLOC
#define TK_IMPL_MEMCMP
#define TK_IMPL_MEMSET
#define TK_IMPL_MEMCPY
#define TK_IMPL_STRCMP
#define TK_IMPL_STRCAT
#define TK_IMPL_STRTOL
#define TK_IMPL_STRCHR
#define TK_IMPL_STRCPY
#define TK_IMPL_STRLEN
#define TK_IMPL_STRPBRK
#define TK_IMPL_STRNCMP
#define TK_IMPL_ISDIGIT
#define TK_IMPL_ISSPACE
#define TK_IMPL_ISALPHA
#define TK_IMPL_ISUPPER
#define TK_IMPL_MEMMOVE
#define TK_IMPL_OPERATOR_NEW
#define TK_IMPL_OPERATOR_DELETE
#include <telkin/Runtime.h>
```

> [!WARNING]
> It is recommended to minimally define functions this way ONLY when absolutely necessary. Prefer linking to existing implementation in the target application when possible for compatibility reasons.

## Compiler-RT
Software implementations of float-double conversions not supported by the Wii U processor are implemented via [compiler-rt](https://compiler-rt.llvm.org/) and will automatically be referenced by user code from the compiler.

## LibC++
Most template classes that are defined within a header should already be available through the [RedStandard](https://github.com/Zenith-Team/RedStandard) distribution of the C++ standard library. Other functions may require linking to existing implementations within the target application.
