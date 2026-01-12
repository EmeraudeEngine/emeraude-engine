/*
 * src/PlatformSpecific/Desktop/Dialog/OpenFile.linux.cpp
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

#include "OpenFile.hpp"

#if IS_LINUX

/* STL inclusions. */
#include <array>
#include <cstdlib>
#include <cstdio>
#include <sstream>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	namespace
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
			const char * desktop = std::getenv("XDG_CURRENT_DESKTOP");

			if ( desktop != nullptr )
			{
				std::string desktopStr{desktop};

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
			std::array< char, 4096 > buffer{};

			FILE * pipe = popen(command.c_str(), "r");

			if ( pipe == nullptr )
			{
				exitCode = -1;

				return output;
			}

			while ( fgets(buffer.data(), static_cast< int >(buffer.size()), pipe) != nullptr )
			{
				output += buffer.data();
			}

			const int status = pclose(pipe);
			exitCode = WEXITSTATUS(status);

			return output;
		}

		std::string
		buildZenityFilters (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters) noexcept
		{
			std::string result;

			for ( const auto & [filterName, extensions] : filters )
			{
				result += " --file-filter=";
				result += escapeShellArg(filterName + "|" + [&extensions]() {
					std::string exts;

					for ( auto it = extensions.cbegin(); it != extensions.cend(); ++it )
					{
						exts += "*." + *it;

						if ( std::next(it) != extensions.cend() )
						{
							exts += ' ';
						}
					}

					return exts;
				}());
			}

			return result;
		}

		std::string
		buildKdialogFilters (const std::vector< std::pair< std::string, std::vector< std::string > > > & filters) noexcept
		{
			std::string result;

			for ( auto it = filters.cbegin(); it != filters.cend(); ++it )
			{
				const auto & [filterName, extensions] = *it;

				if ( it != filters.cbegin() )
				{
					result += " | ";
				}

				result += filterName + "(";

				for ( auto extIt = extensions.cbegin(); extIt != extensions.cend(); ++extIt )
				{
					result += "*." + *extIt;

					if ( std::next(extIt) != extensions.cend() )
					{
						result += ' ';
					}
				}

				result += ")";
			}

			return result;
		}

		std::vector< std::string >
		splitLines (const std::string & output) noexcept
		{
			std::vector< std::string > result;
			std::istringstream stream(output);
			std::string line;

			while ( std::getline(stream, line) )
			{
				/* Remove trailing whitespace/newline. */
				while ( !line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ') )
				{
					line.pop_back();
				}

				if ( !line.empty() )
				{
					result.push_back(line);
				}
			}

			return result;
		}
	}

	bool
	OpenFile::execute (Window * /*window*/) noexcept
	{
		std::string command;

		/* Prefer kdialog on KDE, zenity otherwise. */
		const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());

		if ( useKdialog )
		{
			command = "kdialog";

			if ( m_selectFolder )
			{
				command += " --getexistingdirectory";
			}
			else
			{
				command += " --getopenfilename";
			}

			if ( m_multiSelect )
			{
				command += " --multiple --separate-output";
			}

			/* Default path (empty for current directory). */
			command += " ''";

			/* File filters. */
			if ( !m_selectFolder && !m_extensionFilters.empty() )
			{
				command += " ";
				command += escapeShellArg(buildKdialogFilters(m_extensionFilters));
			}

			command += " --title ";
			command += escapeShellArg(this->title());
		}
		else if ( hasZenity() )
		{
			command = "zenity --file-selection";
			command += " --title ";
			command += escapeShellArg(this->title());
			command += " --separator=$'\\n'";

			if ( m_selectFolder )
			{
				command += " --directory";
			}

			if ( m_multiSelect )
			{
				command += " --multiple";
			}

			/* File filters. */
			if ( !m_selectFolder && !m_extensionFilters.empty() )
			{
				command += buildZenityFilters(m_extensionFilters);
			}
		}
		else
		{
			/* No dialog tool available. */
			m_canceled = true;

			return false;
		}

		/* Execute the command and get the result. */
		int exitCode = 0;
		const std::string output = executeCommand(command, exitCode);

		/* Parse the result. */
		if ( exitCode != 0 || output.empty() )
		{
			m_canceled = true;
		}
		else
		{
			/* Split output by newlines for multiple selections. */
			m_filepaths = splitLines(output);

			if ( m_filepaths.empty() )
			{
				m_canceled = true;
			}
		}

		return true;
	}
}

#endif
