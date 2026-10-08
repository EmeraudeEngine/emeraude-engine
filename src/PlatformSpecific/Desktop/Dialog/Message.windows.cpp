/*
 * src/PlatformSpecific/Desktop/Dialog/Message.windows.cpp
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

#include "Message.hpp"

/* Local inclusions. */
#include "PlatformSpecific/Helpers.hpp"
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
	Message::execute (Window & window, bool parentToWindow) noexcept
	{
		UINT messageType = getMessageType(m_messageType);
		UINT layout = getButtonLayout(m_buttonLayout);

		/* NOTE: Convert strings from UTF-8 to wide char. */
		const std::wstring wsTitle = convertUTF8ToWide(this->title());
		const std::wstring wsMessage = convertUTF8ToWide(m_message);

		HWND parentWindow = parentToWindow ? window.getWin32Window() : nullptr;

		/* NOTE: The second button (No / Cancel) as the default one: Enter, Space or the first button's accelerator no
		 * longer answer Yes by accident. The box is brought to the foreground, and kept on top when it has no parent
		 * (e.g. asked before the window exists): an unseen box with the focus caught a stray key press. */
		UINT behavior = MB_SETFOREGROUND;

		if ( (m_buttonLayout == ButtonLayout::YesNo || m_buttonLayout == ButtonLayout::OKCancel) && (m_defaultAnswer == Answer::No || m_defaultAnswer == Answer::Cancel) )
		{
			behavior |= MB_DEFBUTTON2;
		}

		if ( parentWindow == nullptr )
		{
			behavior |= MB_TOPMOST;
		}

		switch ( MessageBoxW(parentWindow, wsMessage.data(), wsTitle.data(), messageType | layout | behavior) )
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
