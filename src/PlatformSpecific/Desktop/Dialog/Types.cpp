/*
 * src/PlatformSpecific/Desktop/Dialog/Types.cpp
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

#include "Types.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>
#include <string>

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	const char *
	to_cstring (ButtonLayout value) noexcept
	{
		switch ( value )
		{
			case ButtonLayout::NoButton :
				return NoButtonString;

			case ButtonLayout::OK :
				return OKString;

			case ButtonLayout::OKCancel :
				return OKCancelString;

			case ButtonLayout::YesNo :
				return YesNoString;

			case ButtonLayout::Quit :
				return QuitString;

			default:
				return nullptr;
		}
	}

	ButtonLayout
	to_ButtonLayout (const std::string & value) noexcept
	{
		if ( value == NoButtonString )
		{
			return ButtonLayout::NoButton;
		}

		if ( value == OKString )
		{
			return ButtonLayout::OK;
		}

		if ( value == OKCancelString )
		{
			return ButtonLayout::OKCancel;
		}

		if ( value == YesNoString )
		{
			return ButtonLayout::YesNo;
		}

		if ( value == QuitString )
		{
			return ButtonLayout::Quit;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'OK' by default." << "\n";

		return ButtonLayout::OK;
	}

	const char *
	to_cstring (MessageType value) noexcept
	{
		switch ( value )
		{
			case MessageType::Info :
				return InfoString;

			case MessageType::Warning :
				return WarningString;

			case MessageType::Error :
				return ErrorString;

			case MessageType::Question :
				return QuestionString;

			default:
				return nullptr;
		}
	}

	MessageType
	to_MessageType (const std::string & value) noexcept
	{
		if ( value == InfoString )
		{
			return MessageType::Info;
		}

		if ( value == WarningString )
		{
			return MessageType::Warning;
		}

		if ( value == ErrorString )
		{
			return MessageType::Error;
		}

		if ( value == QuestionString )
		{
			return MessageType::Question;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'Info' by default." << "\n";

		return MessageType::Info;
	}

	const char *
	to_cstring (Answer value) noexcept
	{
		switch ( value )
		{
			case Answer::None :
				return NoneString;

			case Answer::OK :
				return OKString;

			case Answer::Cancel :
				return CancelString;

			case Answer::Yes :
				return YesString;

			case Answer::No :
				return NoString;

			case Answer::DialogFailure :
				return DialogFailureString;

			default:
				return nullptr;
		}
	}

	Answer
	to_Answer (const std::string & value) noexcept
	{
		if ( value == NoneString )
		{
			return Answer::None;
		}

		if ( value == OKString )
		{
			return Answer::OK;
		}

		if ( value == CancelString )
		{
			return Answer::Cancel;
		}

		if ( value == YesString )
		{
			return Answer::Yes;
		}

		if ( value == NoString )
		{
			return Answer::No;
		}

		if ( value == DialogFailureString )
		{
			return Answer::DialogFailure;
		}

		std::cerr << __PRETTY_FUNCTION__ << " : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return Answer::None;
	}
}
