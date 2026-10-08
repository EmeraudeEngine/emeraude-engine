/*
 * src/Console/Argument.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <string>
#include <variant>
#include <vector>

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::Console
{
	/** @brief Type-safe variant holding a console argument value. */
	using ArgumentValue = std::variant< std::monostate, bool, int32_t, float, std::string >;

	/**
	 * @brief The console argument class.
	 */
	class EMEN_API Argument final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Argument"};

			/**
			 * @brief Constructs an undefined argument.
			 */
			Argument () noexcept = default;

			/**
			 * @brief Constructs a boolean argument.
			 * @param value The value
			 */
			explicit
			Argument (bool value) noexcept
				: m_type{ArgumentType::Boolean},
				m_value{value}
			{

			}

			/**
			 * @brief Constructs an integer number argument.
			 * @param value The value
			 */
			explicit
			Argument (int32_t value) noexcept
				: m_type{ArgumentType::Integer},
				m_value{value}
			{

			}

			/**
			 * @brief Constructs a floating point number argument.
			 * @param value The value
			 */
			explicit
			Argument (float value) noexcept
				: m_type{ArgumentType::Float},
				m_value{value}
			{

			}

			/**
			 * @brief Constructs a string argument.
			 * @param value The value
			 */
			explicit
			Argument (std::string value) noexcept
				: m_type{ArgumentType::String},
				m_value{std::move(value)}
			{

			}

			/**
			 * @brief Returns the type of argument.
			 * @return ArgumentType
			 */
			[[nodiscard]]
			ArgumentType
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the raw value.
			 * @return const ArgumentValue &
			 */
			[[nodiscard]]
			const ArgumentValue &
			value () const noexcept
			{
				return m_value;
			}

			/**
			 * @brief Returns a boolean value.
			 * @return bool
			 */
			[[nodiscard]]
			bool asBoolean () const noexcept;

			/**
			 * @brief Returns an integer number.
			 * @return int32_t
			 */
			[[nodiscard]]
			int32_t asInteger () const noexcept;

			/**
			 * @brief Returns an integer floating point number.
			 * @return float
			 */
			[[nodiscard]]
			float asFloat () const noexcept;

			/**
			 * @brief Returns a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string asString () const noexcept;

			/**
			 * @brief Returns the text this argument was parsed from, if it came from a command line.
			 * @note Empty for an argument built from a typed value (a default value, a JSON call).
			 * A typed String parameter receives this text verbatim, so a bare `01` stays "01"
			 * instead of travelling through the integer 1.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			source () const noexcept
			{
				return m_source;
			}

			/**
			 * @brief Records the text this argument was parsed from.
			 * @param source The unquoted token [std::move].
			 */
			void
			setSource (std::string source) noexcept
			{
				m_source = std::move(source);
			}

		private:

			ArgumentType m_type{ArgumentType::Undefined};
			ArgumentValue m_value;
			std::string m_source;
	};

	using Arguments = std::vector< Argument >;
}
