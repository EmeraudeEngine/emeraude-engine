/*
 * src/Types.hpp
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

namespace EmEn
{
	/**
	 * @brief Standard cursor handled automatically by GLFW.
	 */
	enum class CursorType : uint8_t
	{
		Arrow,
		TextInput,
		Crosshair,
		Hand,
		HorizontalResize,
		VerticalResize
	};

	/**
	 * @brief The message severity enumeration.
	 */
	enum class Severity : uint8_t
	{
		Debug,
		Success,
		Info,
		Warning,
		Error,
		Fatal
	};

	static constexpr auto DebugString{"Debug"};
	static constexpr auto SuccessString{"Success"};
	static constexpr auto InfoString{"Info"};
	static constexpr auto WarningString{"Warning"};
	static constexpr auto ErrorString{"Error"};
	static constexpr auto FatalString{"Fatal"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (Severity value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (Severity value) noexcept
	{
		return {to_cstring(value)};
	}

	/** @brief Log file format enumeration. */
	enum class LogFormat : uint8_t
	{
		Text,
		JSON,
		HTML
	};

	static constexpr auto TextString{"Text"};
	static constexpr auto JSONString{"JSON"};
	static constexpr auto HTMLString{"HTML"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (LogFormat value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (LogFormat value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to a LogFormat enumeration.
	 * @param value A reference to a string.
	 * @return LogFormat
	 */
	[[nodiscard]]
	LogFormat to_LogFormat (const std::string & value) noexcept;
}
