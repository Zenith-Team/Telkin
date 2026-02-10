# Telkin

## Overview
Telkin is a powerful and dynamic mod-loading framework designed for the Wii U's CafeOS system. It facilitates target-specific loading of plugins and provides an interface for applying runtime patches in a safe and compatible manner.

## Features
- **Hooking Engine:** Supports three primary types of runtime modifications
  - Branch Hook: Redirect function calls and execution to custom code
  - Pointer Hook: Swap out data or function pointers in memory
  - Patch Hook: Apply raw data patches of varying sizes
- **Validation:** We have strict checks in place to ensure a stable environment before booting
  - Semantic Versioning: Built-in support for complex version requirements ensuring mod compatibility is met at runtime
  - Hook Scanning: Automatically check for overlapping memory writes and catch it before applying patches
  - Cache Invalidation: Coherency is enforced via synchronization steps taken place during the patch application step

## Usage (End Users)
### Wii U

### Cemu Emulator
Telkin's integrated Cemu launcher comes packaged with mod bundles that rely on it, so no additional steps are necessary. Simply utilize the GraphicPack system as usual.

## Usage (Developers)
The developer experience is driven by [Tachyon](https://github.com/Zenith-Team/Tachyon), which handles building and management of Telkin codemods.

## Credits
- [Luminyx](https://github.com/Luminyx1) - Loader development, Cemu launcher
- [jhmaster](https://github.com/jhmaster2000) - Loader development
- [techmuse](https://github.com/techmuse8) - Aroma launcher

## License
All code in the Telkin repository has been made available under the [Mozilla Public License v2.0](https://github.com/Zenith-Team/Telkin/blob/main/LICENSE.txt).
