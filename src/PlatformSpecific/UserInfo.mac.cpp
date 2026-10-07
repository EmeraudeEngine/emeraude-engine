/*
 * src/PlatformSpecific/UserInfo.mac.cpp
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

#include "UserInfo.hpp"

/* STL inclusions. */
#include <array>
#include <iostream>
#include <string>

/* Third-party inclusions. */
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h>
#include <unistd.h>

namespace EmEn::PlatformSpecific
{
	void
	resizeBuffer (std::string & buffer) noexcept
	{
		auto bufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);

		/* Value was indeterminate */
		if ( bufferSize < 0 )
		{
			/* Should be more than enough */
			bufferSize = 16384;
		}

		buffer.resize(bufferSize, '\0');
	}

	bool
	UserInfo::onInitialize () noexcept
	{
		/* NOTE: The first of the user's preferred languages, as set in System Settings ("fr-BE"). */
		if ( const auto languages = CFLocaleCopyPreferredLanguages(); languages != nullptr )
		{
			if ( CFArrayGetCount(languages) > 0 )
			{
				const auto firstLanguage = static_cast< CFStringRef >(CFArrayGetValueAtIndex(languages, 0));
				std::array< char, 128 > tag{};

				if ( firstLanguage != nullptr && CFStringGetCString(firstLanguage, tag.data(), static_cast< CFIndex >(tag.size()), kCFStringEncodingUTF8) )
				{
					m_preferredLanguage = tag.data();
				}
			}

			CFRelease(languages);
		}

		passwd userData{};
		std::string buffer;
		passwd * result{nullptr};

		resizeBuffer(buffer);

		if ( getpwuid_r(getuid(), &userData, buffer.data(), buffer.size(), &result) > 0 || result == nullptr )
		{
			return false;
		}

		m_username = userData.pw_gecos;
		m_accountName = userData.pw_name;
		m_homePath = userData.pw_dir;

		if ( m_username.empty() )
		{
			std::cerr << "Warning: Unable to get the username !" "\n";
		}

		if ( m_accountName.empty() )
		{
			std::cerr << "Error: Unable to get the account name !" "\n";

			return false;
		}

		if ( m_homePath.empty() )
		{
			std::cerr << "Warning: Unable to get the home directory !" "\n";

			return false;
		}

		return true;
	}
}
