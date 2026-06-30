# NPCs Name Distributor - Dynamic Reveal

A SKSE plugin for **NPCs Names Distributor (NND)** and **SkyrimNet** when using the Obscuring option.

NND can assign real names to NPCs while hiding them from the player — showing "Whiterun Guard" instead of the NPC's actual name until they're properly introduced. This plugin handles that introduction in two ways:

- **Hotkey reveal** — press **H** (configurable) while your crosshair is on an NPC to reveal their name, without opening any dialogue.
- **SkyrimNet auto-reveal** — when an NPC mentions their own name during a SkyrimNet AI conversation, it's revealed automatically. Only triggers when the spoken text actually contains their name.

It also registers an `nnd_name` **SkyrimNet decorator** that gives SkyrimNet's AI access to the NPC's real name for use in character bios — so the AI knows who the NPC is while still roleplaying whether they'd choose to share it with a stranger.

## Requirements

- [SKSE](https://skse.silverlock.org)
- [NPCs Names Distributor](https://www.nexusmods.com/skyrimspecialedition/mods/73081) 2.6.0+
- [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/121250) (optional — hotkey reveal works without it; decorator and auto-reveal require it)

## Installation

Download the release zip and drag it into Mod Organizer 2 (or install with Vortex). Enable the mod. That's it.

## Hotkey

The default reveal key is **H**. To change it, edit:

```
Data/SKSE/Plugins/NNDDynamicReveal.ini
```

```ini
[General]
sRevealKey=H
```

Valid values: any single key — letters A–Z, digits 0–9, F1–F12, Space, Tab, Enter, Backspace, Escape, Insert, Delete, Home, End, PageUp, PageDown, Comma, Period, Minus, Equals, Semicolon, Apostrophe, Slash, Backslash, LeftBracket, RightBracket. No modifier combos.

## Credits

- **[NPCs Names Distributor](https://github.com/adya/NPCs-Names-Distributor)** by adya — this mod ships with a modified build of NND that exposes a `GetRawName` API used for the SkyrimNet decorator and auto-reveal name matching. NND is MIT licensed.
- **[CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG)** — Skyrim reverse-engineering library used to build the SKSE plugin.