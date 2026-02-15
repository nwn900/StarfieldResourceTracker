# Starfield Resource Tracker

SFSE plugin for **Starfield 1.15.222** (and all versions supported by [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256)). It lets you tag **missing resources** for research, weapon modification, armor modification, and industrial component fabrication, and shows a **magnifier icon** when those resources appear in vendor menus, containers, or when the item is selected—similar to Fallout 4’s “Tag for Search” for crafting components.

## Requirements

- [Starfield Script Extender (SFSE)](https://www.nexusmods.com/starfield/mods/106)
- [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256)  
  Place the `.bin` files in `Data/SFSE/Plugins/` so this plugin stays compatible across game updates.
- Starfield (Steam); Gamepass/MS Store is not supported by SFSE.

## Getting the DLL

- **Pre-built (no compiler):** Push this repo to GitHub and use the **Actions** tab → **Build ResourceTracker DLL** workflow; download the `ResourceTracker-DLL` artifact. See **[BUILD.md](BUILD.md)**.
- **Build locally:** You need **Visual Studio 2022** (Desktop C++), **CMake**, and **vcpkg**. Full steps are in **[BUILD.md](BUILD.md)**.

**Install:** Copy `ResourceTracker.dll` into `Starfield/Data/SFSE/Plugins/`. You must have [SFSE](https://www.nexusmods.com/starfield/mods/106) and [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256) installed.

## Usage (current behavior)

- **Tracked list**: The plugin keeps a persistent set of **form IDs** in `Data/SFSE/Plugins/ResourceTracker.json`. These are the “missing resources” you want to highlight.
- **Magnifier icon**: To show the icon in the game UI (vendor, container, item selection), the plugin must hook the game’s item-drawing code and call `ResourceTracker::TrackedResources::ShouldShowMagnifier(formId)`. Those hooks depend on **Address Library IDs** for the relevant functions (see below).
- **Tagging missing resources**: In a full implementation, the game UI (Research / Weapon mod / Armor mod / Industrial) would offer a “Tag for search” (or similar) action that adds the missing component form IDs to this list. That also requires Address Library (or equivalent) IDs for menu/recipe context.

## Address Library and compatibility

The plugin is built with **`UsesAddressLibrary(true)`** and uses **REL::ID** (and related types) for any game offsets, so it is designed to work with **all game versions** covered by the Address Library `.bin` files, without recompiling.

To finish the feature set you need the correct **Address Library IDs** for your Starfield version (from the [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256) offsets data):

1. **Item list rendering**  
   The function (or equivalent) that draws a single item row in:
   - Vendor menu  
   - Container/inventory  
   - Item selection UI  
   Hook this and, when drawing an entry, call `TrackedResources::ShouldShowMagnifier(itemFormID)`. If true, draw the magnifier icon at the appropriate position.

2. **Crafting / Research UI (optional)**  
   To support “tag missing for search” from the game UI, you need:
   - Menu/context when a recipe or research project is selected.
   - The list of required component form IDs and (optionally) player counts, so you can add only **missing** (shortfall) form IDs to `TrackedResources`.

The code in `Plugin/src/ResourceTracker.cpp` has placeholder comments where these hooks and IDs should go.

## Data file

- **Path**: `Data/SFSE/Plugins/ResourceTracker.json`
- **Format**: `{ "tracked": [ formId1, formId2, ... ] }`  
  Form IDs are stored as unsigned integers. The plugin loads this on startup and saves it when the tracked set changes.

## License

This project follows the same licensing approach as CommonLibSF-based plugins: **GPL-3.0-or-later** with the Modding Exception and GPL-3.0 Linking Exception (with Corresponding Source). See CommonLibSF and SFSE for details.

## Credits

- [SFSE](https://github.com/ianpatt/sfse) and [CommonLibSF](https://github.com/Starfield-Reverse-Engineering/CommonLibSF) / [libxse/commonlibsf](https://github.com/libxse/commonlibsf)
- [DKUtil](https://github.com/gottyduke/DKUtil)
- [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256)
- [SFSE Plugin Template](https://github.com/gottyduke/SF_PluginTemplate)
