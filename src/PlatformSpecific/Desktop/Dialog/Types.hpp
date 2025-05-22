/*
 * src/PlatformSpecific/Desktop/Dialog/Types.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	/**
	 * @brief The button layout enumeration.
	 */
	enum class ButtonLayout : uint8_t
	{
		NoButton,
		OK,
		OKCancel,
		YesNo,
		Quit
	};

	constexpr auto NoButtonString{"NoButton"};
	constexpr auto OKString{"OK"};
	constexpr auto OKCancelString{"OKCancel"};
	constexpr auto YesNoString{"YesNo"};
	constexpr auto QuitString{"Quit"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (ButtonLayout value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (ButtonLayout value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to an enumeration value.
	 * @param value A reference to a string.
	 * @return ButtonLayout
	 */
	[[nodiscard]]
	ButtonLayout to_ButtonLayout (const std::string & value) noexcept;

	/**
	 * @brief The type of message.
	 */
	enum class MessageType : uint8_t
	{
		Info,
		Warning,
		Error,
		Question
	};

	constexpr auto InfoString{"Info"};
	constexpr auto WarningString{"Warning"};
	constexpr auto ErrorString{"Error"};
	constexpr auto QuestionString{"Question"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (MessageType value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (MessageType value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to an enumeration value.
	 * @param value A reference to a string.
	 * @return MessageType
	 */
	[[nodiscard]]
	MessageType to_MessageType (const std::string & value) noexcept;

	/**
	 * @brief The user answer to dialog enumeration.
	 */
	enum class Answer : uint8_t
	{
		None,
		OK,
		Cancel,
		Yes,
		No,
		DialogFailure
	};

	constexpr auto NoneString{"None"};
	//constexpr auto OKString{"OK"};
	constexpr auto CancelString{"Cancel"};
	constexpr auto YesString{"Yes"};
	constexpr auto NoString{"No"};
	constexpr auto DialogFailureString{"DialogFailure"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (Answer value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (Answer value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to an enumeration value.
	 * @param value A reference to a string.
	 * @return Answer
	 */
	[[nodiscard]]
	Answer to_Answer (const std::string & value) noexcept;
}
