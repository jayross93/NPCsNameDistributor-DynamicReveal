# NND Dynamic Reveal

A pair of companion mods for [NPCs Names Distributor (NND)](https://www.nexusmods.com/skyrimspecialedition/mods/73081):

- **`NND-DynamicReveal`** — SKSE plugin. Press **H** (configurable) while your crosshair is on an NPC to reveal their name without opening dialogue. Also integrates with SkyrimNet: if an AI NPC mentions their own name in conversation, it's revealed automatically.
- **`NPCsNamesDistributor`** — A fork of NND with a bug fix: vanilla dialogue (pressing E) now correctly respects the MCM "Reveal on greeting" toggle, instead of always revealing names regardless of it.

## Requirements

- [SKSE64](https://skse.silverlock.org)
- [SKSEMenuFramework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) — for NND's own settings menu
- [SkyrimNet](https://github.com/MinLL/SkyrimNet-GamePlugin) — optional, only needed for the dynamic name-reveal-on-mention feature
- This zip bundles a fixed `NPCsNamesDistributor.dll` that **replaces** the official NND DLL

## Installation

1. Download `NNDDynamicReveal.zip` from the [releases page](https://github.com/jayross93/NPCsNameDistributor-DynamicReveal/releases) and drag it into MO2 (or extract `Data/SKSE/Plugins/` into your Skyrim `Data` folder manually).
2. If you also have the official NPCs Names Distributor mod installed (e.g. for its vanilla NPC exclusion data), make sure `NNDDynamicReveal` has **higher priority** in your mod list, so its bundled `NPCsNamesDistributor.dll` wins the file conflict over the official one.
3. In NND's MCM (open with the SKSEMenuFramework key), under **Obscurity**, turn off "Reveal in Greetings", "Reveal when Looting", and "Reveal when Pickpocketing". This mod's hotkey and SkyrimNet auto-reveal are meant to be the only ways names get revealed automatically — leaving NND's own reveal toggles on will reveal names through those paths too.

The plugin creates `Data/SKSE/Plugins/NNDDynamicReveal.ini` on first run. Change `sRevealKey` to remap the hotkey; set `[SkyrimNet] bAutoReveal=0` to disable the SkyrimNet integration.

## Building

Requires Visual Studio 2022 ("Desktop development with C++" workload), CMake, and [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set. cmake must be run from a VS Developer Command Prompt.

`NPCsNamesDistributor/extern/` (CommonLibSSE, CLibUtil, SKSEHooking) is vendored directly in this repo rather than pulled via git submodules, since the working copies here carry local fixes not present on their public branches.

**Plugin:**
```powershell
cd NND-DynamicReveal
cmake --preset release
cmake --build build/release
# Output: NNDDynamicReveal.zip, written to the repo root
```

**NND fork** (only needed to change NND behaviour):
```powershell
cd NPCsNamesDistributor
cmake --preset vs2022-windows-vcpkg-ae
cmake --build buildae --config Release
# Copy buildae/Release/NPCsNamesDistributor.dll → NND-DynamicReveal/vendor/
# Then rebuild the plugin.
```

Always use the `-ae` preset, not `-se` — the game runtime is the Anniversary Edition (1.6.x), and an `-se` build silently fails to export the version metadata modern SKSE requires, so SKSE refuses to load it at all (no crash, no error — it just never loads).

## Repository layout

```
NND-DynamicReveal/      SKSE plugin source
  plugin.cpp            all plugin logic (~350 lines)
  KeyCodes.h            hotkey name → DirectInput scan code
  NND_API.h             NND modder API (verbatim copy)
  vendor/               prebuilt NPCsNamesDistributor.dll

NPCsNamesDistributor/   NND fork source
  src/Hooks.cpp         key fix: line 461, dialogue reveal respects MCM
  src/ModAPI.cpp        RevealName() — kDialogue bypasses MCM, kGreeting respects it
```
