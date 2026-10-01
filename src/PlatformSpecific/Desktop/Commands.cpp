/*
 * src/PlatformSpecific/Desktop/Commands.cpp
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

#include "Commands.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <string_view>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "Network/URL.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::PlatformSpecific::Desktop
{
	using namespace Base::Network;

	constexpr auto TracerTag{"DesktopCommand"};

	ProgressMode
	to_ProgressMode (const std::string & string) noexcept
	{
		if ( string == "none")
		{
			return ProgressMode::None;
		}

		if ( string == "normal")
		{
			return ProgressMode::Normal;
		}

		if ( string == "indeterminate")
		{
			return ProgressMode::Indeterminate;
		}

		if ( string == "error")
		{
			return ProgressMode::Error;
		}

		if ( string == "paused" )
		{
			return ProgressMode::Paused;
		}

		return ProgressMode::Normal;
	}

	bool
	openURL (const std::string & url) noexcept
	{
		if ( !URL::isURL(url) )
		{
			TraceWarning{TracerTag} << "The URL '" << url << "' is invalid !";

			return false;
		}

		/* NOTE: Web links only (owner ruling 2026-10-01): the string goes to the system as is (ShellExecuteW, open,
		 * xdg-open), which launches any registered protocol handler — file://host/share/x.exe, the Windows ms-*
		 * handlers. The raw string is tested, since it is the raw string that is handed over. */
		const auto hasScheme = [&url] (std::string_view scheme) {
			return url.size() > scheme.size() && std::equal(scheme.begin(), scheme.end(), url.begin(), [] (char expected, char actual) {
				return expected == static_cast< char >(std::tolower(static_cast< unsigned char >(actual)));
			});
		};

		if ( !hasScheme("http://") && !hasScheme("https://") )
		{
			TraceWarning{TracerTag} << "The URL '" << url << "' is refused: only http:// and https:// links are opened !";

			return false;
		}

		return runDefaultDesktopApplication(url);
	}

	bool
	openFile (const std::filesystem::path & filepath) noexcept
	{
		if ( !Base::IO::fileExists(filepath) )
		{
			TraceWarning{TracerTag} << "The file '" << Base::IO::toU8String(filepath) << "' does not exist !";

			return false;
		}

		return runDefaultDesktopApplication(Base::IO::toU8String(filepath));
	}

	bool
	openTextFile (Settings & settings, const std::filesystem::path & filepath) noexcept
	{
		if ( !Base::IO::fileExists(filepath) )
		{
			TraceWarning{TracerTag} << "The file '" << Base::IO::toU8String(filepath) << "' does not exist !";

			return false;
		}

		const auto textEditor = settings.getOrSetDefault< std::string >(TextEditorKey, DefaultTextEditor);

		return runDesktopApplication(textEditor, Base::IO::toU8String(filepath));
	}

	bool
	openFolder (const std::filesystem::path & filepath) noexcept
	{
		if ( !Base::IO::directoryExists(filepath) )
		{
			TraceWarning{TracerTag} << "The file '" << Base::IO::toU8String(filepath) << "' does not exist !";

			return false;
		}

		return runDefaultDesktopApplication(Base::IO::toU8String(filepath));
	}

	bool
	showInFolder (const std::filesystem::path & filepath) noexcept
	{
		if ( !Base::IO::fileExists(filepath) )
		{
			TraceWarning{TracerTag} << "The file '" << Base::IO::toU8String(filepath) << "' does not exist !";

			return false;
		}

		return runDefaultDesktopApplication(Base::IO::toU8String(filepath.parent_path()));
	}
}
