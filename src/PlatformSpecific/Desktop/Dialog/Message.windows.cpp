/*
 * src/PlatformSpecific/Desktop/Dialog/Message.windows.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#if IS_WINDOWS

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	UINT
	getMessageType (MessageType type)
	{
		switch ( type )
		{
			case MessageType::Info:
				return MB_ICONINFORMATION;

			case MessageType::Warning:
				return MB_ICONWARNING;

			case MessageType::Error:
				return MB_ICONERROR;

			case MessageType::Question:
				return MB_ICONQUESTION;

			default:
				return MB_ICONINFORMATION;
		}
	}

	UINT
	getButtonLayout (ButtonLayout type)
	{
		switch ( type )
		{
			case ButtonLayout::OK:
				return MB_OK;

			case ButtonLayout::OKCancel:
				return MB_OKCANCEL;

			case ButtonLayout::YesNo:
				return MB_YESNO;

			default:
				return MB_OK;
		}
	}

	bool
	Message::execute (Window * window) noexcept
	{
		UINT messageType = getMessageType(m_messageType);
		UINT layout = getButtonLayout(m_buttonLayout);

		/* NOTE: Convert strings from 8bit char to wide char. */
		const auto & title = this->title();
		const std::wstring wsTitle(title.cbegin(), title.cend());
		const std::wstring wsMessage(m_message.cbegin(), m_message.cend());

		HWND parentWindow = window != nullptr ? window->getWin32Window() : nullptr;

		switch ( MessageBoxW(parentWindow, wsMessage.data(), wsTitle.data(), messageType | layout) )
		{
			case IDOK :
				m_userAnswer = Answer::OK;
				break;

			case IDCANCEL :
				m_userAnswer = Answer::Cancel;
				break;

			case IDYES:
				m_userAnswer = Answer::Yes;
				break;

			case IDNO:
				m_userAnswer = Answer::No;
				break;

			default:
				break;
		}

		return true;
	}
}

#endif
