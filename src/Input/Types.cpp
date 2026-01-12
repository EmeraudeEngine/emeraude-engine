/*
 * src/Input/Types.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Types.hpp"

/* STL inclusions. */
#include <cstdint>
#include <sstream>
#include <string>

namespace EmEn::Input
{
	const char *
	to_cstring (Key key) noexcept
	{
		switch ( key )
		{
			case KeyUnknown : return KeyUnknownString;
			case KeySpace : return KeySpaceString;
			case KeyApostrophe : return KeyApostropheString;
			case KeyComma : return KeyCommaString;
			case KeyMinus : return KeyMinusString;
			case KeyPeriod : return KeyPeriodString;
			case KeySlash : return KeySlashString;
			case Key0 : return Key0String;
			case Key1 : return Key1String;
			case Key2 : return Key2String;
			case Key3 : return Key3String;
			case Key4 : return Key4String;
			case Key5 : return Key5String;
			case Key6 : return Key6String;
			case Key7 : return Key7String;
			case Key8 : return Key8String;
			case Key9 : return Key9String;
			case KeySemicolon : return KeySemicolonString;
			case KeyEqual : return KeyEqualString;
			case KeyA : return KeyAString;
			case KeyB : return KeyBString;
			case KeyC : return KeyCString;
			case KeyD : return KeyDString;
			case KeyE : return KeyEString;
			case KeyF : return KeyFString;
			case KeyG : return KeyGString;
			case KeyH : return KeyHString;
			case KeyI : return KeyIString;
			case KeyJ : return KeyJString;
			case KeyK : return KeyKString;
			case KeyL : return KeyLString;
			case KeyM : return KeyMString;
			case KeyN : return KeyNString;
			case KeyO : return KeyOString;
			case KeyP : return KeyPString;
			case KeyQ : return KeyQString;
			case KeyR : return KeyRString;
			case KeyS : return KeySString;
			case KeyT : return KeyTString;
			case KeyU : return KeyUString;
			case KeyV : return KeyVString;
			case KeyW : return KeyWString;
			case KeyX : return KeyXString;
			case KeyY : return KeyYString;
			case KeyZ : return KeyZString;
			case KeyLeftBracket : return KeyLeftBracketString;
			case KeyBackSlash : return KeyBackSlashString;
			case KeyRightBracket : return KeyRightBracketString;
			case KeyGraveAccent : return KeyGraveAccentString;
			case KeyWorld1 : return KeyWorld1String;
			case KeyWorld2 : return KeyWorld2String;
			case KeyEscape : return KeyEscapeString;
			case KeyEnter : return KeyEnterString;
			case KeyTab : return KeyTabString;
			case KeyBackspace : return KeyBackspaceString;
			case KeyInsert : return KeyInsertString;
			case KeyDelete : return KeyDeleteString;
			case KeyRight : return KeyRightString;
			case KeyLeft : return KeyLeftString;
			case KeyDown : return KeyDownString;
			case KeyUp : return KeyUpString;
			case KeyPageUp : return KeyPageUpString;
			case KeyPageDown : return KeyPageDownString;
			case KeyHome : return KeyHomeString;
			case KeyEnd : return KeyEndString;
			case KeyCapsLock : return KeyCapsLockString;
			case KeyScrollLock : return KeyScrollLockString;
			case KeyNumLock : return KeyNumLockString;
			case KeyPrintScreen : return KeyPrintScreenString;
			case KeyPause : return KeyPauseString;
			case KeyF1 : return KeyF1String;
			case KeyF2 : return KeyF2String;
			case KeyF3 : return KeyF3String;
			case KeyF4 : return KeyF4String;
			case KeyF5 : return KeyF5String;
			case KeyF6 : return KeyF6String;
			case KeyF7 : return KeyF7String;
			case KeyF8 : return KeyF8String;
			case KeyF9 : return KeyF9String;
			case KeyF10 : return KeyF10String;
			case KeyF11 : return KeyF11String;
			case KeyF12 : return KeyF12String;
			case KeyF13 : return KeyF13String;
			case KeyF14 : return KeyF14String;
			case KeyF15 : return KeyF15String;
			case KeyF16 : return KeyF16String;
			case KeyF17 : return KeyF17String;
			case KeyF18 : return KeyF18String;
			case KeyF19 : return KeyF19String;
			case KeyF20 : return KeyF20String;
			case KeyF21 : return KeyF21String;
			case KeyF22 : return KeyF22String;
			case KeyF23 : return KeyF23String;
			case KeyF24 : return KeyF24String;
			case KeyF25 : return KeyF25String;
			case KeyPad0 : return KeyPad0String;
			case KeyPad1 : return KeyPad1String;
			case KeyPad2 : return KeyPad2String;
			case KeyPad3 : return KeyPad3String;
			case KeyPad4 : return KeyPad4String;
			case KeyPad5 : return KeyPad5String;
			case KeyPad6 : return KeyPad6String;
			case KeyPad7 : return KeyPad7String;
			case KeyPad8 : return KeyPad8String;
			case KeyPad9 : return KeyPad9String;
			case KeyPadDecimal : return KeyPadDecimalString;
			case KeyPadDivide : return KeyPadDivideString;
			case KeyPadMultiply : return KeyPadMultiplyString;
			case KeyPadSubtract : return KeyPadSubtractString;
			case KeyPadAdd : return KeyPadAddString;
			case KeyPadEnter : return KeyPadEnterString;
			case KeyPadEqual : return KeyPadEqualString;
			case KeyLeftShift : return KeyLeftShiftString;
			case KeyLeftControl : return KeyLeftControlString;
			case KeyLeftAlt : return KeyLeftAltString;
			case KeyLeftSuper : return KeyLeftSuperString;
			case KeyRightShift : return KeyRightShiftString;
			case KeyRightControl : return KeyRightControlString;
			case KeyRightAlt : return KeyRightAltString;
			case KeyRightSuper : return KeyRightSuperString;
			case KeyMenu : return KeyMenuString;
			default : return nullptr;
		}
	}

	Key
	convertKey (const std::string & type) noexcept
	{
		if ( type == KeySpaceString ) { return KeySpace; }
		if ( type == KeyApostropheString ) { return KeyApostrophe; }
		if ( type == KeyCommaString ) { return KeyComma; }
		if ( type == KeyMinusString ) { return KeyMinus; }
		if ( type == KeyPeriodString ) { return KeyPeriod; }
		if ( type == KeySlashString ) { return KeySlash; }
		if ( type == Key0String ) { return Key0; }
		if ( type == Key1String ) { return Key1; }
		if ( type == Key2String ) { return Key2; }
		if ( type == Key3String ) { return Key3; }
		if ( type == Key4String ) { return Key4; }
		if ( type == Key5String ) { return Key5; }
		if ( type == Key6String ) { return Key6; }
		if ( type == Key7String ) { return Key7; }
		if ( type == Key8String ) { return Key8; }
		if ( type == Key9String ) { return Key9; }
		if ( type == KeySemicolonString ) { return KeySemicolon; }
		if ( type == KeyEqualString ) { return KeyEqual; }
		if ( type == KeyAString ) { return KeyA; }
		if ( type == KeyBString ) { return KeyB; }
		if ( type == KeyCString ) { return KeyC; }
		if ( type == KeyDString ) { return KeyD; }
		if ( type == KeyEString ) { return KeyE; }
		if ( type == KeyFString ) { return KeyF; }
		if ( type == KeyGString ) { return KeyG; }
		if ( type == KeyHString ) { return KeyH; }
		if ( type == KeyIString ) { return KeyI; }
		if ( type == KeyJString ) { return KeyJ; }
		if ( type == KeyKString ) { return KeyK; }
		if ( type == KeyLString ) { return KeyL; }
		if ( type == KeyMString ) { return KeyM; }
		if ( type == KeyNString ) { return KeyN; }
		if ( type == KeyOString ) { return KeyO; }
		if ( type == KeyPString ) { return KeyP; }
		if ( type == KeyQString ) { return KeyQ; }
		if ( type == KeyRString ) { return KeyR; }
		if ( type == KeySString ) { return KeyS; }
		if ( type == KeyTString ) { return KeyT; }
		if ( type == KeyUString ) { return KeyU; }
		if ( type == KeyVString ) { return KeyV; }
		if ( type == KeyWString ) { return KeyW; }
		if ( type == KeyXString ) { return KeyX; }
		if ( type == KeyYString ) { return KeyY; }
		if ( type == KeyZString ) { return KeyZ; }
		if ( type == KeyLeftBracketString ) { return KeyLeftBracket; }
		if ( type == KeyBackSlashString ) { return KeyBackSlash; }
		if ( type == KeyRightBracketString ) { return KeyRightBracket; }
		if ( type == KeyGraveAccentString ) { return KeyGraveAccent; }
		if ( type == KeyWorld1String ) { return KeyWorld1; }
		if ( type == KeyWorld2String ) { return KeyWorld2; }
		if ( type == KeyEscapeString ) { return KeyEscape; }
		if ( type == KeyEnterString ) { return KeyEnter; }
		if ( type == KeyTabString ) { return KeyTab; }
		if ( type == KeyBackspaceString ) { return KeyBackspace; }
		if ( type == KeyInsertString ) { return KeyInsert; }
		if ( type == KeyDeleteString ) { return KeyDelete; }
		if ( type == KeyRightString ) { return KeyRight; }
		if ( type == KeyLeftString ) { return KeyLeft; }
		if ( type == KeyDownString ) { return KeyDown; }
		if ( type == KeyUpString ) { return KeyUp; }
		if ( type == KeyPageUpString ) { return KeyPageUp; }
		if ( type == KeyPageDownString ) { return KeyPageDown; }
		if ( type == KeyHomeString ) { return KeyHome; }
		if ( type == KeyEndString ) { return KeyEnd; }
		if ( type == KeyCapsLockString ) { return KeyCapsLock; }
		if ( type == KeyScrollLockString ) { return KeyScrollLock; }
		if ( type == KeyNumLockString ) { return KeyNumLock; }
		if ( type == KeyPrintScreenString ) { return KeyPrintScreen; }
		if ( type == KeyPauseString ) { return KeyPause; }
		if ( type == KeyF1String ) { return KeyF1; }
		if ( type == KeyF2String ) { return KeyF2; }
		if ( type == KeyF3String ) { return KeyF3; }
		if ( type == KeyF4String ) { return KeyF4; }
		if ( type == KeyF5String ) { return KeyF5; }
		if ( type == KeyF6String ) { return KeyF6; }
		if ( type == KeyF7String ) { return KeyF7; }
		if ( type == KeyF8String ) { return KeyF8; }
		if ( type == KeyF9String ) { return KeyF9; }
		if ( type == KeyF10String ) { return KeyF10; }
		if ( type == KeyF11String ) { return KeyF11; }
		if ( type == KeyF12String ) { return KeyF12; }
		if ( type == KeyF13String ) { return KeyF13; }
		if ( type == KeyF14String ) { return KeyF14; }
		if ( type == KeyF15String ) { return KeyF15; }
		if ( type == KeyF16String ) { return KeyF16; }
		if ( type == KeyF17String ) { return KeyF17; }
		if ( type == KeyF18String ) { return KeyF18; }
		if ( type == KeyF19String ) { return KeyF19; }
		if ( type == KeyF20String ) { return KeyF20; }
		if ( type == KeyF21String ) { return KeyF21; }
		if ( type == KeyF22String ) { return KeyF22; }
		if ( type == KeyF23String ) { return KeyF23; }
		if ( type == KeyF24String ) { return KeyF24; }
		if ( type == KeyF25String ) { return KeyF25; }
		if ( type == KeyPad0String ) { return KeyPad0; }
		if ( type == KeyPad1String ) { return KeyPad1; }
		if ( type == KeyPad2String ) { return KeyPad2; }
		if ( type == KeyPad3String ) { return KeyPad3; }
		if ( type == KeyPad4String ) { return KeyPad4; }
		if ( type == KeyPad5String ) { return KeyPad5; }
		if ( type == KeyPad6String ) { return KeyPad6; }
		if ( type == KeyPad7String ) { return KeyPad7; }
		if ( type == KeyPad8String ) { return KeyPad8; }
		if ( type == KeyPad9String ) { return KeyPad9; }
		if ( type == KeyPadDecimalString ) { return KeyPadDecimal; }
		if ( type == KeyPadDivideString ) { return KeyPadDivide; }
		if ( type == KeyPadMultiplyString ) { return KeyPadMultiply; }
		if ( type == KeyPadSubtractString ) { return KeyPadSubtract; }
		if ( type == KeyPadAddString ) { return KeyPadAdd; }
		if ( type == KeyPadEnterString ) { return KeyPadEnter; }
		if ( type == KeyPadEqualString ) { return KeyPadEqual; }
		if ( type == KeyLeftShiftString ) { return KeyLeftShift; }
		if ( type == KeyLeftControlString ) { return KeyLeftControl; }
		if ( type == KeyLeftAltString ) { return KeyLeftAlt; }
		if ( type == KeyLeftSuperString ) { return KeyLeftSuper; }
		if ( type == KeyRightShiftString ) { return KeyRightShift; }
		if ( type == KeyRightControlString ) { return KeyRightControl; }
		if ( type == KeyRightAltString ) { return KeyRightAlt; }
		if ( type == KeyRightSuperString ) { return KeyRightSuper; }
		if ( type == KeyMenuString ) { return KeyMenu; }

		return KeyUnknown;
	}

	const char *
	to_cstring (ModKey type) noexcept
	{
		switch ( type )
		{
			case ModKeyUnknown :
				return ModKeyUnknownString;

			case ModKeyShift :
				return ModKeyShiftString;

			case ModKeyControl :
				return ModKeyControlString;

			case ModKeyAlt :
				return ModKeyAltString;

			case ModKeySuper :
				return ModKeySuperString;

			case ModKeyCapsLock :
				return ModKeyCapsLockString;

			case ModKeyNumLock :
				return ModKeyNumLockString;

			default:
				return nullptr;
		}
	}

	ModKey
	convertModKey (const std::string & type) noexcept
	{
		if ( type == ModKeyShiftString ) {return ModKeyShift;}
		if ( type == ModKeyControlString ) {return ModKeyControl;}
		if ( type == ModKeyAltString ) {return ModKeyAlt;}
		if ( type == ModKeySuperString ) {return ModKeySuper;}
		if ( type == ModKeyCapsLockString ) {return ModKeyCapsLock;}
		if ( type == ModKeyNumLockString ) {return ModKeyNumLock;}

		return ModKeyUnknown;
	}

	uint8_t
	ASCII (int key) noexcept
	{
		switch ( key )
		{
			case KeyBackspace : return 8;
			case KeyTab : return 9;
			case KeyEnter : return 13;
			case KeyLeftShift : // fallthrough
			case KeyRightShift : return 16;
			case KeyLeftControl : // fallthrough
			case KeyRightControl : return 17;
			case KeyLeftAlt : return 18;
			case KeyRightAlt : return 225;
			case KeyPause : return 19;
			case KeyCapsLock : return 20;
			case KeyEscape : return 27;
			case KeySpace : return 32;
			case KeyPageUp : return 33;
			case KeyPageDown : return 34;
			case KeyEnd : return 35;
			case KeyHome : return 36;
			case KeyLeft : return 37;
			case KeyUp : return 38;
			case KeyRight : return 39;
			case KeyDown : return 40;
			case KeyInsert : return 45;
			case KeyDelete : return 46;
			case Key0 : return 48;
			case Key1: return 49;
			case Key2: return 50;
			case Key3: return 51;
			case Key4: return 52;
			case Key5: return 53;
			case Key6: return 54;
			case Key7: return 55;
			case Key8: return 56;
			case Key9: return 57;
			case KeyA: return 65;
			case KeyB: return 66;
			case KeyC: return 67;
			case KeyD: return 68;
			case KeyE: return 69;
			case KeyF: return 70;
			case KeyG: return 71;
			case KeyH: return 72;
			case KeyI: return 73;
			case KeyJ: return 74;
			case KeyK: return 75;
			case KeyL: return 76;
			case KeyM: return 77;
			case KeyN: return 78;
			case KeyO: return 79;
			case KeyP: return 80;
			case KeyQ: return 81;
			case KeyR: return 82;
			case KeyS: return 83;
			case KeyT: return 84;
			case KeyU: return 85;
			case KeyV: return 86;
			case KeyW: return 87;
			case KeyX: return 88;
			case KeyY: return 89;
			case KeyZ: return 90;
			case KeyLeftSuper : return 91;
			case KeyRightSuper : return 92;
			case KeyMenu : return 93;
			case KeyPad0 : return 96;
			case KeyPad1 : return 97;
			case KeyPad2 : return 98;
			case KeyPad3 : return 99;
			case KeyPad4 : return 100;
			case KeyPad5 : return 01;
			case KeyPad6 : return 102;
			case KeyPad7 : return 103;
			case KeyPad8 : return 104;
			case KeyPad9 : return 105;
			case KeyPadMultiply : return 106;
			case KeyPadAdd : return 107;
			case KeyPadSubtract : return 109;
			case KeyPadDecimal : return 110;
			case KeyPadDivide : return 111;
			case KeyPadEnter : return 13;
			case KeyF1 : return 112;
			case KeyF2 : return 113;
			case KeyF3 : return 114;
			case KeyF4 : return 115;
			case KeyF5 : return 116;
			case KeyF6 : return 117;
			case KeyF7 : return 118;
			case KeyF8 : return 119;
			case KeyF9 : return 120;
			case KeyF10 : return 121;
			case KeyF11 : return 122;
			case KeyF12 : return 123;
			case KeyNumLock : return 144;
			case KeyScrollLock : return 145;
			case KeySemicolon : return 186;
			case KeyEqual : return 187;
			case KeyComma : return 188;
			case KeyMinus : return 189;
			case KeyPeriod : return 190;
			case KeySlash : return 191;
			case KeyGraveAccent : return 192;
			case KeyLeftBracket : return 219;
			case KeyBackSlash : return 220;
			case KeyRightBracket : return 221;
			case KeyApostrophe : return 222;
			default: return 0;
		}
	}

	bool
	isKeyboardModifierPressed (ModKey modKey, int modifiers) noexcept
	{
		return ( modifiers & modKey ) > 0;
	}

	std::string
	getModifierListString (int modifiers) noexcept
	{
		std::stringstream modifierString{};

		if ( isKeyboardModifierPressed(ModKeyShift, modifiers) )
		{
			modifierString << " SHIFT ";
		}

		if ( isKeyboardModifierPressed(ModKeyControl, modifiers) )
		{
			modifierString << " CTRL ";
		}

		if ( isKeyboardModifierPressed(ModKeyAlt, modifiers) )
		{
			modifierString << " ALT ";
		}

		if ( isKeyboardModifierPressed(ModKeySuper, modifiers) )
		{
			modifierString << " SUPER ";
		}

		if ( isKeyboardModifierPressed(ModKeyCapsLock, modifiers) )
		{
			modifierString << " CAPSLOCK ";
		}

		if ( isKeyboardModifierPressed(ModKeyNumLock, modifiers) )
		{
			modifierString << " NUMLOCK ";
		}

		return modifierString.str();
	}

	const char *
	to_cstring (MouseButton type) noexcept
	{
		switch ( type )
		{
			case Button1Left :
				return Button1LeftString;

			case Button2Right :
				return Button2RightString;

			case Button3Middle :
				return Button3MiddleString;

			case Button4Extra :
				return Button4ExtraString;

			case Button5Extra :
				return Button5ExtraString;

			case Button6Extra :
				return Button6ExtraString;

			case Button7Extra :
				return Button7ExtraString;

			case Button8Extra :
				return Button8ExtraString;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (JoystickAxis type) noexcept
	{
		switch ( type )
		{
			case AxisX :
				return AxisXString;

			case AxisY :
				return AxisYString;

			case AxisZ :
				return AxisZString;

			case ThrottleA :
				return ThrottleAString;

			case ThrottleB :
				return ThrottleBString;

			case ThrottleC :
				return ThrottleCString;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (JoystickHatDirection type) noexcept
	{
		switch ( type )
		{
			case Center :
				return CenterString;

			case Up :
				return UpString;

			case Right :
				return RightString;

			case UpRight :
				return UpRightString;

			case Down :
				return DownString;

			case DownRight :
				return DownRightString;

			case Left :
				return LeftString;

			case UpLeft :
				return UpLeftString;

			case DownLeft :
				return DownLeftString;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (GamepadAxis type) noexcept
	{
		switch ( type )
		{
			case AxisLeftX :
				return AxisLeftXString;

			case AxisLeftY :
				return AxisLeftYString;

			case AxisRightX :
				return AxisRightXString;

			case AxisRightY :
				return AxisRightYString;

			case AxisLeftTrigger :
				return AxisLeftTriggerString;

			case AxisRightTrigger :
				return AxisRightTriggerString;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (GamepadButton type) noexcept
	{
		switch ( type )
		{
			case ButtonA :
				return ButtonAString;

			case ButtonB :
				return ButtonBString;

			case ButtonX :
				return ButtonXString;

			case ButtonY :
				return ButtonYString;

			case ButtonLeftBumper :
				return ButtonLeftBumperString;

			case ButtonRightBumper :
				return ButtonRightBumperString;

			case ButtonBack :
				return ButtonBackString;

			case ButtonStart :
				return ButtonStartString;

			case ButtonGuide :
				return ButtonGuideString;

			case ButtonLeftThumb :
				return ButtonLeftThumbString;

			case ButtonRightThumb :
				return ButtonRightThumbString;

			case ButtonDPadUp :
				return ButtonDPadUpString;

			case ButtonDPadRight :
				return ButtonDPadRightString;

			case ButtonDPadDown :
				return ButtonDPadDownString;

			case ButtonDPadLeft :
				return ButtonDPadLeftString;

			default:
				return nullptr;
		}
	}
}
