/*
 * src/Console/Parameter.hpp
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
#include <cstdint>
#include <optional>
#include <string>

/* Local inclusions for usages. */
#include "Argument.hpp"
#include "Types.hpp"

namespace EmEn::Console
{
	/**
	 * @brief Declaration of one parameter of a typed console command.
	 * @note The author writes the name, the description and optionally a default value; the
	 * type and the arity are deduced from the callable by bindCommand() (see TypedBinding.hpp),
	 * which stores a resolved copy. A default value makes a plain `T` parameter optional:
	 * `Parameter{"frameCount", "Number of frames.", 5}` on an `int32_t` argument.
	 */
	class EMEN_API Parameter final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleParameter"};

			/**
			 * @brief Declares a parameter without a default value.
			 * @param name The parameter name, as shown in the usage and to a machine client [std::move].
			 * @param description One line saying what the value means, with its unit [std::move].
			 */
			Parameter (std::string name, std::string description) noexcept;

			/**
			 * @brief Declares a parameter with a boolean default value.
			 * @param name The parameter name [std::move].
			 * @param description One line saying what the value means [std::move].
			 * @param defaultValue The value used when the argument is omitted.
			 */
			Parameter (std::string name, std::string description, bool defaultValue) noexcept;

			/**
			 * @brief Declares a parameter with an integer default value.
			 * @param name The parameter name [std::move].
			 * @param description One line saying what the value means [std::move].
			 * @param defaultValue The value used when the argument is omitted.
			 */
			Parameter (std::string name, std::string description, int32_t defaultValue) noexcept;

			/**
			 * @brief Declares a parameter with a floating point default value.
			 * @param name The parameter name [std::move].
			 * @param description One line saying what the value means [std::move].
			 * @param defaultValue The value used when the argument is omitted.
			 */
			Parameter (std::string name, std::string description, float defaultValue) noexcept;

			/**
			 * @brief Declares a parameter with a string default value.
			 * @param name The parameter name [std::move].
			 * @param description One line saying what the value means [std::move].
			 * @param defaultValue The value used when the argument is omitted [std::move].
			 */
			Parameter (std::string name, std::string description, std::string defaultValue) noexcept;

			/**
			 * @brief Declares a parameter with a string literal default value.
			 * @note Without this overload a string literal would silently pick the bool overload.
			 * @param name The parameter name [std::move].
			 * @param description One line saying what the value means [std::move].
			 * @param defaultValue The value used when the argument is omitted.
			 */
			Parameter (std::string name, std::string description, const char * defaultValue) noexcept;

			/**
			 * @brief A double literal is refused: write a float literal (1.0F) or an integer.
			 * @note Deleted so `Parameter{"scale", "…", 1.0}` fails to compile with this message
			 * instead of an ambiguity between the int32_t, float and bool overloads.
			 */
			Parameter (std::string name, std::string description, double defaultValue) = delete;

			/**
			 * @brief Returns the parameter name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the parameter description.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			description () const noexcept
			{
				return m_description;
			}

			/**
			 * @brief Returns the default value, if one was declared.
			 * @return const std::optional< Argument > &
			 */
			[[nodiscard]]
			const std::optional< Argument > &
			defaultValue () const noexcept
			{
				return m_defaultValue;
			}

			/**
			 * @brief Returns the deduced type (ParameterType::Any until resolved by bindCommand()).
			 * @return ParameterType
			 */
			[[nodiscard]]
			ParameterType
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the deduced arity (ParameterArity::Required until resolved by bindCommand()).
			 * @return ParameterArity
			 */
			[[nodiscard]]
			ParameterArity
			arity () const noexcept
			{
				return m_arity;
			}

			/**
			 * @brief Returns whether the argument may be omitted (optional, variadic, or with a default).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOmittable () const noexcept
			{
				return m_arity != ParameterArity::Required || m_defaultValue.has_value();
			}

			/**
			 * @brief Returns a copy completed with the type and arity deduced from the callable.
			 * @param type The deduced type.
			 * @param arity The deduced arity.
			 * @return Parameter
			 */
			[[nodiscard]]
			Parameter resolved (ParameterType type, ParameterArity arity) const noexcept;

		private:

			std::string m_name;
			std::string m_description;
			std::optional< Argument > m_defaultValue;
			ParameterType m_type{ParameterType::Any};
			ParameterArity m_arity{ParameterArity::Required};
	};

	/**
	 * @brief Returns the lowercase name of a parameter type, as shown in the usage ("integer", "float", …).
	 * @param type The parameter type.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API const char * to_cstring (ParameterType type) noexcept;
}
