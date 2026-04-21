<img alt="banner" src="https://github.com/user-attachments/assets/ad115c21-629c-41f0-9e2f-35699e8909dc" />
<div align="center">
  <img alt="wiiu" height="56" src="https://github.com/user-attachments/assets/8afc18be-3148-4c23-8d14-9532606f103f">
  <a href="https://go.nsmbu.net/discord">
    <img alt="discord" height="56" src="https://github.com/user-attachments/assets/4f7029c3-2eec-4ea4-b832-1c83c89ff663">
  </a>
  <a href="https://zenith.nsmbu.net/wiki/Telkin">
    <img alt="docs" height="56" src="https://github.com/user-attachments/assets/f3cf0ed1-75fe-472f-8ec7-4e6ce971f8bf">
  </a>
</div>

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
- And more coming soon: Hot-reload and reversible hooks are currently being investigated

## Usage (Players)
### Wii U
Telkin's Aroma launcher requires a modified Wii U running [Aroma](https://aroma.foryour.cafe/) custom firmware. Visit [here](https://wiiu.hacks.guide) for a tutorial on installing Aroma. The launcher plugin is to be placed in the directory `sd:/wiiu/environments/aroma/plugins` on your SD card. Afterwards, place the `telkin` folder bundled with the mod on the root of your SD card. Once everything is in place, simply launch the game and the mods should be applied!

### Cemu Emulator
Telkin's integrated Cemu launcher comes bundled with mods that rely on it, so no additional steps are necessary. A tutorial for installing mods can be found [here](https://zenith.nsmbu.net/wiki/Installing_Mods).

## Usage (Developers)
The developer experience is driven by [Tachyon](https://github.com/Zenith-Team/Tachyon), which handles building and management of Telkin codemods. Read the [documentation](https://zenith.nsmbu.net/wiki/Other:WURPLS) for more information.

## Credits
- [Luminyx](https://github.com/Luminyx1) - Loader development, Cemu launcher
- [jhmaster](https://github.com/jhmaster2000) - Loader development
- [techmuse](https://github.com/techmuse8) - Aroma launcher
- [STUPID](https://github.com/stupidestmodder) - Cemu launcher

## License
All code in the Telkin repository has been made available under the [Mozilla Public License v2.0](https://github.com/Zenith-Team/Telkin/blob/main/LICENSE.txt).
