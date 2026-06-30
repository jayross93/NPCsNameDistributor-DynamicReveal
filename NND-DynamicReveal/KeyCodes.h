#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

// Maps a human-readable key name (as you'd write it in an INI file) to its
// DirectInput keyboard scan code. Only single keys are supported (no
// modifiers like Ctrl/Shift combos) - that's all this plugin needs.
namespace KeyCodes
{
	inline const std::unordered_map<std::string, std::uint32_t>& Table()
	{
		static const std::unordered_map<std::string, std::uint32_t> table = {
			{ "A", 0x1E }, { "B", 0x30 }, { "C", 0x2E }, { "D", 0x20 }, { "E", 0x12 },
			{ "F", 0x21 }, { "G", 0x22 }, { "H", 0x23 }, { "I", 0x17 }, { "J", 0x24 },
			{ "K", 0x25 }, { "L", 0x26 }, { "M", 0x32 }, { "N", 0x31 }, { "O", 0x18 },
			{ "P", 0x19 }, { "Q", 0x10 }, { "R", 0x13 }, { "S", 0x1F }, { "T", 0x14 },
			{ "U", 0x16 }, { "V", 0x2F }, { "W", 0x11 }, { "X", 0x2D }, { "Y", 0x15 },
			{ "Z", 0x2C },

			{ "0", 0x0B }, { "1", 0x02 }, { "2", 0x03 }, { "3", 0x04 }, { "4", 0x05 },
			{ "5", 0x06 }, { "6", 0x07 }, { "7", 0x08 }, { "8", 0x09 }, { "9", 0x0A },

			{ "F1", 0x3B }, { "F2", 0x3C }, { "F3", 0x3D }, { "F4", 0x3E },
			{ "F5", 0x3F }, { "F6", 0x40 }, { "F7", 0x41 }, { "F8", 0x42 },
			{ "F9", 0x43 }, { "F10", 0x44 }, { "F11", 0x57 }, { "F12", 0x58 },

			{ "SPACE", 0x39 }, { "TAB", 0x0F }, { "ENTER", 0x1C },
			{ "BACKSPACE", 0x0E }, { "ESCAPE", 0x01 }, { "ESC", 0x01 },
			{ "INSERT", 0xD2 }, { "DELETE", 0xD3 }, { "HOME", 0xC7 }, { "END", 0xCF },
			{ "PAGEUP", 0xC9 }, { "PAGEDOWN", 0xD1 },
			{ "LBRACKET", 0x1A }, { "RBRACKET", 0x1B }, { "SEMICOLON", 0x27 },
			{ "APOSTROPHE", 0x28 }, { "COMMA", 0x33 }, { "PERIOD", 0x34 }, { "SLASH", 0x35 },
			{ "BACKSLASH", 0x2B }, { "MINUS", 0x0C }, { "EQUALS", 0x0D }, { "GRAVE", 0x29 },
		};
		return table;
	}

	// Parses a key name like "H" or "f5" (case-insensitive, whitespace trimmed)
	// into its scan code. Returns std::nullopt if the name isn't recognized.
	inline std::optional<std::uint32_t> Parse(std::string_view a_name)
	{
		std::string name{ a_name };

		// trim whitespace
		const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		name.erase(name.begin(), std::find_if(name.begin(), name.end(), notSpace));
		name.erase(std::find_if(name.rbegin(), name.rend(), notSpace).base(), name.end());

		std::transform(name.begin(), name.end(), name.begin(),
			[](unsigned char c) { return static_cast<char>(std::toupper(c)); });

		const auto& table = Table();
		if (const auto it = table.find(name); it != table.end()) {
			return it->second;
		}
		return std::nullopt;
	}
}
