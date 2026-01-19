/*
 * src/PlatformSpecific/Helpers.linux.cpp
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

/* Local inclusions. */
#include "Helpers.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <cstdio>

namespace EmEn::PlatformSpecific
{
	bool
	checkProgram (const std::string & program) noexcept
	{
		const std::string command = "which " + program + " > /dev/null 2>&1";

		return system(command.c_str()) == 0;
	}

	bool
	hasZenity () noexcept
	{
		static const bool result = checkProgram("zenity");

		return result;
	}

	bool
	hasKdialog () noexcept
	{
		static const bool result = checkProgram("kdialog");

		return result;
	}

	bool
	isKdeDesktop () noexcept
	{
		if ( const char * desktop = std::getenv("XDG_CURRENT_DESKTOP"); desktop != nullptr )
		{
			const std::string desktopStr{desktop};

			return desktopStr.find("KDE") != std::string::npos;
		}

		return false;
	}

	std::string
	escapeShellArg (const std::string & arg) noexcept
	{
		std::string escaped;
		escaped.reserve(arg.size() + 2);
		escaped += '\'';

		for ( const char c : arg )
		{
			if ( c == '\'' )
			{
				/* End quote, add escaped quote, restart quote. */
				escaped += "'\\''";
			}
			else
			{
				escaped += c;
			}
		}

		escaped += '\'';

		return escaped;
	}

	std::string
	executeCommand (const std::string & command, int & exitCode) noexcept
	{
		std::string output;

		FILE * pipe = popen(command.c_str(), "r");

		if ( pipe == nullptr )
		{
			exitCode = -1;

			return output;
		}

		char buffer[4096];

		while ( fgets(buffer, sizeof(buffer), pipe) != nullptr )
		{
			output += buffer;
		}

		const int status = pclose(pipe);
		exitCode = WEXITSTATUS(status);

		/* Remove trailing newline if any. */
		while ( !output.empty() && output.back() == '\n' )
		{
			output.pop_back();
		}

		return output;
	}

	std::string
	buildZenityFilters (const ExtensionFilters & filters) noexcept
	{
		std::string result;

		for ( const auto & [filterName, extensions] : filters )
		{
			std::string filterArg = filterName;
			filterArg += '|';

			for ( auto it = extensions.cbegin(); it != extensions.cend(); ++it )
			{
				filterArg += "*.";
				filterArg += *it;

				if ( std::next(it) != extensions.cend() )
				{
					filterArg += ' ';
				}
			}

			result += " --file-filter=";
			result += escapeShellArg(filterArg);
		}

		return result;
	}

	std::string
	buildKdialogFilters (const ExtensionFilters & filters) noexcept
	{
		std::string result;

		for ( auto it = filters.cbegin(); it != filters.cend(); ++it )
		{
			const auto & [filterName, extensions] = *it;

			if ( it != filters.cbegin() )
			{
				result += " | ";
			}

			result += filterName;
			result += '(';

			for ( auto extIt = extensions.cbegin(); extIt != extensions.cend(); ++extIt )
			{
				result += "*.";
				result += *extIt;

				if ( std::next(extIt) != extensions.cend() )
				{
					result += ' ';
				}
			}

			result += ')';
		}

		return result;
	}
}