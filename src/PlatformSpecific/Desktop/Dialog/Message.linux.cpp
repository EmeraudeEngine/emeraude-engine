/*
 * src/PlatformSpecific/Desktop/Dialog/Message.linux.cpp
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

#include "Message.hpp"

#if IS_LINUX

/* STL inclusions. */
#include <array>
#include <cstdlib>
#include <cstdio>

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

		[[maybe_unused]]
		const char *
		getKdialogIconName (MessageType messageType) noexcept
		{
			switch ( messageType )
			{
				case MessageType::Warning:
					return "warning";

				case MessageType::Error:
					return "error";

				case MessageType::Question:
					return "question";

				case MessageType::Info:
				default:
					return "information";
			}
		}

		std::string
		executeCommand (const std::string & command, int & exitCode) noexcept
		{
			std::string output;
			std::array< char, 256 > buffer{};

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

			/* Remove trailing newline if any. */
			while ( !output.empty() && output.back() == '\n' )
			{
				output.pop_back();
			}

			return output;
		}
	}

	bool
	Message::execute (Window * /*window*/) noexcept
	{
		std::string command;

		/* Prefer kdialog on KDE, zenity otherwise. */
		const bool useKdialog = hasKdialog() && (!hasZenity() || isKdeDesktop());

		if ( useKdialog )
		{
			command = "kdialog";

			/* KDialog dialog type and button handling. */
			switch ( m_buttonLayout )
			{
				case ButtonLayout::OK:
				case ButtonLayout::Quit:
				case ButtonLayout::NoButton:
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
					break;

				case ButtonLayout::OKCancel:
					if ( m_messageType == MessageType::Warning || m_messageType == MessageType::Error )
					{
						command += " --warningyesno";
					}
					else
					{
						command += " --yesno";
					}
					break;

				case ButtonLayout::YesNo:
					if ( m_messageType == MessageType::Warning || m_messageType == MessageType::Error )
					{
						command += " --warningyesno";
					}
					else
					{
						command += " --yesno";
					}
					break;
			}

			command += " ";
			command += escapeShellArg(m_message);
			command += " --title ";
			command += escapeShellArg(this->title());

			/* OKCancel uses Yes/No labels in kdialog. */
			if ( m_buttonLayout == ButtonLayout::OKCancel )
			{
				command += " --yes-label OK --no-label Cancel";
			}
		}
		else if ( hasZenity() )
		{
			command = "zenity";

			/* Zenity dialog type. */
			switch ( m_buttonLayout )
			{
				case ButtonLayout::OK:
				case ButtonLayout::Quit:
				case ButtonLayout::NoButton:
					switch ( m_messageType )
					{
						case MessageType::Error:
							command += " --error";
							break;

						case MessageType::Warning:
							command += " --warning";
							break;

						case MessageType::Question:
						case MessageType::Info:
						default:
							command += " --info";
							break;
					}
					break;

				case ButtonLayout::OKCancel:
					command += " --question --cancel-label=Cancel --ok-label=OK";
					break;

				case ButtonLayout::YesNo:
					command += " --question --switch --extra-button=No --extra-button=Yes";
					break;
			}

			command += " --title ";
			command += escapeShellArg(this->title());
			command += " --width=300 --height=0 --no-markup";
			command += " --text ";
			command += escapeShellArg(m_message);
			command += " --icon=";
			command += getZenityIconName(m_messageType);
		}
		else
		{
			/* No dialog tool available. */
			m_userAnswer = Answer::DialogFailure;

			return false;
		}

		/* Execute the command and get the result. */
		int exitCode = 0;
		const std::string output = executeCommand(command, exitCode);

		/* Parse the result based on the tool and button layout. */
		if ( useKdialog )
		{
			/* KDialog: exit code 0 = Yes/OK, non-zero = No/Cancel. */
			switch ( m_buttonLayout )
			{
				case ButtonLayout::OK:
				case ButtonLayout::Quit:
				case ButtonLayout::NoButton:
					m_userAnswer = Answer::OK;
					break;

				case ButtonLayout::OKCancel:
					m_userAnswer = (exitCode == 0) ? Answer::OK : Answer::Cancel;
					break;

				case ButtonLayout::YesNo:
					m_userAnswer = (exitCode == 0) ? Answer::Yes : Answer::No;
					break;
			}
		}
		else
		{
			/* Zenity: parse output for switch mode, exit code otherwise. */
			if ( m_buttonLayout == ButtonLayout::YesNo )
			{
				/* Switch mode: output contains the button text. */
				if ( output == "Yes" )
				{
					m_userAnswer = Answer::Yes;
				}
				else
				{
					m_userAnswer = Answer::No;
				}
			}
			else if ( m_buttonLayout == ButtonLayout::OKCancel )
			{
				m_userAnswer = (exitCode == 0) ? Answer::OK : Answer::Cancel;
			}
			else
			{
				m_userAnswer = Answer::OK;
			}
		}

		return true;
	}
}

#endif
