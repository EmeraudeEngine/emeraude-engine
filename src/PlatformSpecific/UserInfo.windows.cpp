/*
 * src/PlatformSpecific/UserInfo.windows.cpp
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

#if IS_WINDOWS

/* STL inclusions. */
#include <iostream>
#include <array>

/* Third-party libraries. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Lmcons.h>
#include <Shlobj.h>

/* Local inclusions. */
#include "Helpers.hpp"

namespace EmEn::PlatformSpecific
{
	bool
	getAccountName (std::string & accountName) noexcept
	{
		std::array< wchar_t, UNLEN + 1 > buffer{};
		auto size = static_cast< DWORD >(buffer.size());

		if ( GetUserNameW(buffer.data(), &size) == 0 )
		{
			return false;
		}

		accountName = convertWideToUTF8({buffer.data(), size - 1});

		return true;
	}

	bool
	getHomeDirectory (std::filesystem::path & homePath) noexcept
	{
		PWSTR path = nullptr;

		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);

		if ( FAILED(hr) )
		{
			return false;
		}

		homePath.assign(path);

		CoTaskMemFree(path);

		return true;
	}

	UserInfo::UserInfo () noexcept
	{
		if ( !getAccountName(m_accountName) )
		{
			std::cerr << "Error: Unable to get the account name !" "\n";

			return;
		}

		/* NOTE: On Windows the account name is the username. */
		m_name = m_accountName;

		if ( !getHomeDirectory(m_homePath) )
		{
			std::cerr << "Warning: Unable to get the home directory !" "\n";

			return;
		}

		m_informationFound = true;
	}
}

#endif
