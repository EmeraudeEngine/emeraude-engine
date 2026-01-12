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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <string>
#include <vector>
#include <any>

/* Local inclusions. */
#include "Types.hpp"

namespace EmEn::Console
{
	/**
	 * @brief The console argument class.
	 */
	class Argument final
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
			 * @return const std::any &
			 */
			[[nodiscard]]
			const std::any &
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

		private:

			ArgumentType m_type{ArgumentType::Undefined};
			std::any m_value;
	};

	using Arguments = std::vector< Argument >;
}
