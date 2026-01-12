/*
 * src/PlatformSpecific/Desktop/Notification.linux.cpp
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

#include "Notification.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <string>

namespace EmEn::PlatformSpecific::Desktop
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

		const char *
		toIconName (NotificationIcon icon) noexcept
		{
			/* Use freedesktop standard icon names. */
			switch ( icon )
			{
				case NotificationIcon::Info:
					return "dialog-information";

				case NotificationIcon::Warning:
					return "dialog-warning";

				case NotificationIcon::Error:
					return "dialog-error";

				default:
					return "dialog-information";
			}
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
	}

	bool
	Notification::show () noexcept
	{
		std::string command;

		/* Prefer kdialog on KDE, zenity otherwise. */
		const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());

		if ( useKdialog )
		{
			/* kdialog --icon <icon> --title "title" --passivepopup "message" 5 */
			command = "kdialog";

			if ( m_icon.has_value() )
			{
				command += " --icon ";
				command += toIconName(m_icon.value());
			}

			command += " --title ";
			command += escapeShellArg(m_title);
			command += " --passivepopup ";
			command += escapeShellArg(m_message);
			command += " 5";
		}
		else if ( hasZenity() )
		{
			/* zenity --notification --icon=<icon> --text="title\nmessage" */
			command = "zenity --notification";

			if ( m_icon.has_value() )
			{
				command += " --icon=";
				command += toIconName(m_icon.value());
			}

			command += " --text=";
			command += escapeShellArg(m_title + "\n" + m_message);
		}
		else
		{
			/* No notification tool available. */
			return false;
		}

		/* Redirect output to /dev/null. */
		command += " > /dev/null 2>&1 &";

		/* Execute command in background. */
		const int result = system(command.c_str());

		return result == 0;
	}
}
