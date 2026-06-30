# NND Dynamic Reveal — Plugin

A tiny companion SKSE plugin for **NPCs Names Distributor (NND)**.

Press a hotkey (default **H**) while your crosshair is on an NPC to reveal their real name — without opening dialogue, looting, or pickpocketing them. Built for setups like SkyrimNet where pressing E to talk has side effects you don't want just to find out who someone is.

Also integrates with SkyrimNet: if an AI NPC's generated dialogue contains their own name, it's revealed automatically (respects NND's "Reveal on greeting" MCM toggle).

## Requirements

- [SKSE64](https://skse.silverlock.org)
- NPCs Names Distributor (this repo ships a fixed fork — use that instead of the official release)

## Building

1. Set `VCPKG_ROOT` to your vcpkg install path.
2. From a VS Developer Command Prompt, inside this folder:
   ```powershell
   cmake --preset release
   cmake --build build/release
   ```
3. Output: `build/release/NNDDynamicReveal.dll` and `build/release/NNDDynamicReveal.zip`.

## Hotkey configuration

Launch once and the plugin creates `Data/SKSE/Plugins/NNDDynamicReveal.ini`:

```ini
[General]
sRevealKey=H

[SkyrimNet]
bAutoReveal=1
```

Valid key names are the uppercase strings in `KeyCodes.h` (A–Z, 0–9, F1–F12, Space, Tab, Enter, etc.). No modifier combos. Set `bAutoReveal=0` to disable the SkyrimNet integration.

## Source files

- `plugin.cpp` — entire plugin implementation
- `KeyCodes.h` — key-name → DirectInput scan code table
- `NND_API.h` — NND modder API (verbatim copy from [NND's GitHub](https://github.com/adya/NPCs-Names-Distributor/blob/main/include/NND_API.h))
- `PCH.h` — precompiled header
- `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `vcpkg-configuration.json` — CommonLibSSE-NG project (works across SE/AE/VR)
