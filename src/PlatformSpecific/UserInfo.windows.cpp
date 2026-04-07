/*
 * src/PlatformSpecific/UserInfo.windows.cpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_WINDOWS
#include "UserInfo.hpp"

/* STL inclusions. */
#include <iostream>
#include <array>

/* Third-party libraries. */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Lmcons.h>
#include <Shlobj.h>

#define SECURITY_WIN32
#include <Security.h>

/* Local inclusions. */
#include "Helpers.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific
{
	bool
	UserInfo::onInitialize () noexcept
	{
		std::array< wchar_t, UNLEN + 1 > buffer{};
		auto size = static_cast< DWORD >(buffer.size());

		if ( GetUserNameW(buffer.data(), &size) == 0 )
		{
			TraceError{ClassId} << "Unable to get the account name!";

			return false;
		}

		m_accountName = convertWideToUTF8({buffer.data(), size - 1});

		size = static_cast< DWORD >(buffer.size());

		if ( GetUserNameExW(NameDisplay, buffer.data(), &size) != 0 && size > 0 )
		{
			m_username = convertWideToUTF8({buffer.data(), size});
		}
		else
		{
			/* Fallback to account name if display name is unavailable. */
			m_username = m_accountName;
		}

		{
			PWSTR path = nullptr;

			const HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);

			if ( FAILED(hr) )
			{
				TraceError{ClassId} << "Unable to get the home directory!";

				return false;
			}

			m_homePath.assign(path);

			/* NOTE: No memory leak as a SHGetKnownFolderPath() failure guarantees there is no allocation. */
			CoTaskMemFree(path);
		}

		return true;
	}
}

#endif
