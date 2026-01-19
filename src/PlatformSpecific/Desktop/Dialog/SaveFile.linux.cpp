/*
 * src/PlatformSpecific/Desktop/Dialog/SaveFile.linux.cpp
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

#include "SaveFile.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	SaveFile::execute (Window * /*window*/) noexcept
	{
		std::string command;

		/* Prefer kdialog on KDE, zenity otherwise. */
		if ( hasKdialog() && (!hasZenity() || isKdeDesktop()) )
		{
			command = "kdialog --getsavefilename";

			/* Default path (empty for current directory). */
			command += " ''";

			/* File filters. */
			if ( !m_extensionFilters.empty() )
			{
				command += " ";
				command += escapeShellArg(buildKdialogFilters(m_extensionFilters));
			}

			command += " --title ";
			command += escapeShellArg(this->title());
		}
		else if ( hasZenity() )
		{
			command = "zenity --file-selection --save";
			command += " --title ";
			command += escapeShellArg(this->title());

			/* File filters. */
			if ( !m_extensionFilters.empty() )
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
			m_filepath = output;
		}

		return true;
	}
}
