/*
 * src/PlatformSpecific/Desktop/Dialog/TextInput.linux.cpp
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

#include "TextInput.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	TextInput::execute (Window & /*window*/, bool /*parentToWindow*/) noexcept
	{
		std::string command;

		/* Prefer kdialog on KDE, zenity otherwise. */
		const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());

		if ( useKdialog )
		{
			switch ( m_inputMode )
			{
				case InputMode::Password:
					command = "kdialog --password ";
					command += escapeShellArg(m_message);
					break;

				case InputMode::MultiLine:
					command = "kdialog --textinputbox ";
					command += escapeShellArg(m_message);
					command += " ";
					command += escapeShellArg(m_defaultText);
					break;

				case InputMode::SingleLine:
				default:
					command = "kdialog --inputbox ";
					command += escapeShellArg(m_message);

					if ( !m_defaultText.empty() )
					{
						command += " ";
						command += escapeShellArg(m_defaultText);
					}
					break;
			}

			command += " --title ";
			command += escapeShellArg(this->title());
		}
		else if ( hasZenity() )
		{
			switch ( m_inputMode )
			{
				case InputMode::Password:
					command = "zenity --password";
					command += " --title ";
					command += escapeShellArg(this->title());
					break;

				case InputMode::MultiLine:
					command = "zenity --text-info --editable";
					command += " --title ";
					command += escapeShellArg(this->title());
					command += " --width=500 --height=300";
					break;

				case InputMode::SingleLine:
				default:
					command = "zenity --entry";
					command += " --title ";
					command += escapeShellArg(this->title());
					command += " --text ";
					command += escapeShellArg(m_message);

					if ( !m_defaultText.empty() )
					{
						command += " --entry-text ";
						command += escapeShellArg(m_defaultText);
					}
					break;
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
		if ( exitCode != 0 )
		{
			m_canceled = true;
		}
		else
		{
			m_text = output;
		}

		return true;
	}
}
