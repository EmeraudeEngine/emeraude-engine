/*
 * src/PlatformSpecific/Desktop/Dialog/CustomMessage.linux.cpp
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

#include "CustomMessage.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	namespace
	{
		const char *
		getZenityIconName (MessageType messageType) noexcept
		{
			switch ( messageType )
			{
				case MessageType::Warning:
					return "dialog-warning";

				case MessageType::Error:
					return "dialog-error";

				case MessageType::Question:
					return "dialog-question";

				case MessageType::Info:
				default:
					return "dialog-information";
			}
		}
	}

	bool
	CustomMessage::execute (Window * /*window*/) noexcept
	{
		if ( m_buttons.empty() )
		{
			m_clickedIndex = -1;

			return false;
		}

		std::string command;

		/* Prefer kdialog on KDE for up to 3 buttons, zenity otherwise.
		 * Note: kdialog only supports up to 3 buttons (yesnocancel), so we use zenity for more. */
		if ( hasKdialog() && (!hasZenity() || isKdeDesktop()) && m_buttons.size() <= 3 )
		{
			command = "kdialog";

			/* Build kdialog command based on button count. */
			if ( m_buttons.size() == 1 )
			{
				/* Single button: use msgbox or appropriate type. */
				switch ( m_messageType )
				{
					case MessageType::Error:
						command += " --error";
						break;

					case MessageType::Warning:
						command += " --sorry";
						break;

					case MessageType::Question:
					case MessageType::Info:
					default:
						command += " --msgbox";
						break;
				}

				command += " ";
				command += escapeShellArg(m_message);
				command += " --title ";
				command += escapeShellArg(this->title());
			}
			else if ( m_buttons.size() == 2 )
			{
				/* Two buttons: use yesno with custom labels. */
				if ( m_messageType == MessageType::Warning || m_messageType == MessageType::Error )
				{
					command += " --warningyesno";
				}
				else
				{
					command += " --yesno";
				}

				command += " ";
				command += escapeShellArg(m_message);
				command += " --title ";
				command += escapeShellArg(this->title());
				command += " --yes-label ";
				command += escapeShellArg(m_buttons[0]);
				command += " --no-label ";
				command += escapeShellArg(m_buttons[1]);
			}
			else /* m_buttons.size() == 3 */
			{
				/* Three buttons: use yesnocancel with custom labels. */
				if ( m_messageType == MessageType::Warning || m_messageType == MessageType::Error )
				{
					command += " --warningyesnocancel";
				}
				else
				{
					command += " --yesnocancel";
				}

				command += " ";
				command += escapeShellArg(m_message);
				command += " --title ";
				command += escapeShellArg(this->title());
				command += " --yes-label ";
				command += escapeShellArg(m_buttons[0]);
				command += " --no-label ";
				command += escapeShellArg(m_buttons[1]);
				command += " --cancel-label ";
				command += escapeShellArg(m_buttons[2]);
			}

			/* Execute and parse kdialog result. */
			int exitCode = 0;
			(void)executeCommand(command, exitCode);

			if ( m_buttons.size() == 1 )
			{
				m_clickedIndex = 0;
			}
			else if ( m_buttons.size() == 2 )
			{
				/* kdialog: 0 = Yes (first button), 1 = No (second button). */
				m_clickedIndex = (exitCode == 0) ? 0 : 1;
			}
			else /* m_buttons.size() == 3 */
			{
				/* kdialog: 0 = Yes (first), 1 = No (second), 2 = Cancel (third). */
				switch ( exitCode )
				{
					case 0:
						m_clickedIndex = 0;
						break;

					case 1:
						m_clickedIndex = 1;
						break;

					case 2:
					default:
						m_clickedIndex = 2;
						break;
				}
			}

			return true;
		}

		if ( hasZenity() )
		{
			/* Zenity with --question --switch and --extra-button for custom buttons.
			 * The output contains the button text that was clicked. */
			command = "zenity --question --switch";
			command += " --title ";
			command += escapeShellArg(this->title());
			command += " --width=300 --height=0 --no-markup";
			command += " --text ";
			command += escapeShellArg(m_message);
			command += " --icon=";
			command += getZenityIconName(m_messageType);

			/* Add all buttons as extra buttons. */
			for ( const auto & label : m_buttons )
			{
				command += " --extra-button=";
				command += escapeShellArg(label);
			}

			/* Execute and parse zenity result. */
			int exitCode = 0;
			const std::string output = executeCommand(command, exitCode);

			/* In switch mode, zenity outputs the button label that was clicked. */
			m_clickedIndex = -1;

			for ( size_t i = 0; i < m_buttons.size(); ++i )
			{
				if ( output == m_buttons[i] )
				{
					m_clickedIndex = static_cast< int >(i);
					break;
				}
			}

			/* If no match found and dialog was closed, keep -1. */
			return true;
		}

		/* No dialog tool available. */
		m_clickedIndex = -1;

		return false;
	}
}
