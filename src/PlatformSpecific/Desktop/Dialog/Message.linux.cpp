/*
 * src/PlatformSpecific/Desktop/Dialog/Message.linux.cpp
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

#if IS_LINUX

/* Third-party inclusions. */
#include "portable-file-dialogs.h"

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	pfd::icon
	getMessageType (MessageType messageType) noexcept
	{
		switch ( messageType )
		{
			case MessageType::Info:
				return pfd::icon::info;

			case MessageType::Warning:
				return pfd::icon::warning;

			case MessageType::Error:
				return pfd::icon::error;

			case MessageType::Question:
				return pfd::icon::question;

			default:
				return pfd::icon::info;
		}
	}

	pfd::choice
	getButtonLayout (ButtonLayout buttonLayout) noexcept
	{
		switch ( buttonLayout )
		{
			case ButtonLayout::OKCancel :
				return pfd::choice::ok_cancel;

			case ButtonLayout::YesNo :
				return pfd::choice::yes_no;

			case ButtonLayout::OK :
			case ButtonLayout::Quit :
			case ButtonLayout::NoButton :
			default:
				return pfd::choice::ok;
		}
	}

	bool
	Message::execute (Window * /*window*/) noexcept
	{
		auto output = pfd::message{
			this->title(),
			m_message,
			getButtonLayout(m_buttonLayout),
			getMessageType(m_messageType)
		};

		switch ( output.result() )
		{
			case pfd::button::cancel :
			case pfd::button::abort :
			case pfd::button::ignore :
			case pfd::button::retry :
				m_userAnswer = Answer::Cancel;
				break;

			case pfd::button::ok :
				m_userAnswer = Answer::OK;
				break;

			case pfd::button::yes :
				m_userAnswer = Answer::Yes;
				break;

			case pfd::button::no :
				m_userAnswer = Answer::No;
				break;
		}

		return true;
	}
}

#endif
