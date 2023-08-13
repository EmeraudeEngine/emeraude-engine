/*
 * Emeraude/Input/KeyboardTypes.hpp
 * This file is part of Emeraude
 *
 * Copyright (C) 2012-2023 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude
 * 
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* C/C++ standard libraries. */
#include <cstdint>
#include <string>

namespace Emeraude::Input
{
	enum Key
	{
		KeyUnknown = -1, // GLFW_KEY_UNKNOWN
		KeySpace = 32, //GLFW_KEY_SPACE
		KeyApostrophe = 39, // GLFW_KEY_APOSTROPHE '\''
		KeyComma = 44, // GLFW_KEY_COMMA ','
		KeyMinus = 45, // GLFW_KEY_MINUS '-'
		KeyPeriod = 46, // GLFW_KEY_PERIOD '.'
		KeySlash = 47, // GLFW_KEY_SLASH '/'
		Key0 = 48, // GLFW_KEY_0
		Key1 = 49, // GLFW_KEY_1
		Key2 = 50, // GLFW_KEY_2
		Key3 = 51, // GLFW_KEY_3
		Key4 = 52, // GLFW_KEY_4
		Key5 = 53, // GLFW_KEY_5
		Key6 = 54, // GLFW_KEY_6
		Key7 = 55, // GLFW_KEY_7
		Key8 = 56, // GLFW_KEY_8
		Key9 = 57, // GLFW_KEY_9
		KeySemicolon = 59, // GLFW_KEY_SEMICOLON ';'
		KeyEqual = 61, // GLFW_KEY_EQUAL '='
		KeyA = 65, // GLFW_KEY_A
		KeyB = 66, // GLFW_KEY_B
		KeyC = 67, // GLFW_KEY_C
		KeyD = 68, // GLFW_KEY_D
		KeyE = 69, // GLFW_KEY_E
		KeyF = 70, // GLFW_KEY_F
		KeyG = 71, // GLFW_KEY_G
		KeyH = 72, // GLFW_KEY_H
		KeyI = 73, // GLFW_KEY_I
		KeyJ = 74, // GLFW_KEY_J
		KeyK = 75, // GLFW_KEY_K
		KeyL = 76, // GLFW_KEY_L
		KeyM = 77, // GLFW_KEY_M
		KeyN = 78, // GLFW_KEY_N
		KeyO = 79, // GLFW_KEY_O
		KeyP = 80, // GLFW_KEY_P
		KeyQ = 81, // GLFW_KEY_Q
		KeyR = 82, // GLFW_KEY_R
		KeyS = 83, // GLFW_KEY_S
		KeyT = 84, // GLFW_KEY_T
		KeyU = 85, // GLFW_KEY_U
		KeyV = 86, // GLFW_KEY_V
		KeyW = 87, // GLFW_KEY_W
		KeyX = 88, // GLFW_KEY_X
		KeyY = 89, // GLFW_KEY_Y
		KeyZ = 90, // GLFW_KEY_Z
		KeyLeftBracket = 91, // GLFW_KEY_LEFT_BRACKET '['
		KeyBackSlash = 92, // GLFW_KEY_BACKSLASH '\'
		KeyRightBracket = 93, // GLFW_KEY_RIGHT_BRACKET ']'
		KeyGraveAccent = 96, // GLFW_KEY_GRAVE_ACCENT '`'
		KeyWorld1 = 161, // GLFW_KEY_WORLD_1 non-US #1
		KeyWorld2 = 162, // GLFW_KEY_WORLD_2 non-US #2
		KeyEscape = 256, // GLFW_KEY_ESCAPE
		KeyEnter = 257, // GLFW_KEY_ENTER
		KeyTab = 258, // GLFW_KEY_TAB
		KeyBackspace = 259, // GLFW_KEY_BACKSPACE
		KeyInsert = 260, // GLFW_KEY_INSERT
		KeyDelete = 261, // GLFW_KEY_DELETE
		KeyRight = 262, // GLFW_KEY_RIGHT
		KeyLeft = 263, // GLFW_KEY_LEFT
		KeyDown = 264, // GLFW_KEY_DOWN
		KeyUp = 265, // GLFW_KEY_UP
		KeyPageUp = 266, // GLFW_KEY_PAGE_UP
		KeyPageDown = 267, // GLFW_KEY_PAGE_DOWN
		KeyHome = 268, // GLFW_KEY_HOME
		KeyEnd = 269, // GLFW_KEY_END
		KeyCapsLock = 280, // GLFW_KEY_CAPS_LOCK
		KeyScrollLock = 281, // GLFW_KEY_SCROLL_LOCK
		KeyNumLock = 282, // GLFW_KEY_NUM_LOCK
		KeyPrintScreen = 283, // GLFW_KEY_PRINT_SCREEN
		KeyPause = 284, // GLFW_KEY_PAUSE
		KeyF1 = 290, // GLFW_KEY_F1
		KeyF2 = 291, // GLFW_KEY_F2
		KeyF3 = 292, // GLFW_KEY_F3
		KeyF4 = 293, // GLFW_KEY_F4
		KeyF5 = 294, // GLFW_KEY_F5
		KeyF6 = 295, // GLFW_KEY_F6
		KeyF7 = 296, // GLFW_KEY_F7
		KeyF8 = 297, // GLFW_KEY_F8
		KeyF9 = 298, // GLFW_KEY_F9
		KeyF10 = 299, // GLFW_KEY_F10
		KeyF11 = 300, // GLFW_KEY_F11
		KeyF12 = 301, // GLFW_KEY_F12
		KeyF13 = 302, // GLFW_KEY_F13
		KeyF14 = 303, // GLFW_KEY_F14
		KeyF15 = 304, // GLFW_KEY_F15
		KeyF16 = 305, // GLFW_KEY_F16
		KeyF17 = 306, // GLFW_KEY_F17
		KeyF18 = 307, // GLFW_KEY_F18
		KeyF19 = 308, // GLFW_KEY_F19
		KeyF20 = 309, // GLFW_KEY_F20
		KeyF21 = 310, // GLFW_KEY_F21
		KeyF22 = 311, // GLFW_KEY_F22
		KeyF23 = 312, // GLFW_KEY_F23
		KeyF24 = 313, // GLFW_KEY_F24
		KeyF25 = 314, // GLFW_KEY_F25
		KeyPad0 = 320, // GLFW_KEY_KP_0
		KeyPad1 = 321, // GLFW_KEY_KP_1
		KeyPad2 = 322, // GLFW_KEY_KP_2
		KeyPad3 = 323, // GLFW_KEY_KP_3
		KeyPad4 = 324, // GLFW_KEY_KP_4
		KeyPad5 = 325, // GLFW_KEY_KP_5
		KeyPad6 = 326, // GLFW_KEY_KP_6
		KeyPad7 = 327, // GLFW_KEY_KP_7
		KeyPad8 = 328, // GLFW_KEY_KP_8
		KeyPad9 = 329, // GLFW_KEY_KP_9
		KeyPadDecimal = 330, // GLFW_KEY_KP_DECIMAL
		KeyPadDivide = 331, // GLFW_KEY_KP_DIVIDE
		KeyPadMultiply = 332, // GLFW_KEY_KP_MULTIPLY
		KeyPadSubtract = 333, // GLFW_KEY_KP_SUBTRACT
		KeyPadAdd = 334, // GLFW_KEY_KP_ADD
		KeyPadEnter = 335, // GLFW_KEY_KP_ENTER
		KeyPadEqual = 336, // GLFW_KEY_KP_EQUAL
		KeyLeftShift = 340, // GLFW_KEY_LEFT_SHIFT
		KeyLeftControl = 341, // GLFW_KEY_LEFT_CONTROL
		KeyLeftAlt = 342, // GLFW_KEY_LEFT_ALT
		KeyLeftSuper = 343, // GLFW_KEY_LEFT_SUPER
		KeyRightShift = 344, // GLFW_KEY_RIGHT_SHIFT
		KeyRightControl = 345, // GLFW_KEY_RIGHT_CONTROL
		KeyRightAlt = 346, // GLFW_KEY_RIGHT_ALT
		KeyRightSuper = 347, // GLFW_KEY_RIGHT_SUPER
		KeyMenu = 348 // GLFW_KEY_MENU
	};

	static constexpr auto KeyUnknownString = "KeyUnknown";
	static constexpr auto KeySpaceString = "KeySpace";
	static constexpr auto KeyApostropheString = "KeyApostrophe";
	static constexpr auto KeyCommaString = "KeyComma";
	static constexpr auto KeyMinusString = "KeyMinus";
	static constexpr auto KeyPeriodString = "KeyPeriod";
	static constexpr auto KeySlashString = "KeySlash";
	static constexpr auto Key0String = "Key0";
	static constexpr auto Key1String = "Key1";
	static constexpr auto Key2String = "Key2";
	static constexpr auto Key3String = "Key3";
	static constexpr auto Key4String = "Key4";
	static constexpr auto Key5String = "Key5";
	static constexpr auto Key6String = "Key6";
	static constexpr auto Key7String = "Key7";
	static constexpr auto Key8String = "Key8";
	static constexpr auto Key9String = "Key9";
	static constexpr auto KeySemicolonString = "KeySemicolon";
	static constexpr auto KeyEqualString = "KeyEqual";
	static constexpr auto KeyAString = "KeyA";
	static constexpr auto KeyBString = "KeyB";
	static constexpr auto KeyCString = "KeyC";
	static constexpr auto KeyDString = "KeyD";
	static constexpr auto KeyEString = "KeyE";
	static constexpr auto KeyFString = "KeyF";
	static constexpr auto KeyGString = "KeyG";
	static constexpr auto KeyHString = "KeyH";
	static constexpr auto KeyIString = "KeyI";
	static constexpr auto KeyJString = "KeyJ";
	static constexpr auto KeyKString = "KeyK";
	static constexpr auto KeyLString = "KeyL";
	static constexpr auto KeyMString = "KeyM";
	static constexpr auto KeyNString = "KeyN";
	static constexpr auto KeyOString = "KeyO";
	static constexpr auto KeyPString = "KeyP";
	static constexpr auto KeyQString = "KeyQ";
	static constexpr auto KeyRString = "KeyR";
	static constexpr auto KeySString = "KeyS";
	static constexpr auto KeyTString = "KeyT";
	static constexpr auto KeyUString = "KeyU";
	static constexpr auto KeyVString = "KeyV";
	static constexpr auto KeyWString = "KeyW";
	static constexpr auto KeyXString = "KeyX";
	static constexpr auto KeyYString = "KeyY";
	static constexpr auto KeyZString = "KeyZ";
	static constexpr auto KeyLeftBracketString = "KeyLeftBracket";
	static constexpr auto KeyBackSlashString = "KeyBackSlash";
	static constexpr auto KeyRightBracketString = "KeyRightBracket";
	static constexpr auto KeyGraveAccentString = "KeyGraveAccent";
	static constexpr auto KeyWorld1String = "KeyWorld1";
	static constexpr auto KeyWorld2String = "KeyWorld2";
	static constexpr auto KeyEscapeString = "KeyEscape";
	static constexpr auto KeyEnterString = "KeyEnter";
	static constexpr auto KeyTabString = "KeyTab";
	static constexpr auto KeyBackspaceString = "KeyBackspace";
	static constexpr auto KeyInsertString = "KeyInsert";
	static constexpr auto KeyDeleteString = "KeyDelete";
	static constexpr auto KeyRightString = "KeyRight";
	static constexpr auto KeyLeftString = "KeyLeft";
	static constexpr auto KeyDownString = "KeyDown";
	static constexpr auto KeyUpString = "KeyUp";
	static constexpr auto KeyPageUpString = "KeyPageUp";
	static constexpr auto KeyPageDownString = "KeyPageDown";
	static constexpr auto KeyHomeString = "KeyHome";
	static constexpr auto KeyEndString = "KeyEnd";
	static constexpr auto KeyCapsLockString = "KeyCapsLock";
	static constexpr auto KeyScrollLockString = "KeyScrollLock";
	static constexpr auto KeyNumLockString = "KeyNumLock";
	static constexpr auto KeyPrintScreenString = "KeyPrintScreen";
	static constexpr auto KeyPauseString = "KeyPause";
	static constexpr auto KeyF1String = "KeyF1";
	static constexpr auto KeyF2String = "KeyF2";
	static constexpr auto KeyF3String = "KeyF3";
	static constexpr auto KeyF4String = "KeyF4";
	static constexpr auto KeyF5String = "KeyF5";
	static constexpr auto KeyF6String = "KeyF6";
	static constexpr auto KeyF7String = "KeyF7";
	static constexpr auto KeyF8String = "KeyF8";
	static constexpr auto KeyF9String = "KeyF9";
	static constexpr auto KeyF10String = "KeyF10";
	static constexpr auto KeyF11String = "KeyF11";
	static constexpr auto KeyF12String = "KeyF12";
	static constexpr auto KeyF13String = "KeyF13";
	static constexpr auto KeyF14String = "KeyF14";
	static constexpr auto KeyF15String = "KeyF15";
	static constexpr auto KeyF16String = "KeyF16";
	static constexpr auto KeyF17String = "KeyF17";
	static constexpr auto KeyF18String = "KeyF18";
	static constexpr auto KeyF19String = "KeyF19";
	static constexpr auto KeyF20String = "KeyF20";
	static constexpr auto KeyF21String = "KeyF21";
	static constexpr auto KeyF22String = "KeyF22";
	static constexpr auto KeyF23String = "KeyF23";
	static constexpr auto KeyF24String = "KeyF24";
	static constexpr auto KeyF25String = "KeyF25";
	static constexpr auto KeyPad0String = "KeyPad0";
	static constexpr auto KeyPad1String = "KeyPad1";
	static constexpr auto KeyPad2String = "KeyPad2";
	static constexpr auto KeyPad3String = "KeyPad3";
	static constexpr auto KeyPad4String = "KeyPad4";
	static constexpr auto KeyPad5String = "KeyPad5";
	static constexpr auto KeyPad6String = "KeyPad6";
	static constexpr auto KeyPad7String = "KeyPad7";
	static constexpr auto KeyPad8String = "KeyPad8";
	static constexpr auto KeyPad9String = "KeyPad9";
	static constexpr auto KeyPadDecimalString = "KeyPadDecimal";
	static constexpr auto KeyPadDivideString = "KeyPadDivide";
	static constexpr auto KeyPadMultiplyString = "KeyPadMultiply";
	static constexpr auto KeyPadSubtractString = "KeyPadSubtract";
	static constexpr auto KeyPadAddString = "KeyPadAdd";
	static constexpr auto KeyPadEnterString = "KeyPadEnter";
	static constexpr auto KeyPadEqualString = "KeyPadEqual";
	static constexpr auto KeyLeftShiftString = "KeyLeftShift";
	static constexpr auto KeyLeftControlString = "KeyLeftControl";
	static constexpr auto KeyLeftAltString = "KeyLeftAlt";
	static constexpr auto KeyLeftSuperString = "KeyLeftSuper";
	static constexpr auto KeyRightShiftString = "KeyRightShift";
	static constexpr auto KeyRightControlString = "KeyRightControl";
	static constexpr auto KeyRightAltString = "KeyRightAlt";
	static constexpr auto KeyRightSuperString = "KeyRightSuper";
	static constexpr auto KeyMenuString = "KeyMenu";

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (Key type) noexcept;

	[[nodiscard]]
	inline
	std::string
	to_string (Key type) noexcept
	{
		return {to_cstring(type)};
	}

	[[nodiscard]]
	Key convertKey (const std::string & type) noexcept;

	enum ModKey
	{
		ModKeyUnknown = 0x0000,
		ModKeyShift = 0x0001, // GLFW_MOD_SHIFT
		ModKeyControl = 0x0002, // GLFW_MOD_CONTROL
		ModKeyAlt = 0x0004, // GLFW_MOD_ALT
		ModKeySuper = 0x0008 // GLFW_MOD_SUPER
	};

	static constexpr auto ModKeyUnknownString = "ModKeyUnknown";
	static constexpr auto ModKeyShiftString = "ModKeyShift";
	static constexpr auto ModKeyControlString = "ModKeyControl";
	static constexpr auto ModKeyAltString = "ModKeyAlt";
	static constexpr auto ModKeySuperString = "ModKeySuper";

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (ModKey type) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (ModKey type) noexcept
	{
		return {to_cstring(type)};
	}

	[[nodiscard]]
	ModKey convertModKey (const std::string & type) noexcept;

	/**
	 * @brief Converts GLFW key to standard ASCII keycode.
	 * @param key The GLFW key token.
	 * @return int
	 */
	[[nodiscard]]
	uint8_t ASCII (int key) noexcept;
}
