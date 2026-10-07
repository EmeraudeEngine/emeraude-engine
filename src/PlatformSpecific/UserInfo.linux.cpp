/*
 * src/PlatformSpecific/UserInfo.linux.cpp
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
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

/* Third-party inclusions. */
#include <pwd.h>
#include <unistd.h>

namespace EmEn::PlatformSpecific
{
	namespace
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
	}

	namespace
	{
		/**
		 * @brief Returns the interface language from the environment, as a BCP 47 tag.
		 * @note gettext's order: LANGUAGE (a ':' list, its first entry), LC_ALL, LC_MESSAGES, LANG. "fr_BE.UTF-8@euro"
		 * becomes "fr-BE"; "C" and "POSIX" say nothing.
		 * @return std::string Empty when unknown.
		 */
		[[nodiscard]]
		std::string
		languageFromEnvironment () noexcept
		{
			for ( const auto * name : {"LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"} )
			{
				const auto * value = std::getenv(name);

				if ( value == nullptr || *value == '\0' )
				{
					continue;
				}

				std::string tag{value};

				if ( const auto end = tag.find_first_of(":.@"); end != std::string::npos )
				{
					tag.resize(end);
				}

				if ( tag.empty() || tag == "C" || tag == "POSIX" )
				{
					continue;
				}

				std::ranges::replace(tag, '_', '-');

				return tag;
			}

			return {};
		}
	}

	bool
	UserInfo::onInitialize () noexcept
	{
		m_preferredLanguage = languageFromEnvironment();

		passwd userData{};
		std::string buffer;
		passwd * result{nullptr};

		resizeBuffer(buffer);

		if ( getpwuid_r(getuid(), &userData, buffer.data(), buffer.size(), &result) > 0 || result == nullptr )
		{
			return false;
		}

		/* NOTE: A field the system leaves NULL is an empty string (a NULL `const char *` into std::string is UB). */
		m_username = userData.pw_gecos != nullptr ? userData.pw_gecos : "";
		m_accountName = userData.pw_name != nullptr ? userData.pw_name : "";
		m_homePath = userData.pw_dir != nullptr ? userData.pw_dir : "";

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
