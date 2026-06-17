# Hooks
Telkin provides a number of macros for defining hooks which will be injected at runtime. These can be split into the following categories:

## Branch Hooks
| Signature | Description | Example |
| - | - | - |
| `tBranch(addr, target, type)` | Address to write the branch instruction. <br> Identifier of function to branch to. <br> tk::BranchType of branch to take. | `tBranch(0x02000000, mod::func, tk::BranchType::bl);` |
| `tBranch(addr, target, sig, type)` | Address to write the branch instruction. <br> Identifier of function to branch to. <br> Signature of target overload. <br> tk::BranchType of branch to take. | `tBranch(0x02000000, mod::func, bool(int, void*), tk::BranchType::bl);` |
| `tBranchEx(addr, targetSym, type)` | Address to write the branch instruction. <br> Mangled symbol string of function to branch to. <br> tk::BranchType of branch to take. | `tBranchEx(0x02000000, "_Z8MyFuncv", tk::BranchType::bl);` |

## Pointer Hooks
| Signature | Description | Example |
| - | - | - |
| `tPointerCode(addr, target)` | Address to write the pointer to. <br> Identifier of function to point to. | `tPointerCode(0x10000000, mod::func);` |
| `tPointerCode(addr, target, sig)` | Address to write the pointer to. <br> Identifier of function to point to. <br> Signature of target overload. | `tPointerCode(0x10000000, mod::func, bool(int, void*));` |
| `tPointerData(addr, target)` | Address to write the pointer to. <br> Identifier of variable to point to. | `tPointerData(0x10000000, mod::variable);` |
| `tPointerEx(addr, targetSym, isData)` | Address to write the pointer to. <br> Mangled string of symbol to point to. <br> Whether the target symbol is data rather than code. | `tPointerEx(0x10000000, "_ZN4sead8Matrix34IfE5identE")` |

## Patch Hooks
The syntax for patch hooks is as follows:
```cpp
tPatch{unit size in bits}{type, one of u/s/f}(addr, values...)
```
Types correspond to:
| Shorthand | Type |
| - | - |
| u | Unsigned Int |
| s | Signed Int |
| f | Float |

Unit size is the type for each entry in the patch, since patch hooks accept a variable number of data units to be applied contiguously as an array.

The following predefined utility patches are available for your convenience:
- `tPatchNop(addr)`: Nullifies the target instruction
- `tPatchBlr(addr)`: Causes the function to return
