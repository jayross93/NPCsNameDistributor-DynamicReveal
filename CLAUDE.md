# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A single-file SKSE plugin for Skyrim Special Edition (SE/AE/VR) that integrates NPCs Names Distributor (NND) with SkyrimNet. Features:
- Press **H** (configurable) to reveal whatever NPC the crosshair is on, without opening dialogue.
- Auto-reveal when an NPC mentions their own name in a SkyrimNet AI conversation (v1.1 build).
- Registers an `nnd_name` SkyrimNet decorator so the AI knows the NPC's real name.

## Building

Requires: Visual Studio 2022 with the "Desktop development with C++" workload, and vcpkg with `VCPKG_ROOT` set.

**From VS Code** (primary workflow):
1. Open folder → CMake Tools prompts to select a preset → choose **release-1.0** or **release-1.1**
2. First configure downloads and compiles dependencies (~10–20 min via vcpkg)
3. `Ctrl+Shift+P` → `CMake: Build`
4. Output: `build/<preset>/NNDDynamicReveal.dll`
5. Release zip: `build/<preset>/NNDDynamicReveal.zip`

**From the command line** (needs VS developer environment):
```powershell
cmake --preset release-1.1
cmake --build build/release-1.1
```

Use `release-1.0` for hotkey-only build. Use `release-1.1` for hotkey + SkyrimNet auto-reveal.

**Auto-deploy:** Set `SKYRIM_MODS_FOLDER` or `SKYRIM_FOLDER` environment variables and the post-build step copies the DLL automatically.

**Note:** Must be run from a Visual Studio developer prompt (or via `VsDevCmd.bat`) so that MSVC standard library headers are on the include path.

## Architecture

All plugin logic lives in `plugin.cpp` (~280 lines). The other source files are pure data/headers.

**Startup sequence (`SKSEPluginLoad`):**
1. `SKSE::Init` + logging setup + INI load
2. On `kPostLoad` — calls `NND_API::RequestPluginAPI()` to get `IVNND4*` from NND; stored in `g_nnd`
3. On `kInputLoaded` — registers `InputHandler` singleton with `RE::BSInputDeviceManager`
4. On `kDataLoaded` — registers `nnd_name` decorator + `dialogue` event callback with SkyrimNet (v1.1)

**Per-frame input path:**
`InputHandler::ProcessEvent` → consumes pending auto-reveal from `g_pendingReveal` (main thread safe) → scans `RE::InputEvent*` chain for `g_revealKeyCode` → calls `RevealCrosshairTarget()`

**Auto-reveal path (v1.1):**
SkyrimNet fires `dialogue` callback on its ThreadPool → callback checks `isPlayerInitiated`, extracts dialogue text, stores `{formId, text}` in mutex-protected `g_pendingReveal` → `ProcessEvent` on main thread looks up actor, gets NND name, checks if text contains name, calls `RevealName` if match.

**`nnd_name` decorator:**
Registered with SkyrimNet on `kDataLoaded`. Returns `GetRawName(actor)` with bracket disambiguation suffix stripped (e.g. `"Olffik Jurgensen [Whiterun Guard]"` → `"Olffik Jurgensen"`). Returns empty string if actor has no NND name or if the NND name equals the vanilla name.

**Key files:**
- `plugin.cpp` — entire plugin implementation
- `KeyCodes.h` — `KeyCodes::Parse()` maps INI key names to DirectInput scan codes
- `NND_API.h` — verbatim copy of NND's public modder API; do not modify; update by replacing with the latest from [NND's GitHub](https://github.com/adya/NPCs-Names-Distributor/blob/main/include/NND_API.h)
- `PCH.h` — precompiled header; includes `RE/Skyrim.h` and `SKSE/SKSE.h` (CommonLibSSE-NG)
- `SkyrimNet/prompts/submodules/character_bio/0005_nnd_name.prompt` — Jinja2 template injected into SkyrimNet character bios

## Dependencies (vcpkg)

Declared in `vcpkg.json`; pinned via `vcpkg-configuration.json`:
- **commonlibsse-ng** — Skyrim reverse-engineering library (from the colorglass GitLab registry)
- **simpleini** — INI file reading/writing

spdlog and xbyak are pulled in transitively by commonlibsse-ng.

## INI configuration

The plugin creates `Data/SKSE/Plugins/NNDDynamicReveal.ini` on first run with `sRevealKey=H`.

## Runtime artifacts

- Log: `Documents/My Games/Skyrim Special Edition/SKSE/NNDDynamicReveal.log`
- INI: `Data/SKSE/Plugins/NNDDynamicReveal.ini`

## Known quirks

- MO2 may redirect the log path to `Documents/My Games/Skyrim.INI/SKSE/NNDDynamicReveal.log` depending on the active profile.
- `SKSE::GetTaskInterface()->AddTask()` silently no-ops when called from SkyrimNet's ThreadPool in AE. The auto-reveal therefore uses `std::mutex`-protected cross-thread state consumed in `ProcessEvent` instead.
- SkyrimNet wraps dialogue text values in an extra escaped-quote layer: `\"dialogue\":\"\\\"actual text\\\"\"`. `ExtractEscapedField` handles this by skipping `\"` sequences that are preceded by `\`.

## Agent workflow

Three subagents handle all non-trivial tasks. Claude MUST invoke them via the Agent tool — never do research, planning, or coding inline.

| Task | Agent |
|------|-------|
| Gather information (codebase + online) | `researcher` |
| Write an implementation plan | `planner` |
| Implement code from a plan | `code-writer` |

**Work files folder:** `.claude/work/`
All research and plan artifacts are written here as `<slug>-research.md` and `<slug>-plan.md`.

**Standard workflow for any feature or fix:**
1. Spawn **researcher** — write full findings to `.claude/work/<slug>-research.md`, return brief summary.
2. Spawn **planner** — read research file, write step-by-step plan to `.claude/work/<slug>-plan.md`, return brief summary.
3. Spawn **code-writer** — read plan file and implement exactly as written.
