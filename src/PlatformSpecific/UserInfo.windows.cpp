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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "UserInfo.hpp"

/* STL inclusions. */
#include <array>

/* Third-party inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Lmcons.h>
#define SECURITY_WIN32
#include <Security.h>
#include <Shlobj.h>

/* Local inclusions. */
#include "Helpers.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific
{
	bool
	UserInfo::onInitialize () noexcept
	{
		/* NOTE: The first of the user's interface languages ("fr-BE"), else the user's locale. A NUL-separated list
		 * ending with two NULs: the std::wstring built from its start stops at the first entry. */
		{
			ULONG languageCount = 0;
			ULONG bufferSize = 0;

			if ( GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &languageCount, nullptr, &bufferSize) != 0 && bufferSize > 0 )
			{
				std::wstring languages(bufferSize, L'\0');

				if ( GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &languageCount, languages.data(), &bufferSize) != 0 && languageCount > 0 )
				{
					m_preferredLanguage = convertWideToUTF8(std::wstring{languages.c_str()});
				}
			}

			if ( m_preferredLanguage.empty() )
			{
				std::array< wchar_t, LOCALE_NAME_MAX_LENGTH > localeName{};

				if ( GetUserDefaultLocaleName(localeName.data(), static_cast< int >(localeName.size())) > 0 )
				{
					m_preferredLanguage = convertWideToUTF8(std::wstring{localeName.data()});
				}
			}
		}

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
				/* NOTE: The documentation asks for CoTaskMemFree() whether the call succeeds or not
				 * (CoTaskMemFree(nullptr) is a no-op). */
				CoTaskMemFree(path);

				TraceError{ClassId} << "Unable to get the home directory!";

				return false;
			}

			m_homePath.assign(path);

			CoTaskMemFree(path);
		}

		return true;
	}
}
