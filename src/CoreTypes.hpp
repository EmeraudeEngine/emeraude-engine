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

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class Renderer;
	}

	namespace Audio
	{
		class Manager;
	}
}

namespace EmEn
{
	/**
	 * @brief Context object containing engine-level services needed by various subsystems.
	 * @note All references are guaranteed to be valid for the lifetime of the engine.
	 */
	struct EngineContext
	{
		Graphics::Renderer & graphicsRenderer;
		Audio::Manager & audioManager;

		EngineContext (Graphics::Renderer & renderer, Audio::Manager & audioManager) noexcept
			: graphicsRenderer{renderer},
			audioManager{audioManager}
		{

		}

		/**
		 * @brief Non-copyable, non-movable (contains references)
		 */
		EngineContext (const EngineContext & ) = delete;

		/**
		 * @brief Non-copyable, non-movable (contains references)
		 * @return EngineContext &
		 */
		EngineContext & operator= (const EngineContext & ) = delete;
	};

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
	inline
	const char *
	to_cstring (Severity value) noexcept
	{
		switch ( value )
		{
			case Severity::Debug :
				return DebugString;

			case Severity::Info :
				return InfoString;

			case Severity::Success :
				return SuccessString;

			case Severity::Warning :
				return WarningString;

			case Severity::Error :
				return ErrorString;

			case Severity::Fatal :
				return FatalString;

			default:
				return "Unknown";
		}
	}

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (Severity value)
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
	inline
	const char *
	to_cstring (LogFormat value) noexcept
	{
		switch ( value )
		{
			case LogFormat::Text :
				return TextString;

			case LogFormat::JSON :
				return JSONString;

			case LogFormat::HTML :
				return HTMLString;

			default:
				return "Text";
		}
	}

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (LogFormat value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to a LogFormat enumeration.
	 * @param value A reference to a string.
	 * @return LogFormat
	 */
	[[nodiscard]]
	inline
	LogFormat
	to_LogFormat (const std::string & value) noexcept
	{
		if ( value == TextString )
		{
			return LogFormat::Text;
		}

		if ( value == JSONString )
		{
			return LogFormat::JSON;
		}

		if ( value == HTMLString )
		{
			return LogFormat::HTML;
		}

		return LogFormat::Text;
	}

	enum class ANSIColorCode : uint8_t
	{
		Black = 30,
		Red = 31,
		Green = 32,
		Yellow = 33,
		Blue = 34,
		Magenta = 35,
		Cyan = 36,
		White = 37,
		BrightBlack = 90,
		BrightRed = 91,
		BrightGreen = 92,
		BrightYellow = 93,
		BrightBlue = 94,
		BrightMagenta = 95,
		BrightCyan = 96,
		BrightWhite = 97
	};
}
