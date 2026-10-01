/*
 * src/PlatformSpecific/StringConversion.mac.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* NOTE: Objective-C++ only (the engine's .mm files, compiled with ARC). */
#ifndef __OBJC__
#error "StringConversion.mac.hpp is for Objective-C++ sources (.mm) only."
#endif

/* STL inclusions. */
#include <string>

/* Third-party inclusions. */
#import <Foundation/Foundation.h>

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief Returns the UTF-8 copy of an NSString, empty for nil or a string without a UTF-8 form.
	 * @note -UTF8String may return NULL, and a NULL `const char *` into std::string is UB (triad 14, 2026-10-01).
	 * @param string The string, or nil.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	toStdString (NSString * string) noexcept
	{
		const char * utf8 = string != nil ? [string UTF8String] : nullptr;

		return utf8 != nullptr ? std::string{utf8} : std::string{};
	}

	/**
	 * @brief Returns an NSString from a UTF-8 string, never nil (an empty string for invalid UTF-8).
	 * @note -stringWithUTF8String: and @(…) return nil on invalid UTF-8, and inserting nil into a collection (or
	 * -fileURLWithPath:nil) raises an Objective-C exception: an abort (triad 14, 2026-10-01).
	 * @param string The UTF-8 string.
	 * @return NSString *
	 */
	[[nodiscard]]
	inline
	NSString *
	toNSString (const std::string & string) noexcept
	{
		NSString * converted = [NSString stringWithUTF8String:string.c_str()];

		return converted != nil ? converted : @"";
	}
}
