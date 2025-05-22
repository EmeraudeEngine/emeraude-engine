/*
 * src/PlatformSpecific/UserInfo.linux.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "UserInfo.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_LINUX

/* STL inclusions. */
#include <iostream>

/* System inclusions. */
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

	UserInfo::UserInfo () noexcept
	{
		passwd userData{};
		std::string buffer;
		passwd * result{nullptr};

		resizeBuffer(buffer);

		if ( getpwuid_r(getuid(), &userData, buffer.data(), buffer.size(), &result) > 0 || result == nullptr )
		{
			return;
		}

		m_name = userData.pw_gecos;
		m_accountName = userData.pw_name;
		m_homePath = userData.pw_dir;

		if ( m_name.empty() )
		{
			std::cerr << "Warning: Unable to get the username !" "\n";
		}

		if ( m_accountName.empty() )
		{
			std::cerr << "Error: Unable to get the account name !" "\n";

			return;
		}

		if ( m_homePath.empty() )
		{
			std::cerr << "Warning: Unable to get the home directory !" "\n";

			return;
		}

		m_informationFound = true;
	}
}

#endif
