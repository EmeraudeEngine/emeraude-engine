/*
 * src/Console/CommandSignature.hpp
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
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "Argument.hpp"
#include "Parameter.hpp"
#include "Types.hpp"

namespace EmEn::Console
{
	/**
	 * @brief The declared contract of a typed console command: its resolved parameters and its hints.
	 * @note Built by a typed bindCommand() from the callable's C++ signature and the Parameter list,
	 * so the declaration cannot drift from the code that runs. It is what the help dump prints
	 * and what a machine client (MCP) turns into an input schema.
	 */
	class EMEN_API CommandSignature final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleCommandSignature"};

			/**
			 * @brief Constructs a command signature.
			 * @param parameters The resolved parameters, in call order [std::move].
			 * @param hints The behaviour hints.
			 */
			CommandSignature (std::vector< Parameter > parameters, CommandHint hints) noexcept;

			/**
			 * @brief Returns the resolved parameters, in call order.
			 * @return const std::vector< Parameter > &
			 */
			[[nodiscard]]
			const std::vector< Parameter > &
			parameters () const noexcept
			{
				return m_parameters;
			}

			/**
			 * @brief Returns the behaviour hints.
			 * @return CommandHint
			 */
			[[nodiscard]]
			CommandHint
			hints () const noexcept
			{
				return m_hints;
			}

			/**
			 * @brief Checks the declaration itself: unique names, defaults convertible to their type,
			 * no default on an optional or variadic parameter, required parameters before omittable ones.
			 * @param error Receives the reason when the declaration is invalid.
			 * @return bool
			 */
			[[nodiscard]]
			bool validate (std::string & error) const noexcept;

			/**
			 * @brief Checks the number of supplied arguments against the declaration.
			 * @note Missing arguments are reported by the parameter they belong to, not here.
			 * @param arguments The supplied arguments.
			 * @param error Receives the reason when there are too many arguments.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkArgumentCount (const Arguments & arguments, std::string & error) const noexcept;

			/**
			 * @brief Returns the usage line of the command, e.g. "createScene(name, boundary [, backgroundName])".
			 * @param commandName The name the command is called by.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string usage (const std::string & commandName) const noexcept;

			/**
			 * @brief Returns the names of the hints set, e.g. "read-only, idempotent" (empty when none).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string hintsText () const noexcept;

		private:

			std::vector< Parameter > m_parameters;
			CommandHint m_hints;
	};

	/**
	 * @brief Returns the supplied argument at an index, else the parameter's default value, else nullptr.
	 * @param arguments The supplied arguments.
	 * @param index The index of the parameter.
	 * @param parameter The resolved parameter.
	 * @return const Argument *
	 */
	[[nodiscard]]
	EMEN_API const Argument * selectArgument (const Arguments & arguments, size_t index, const Parameter & parameter) noexcept;

	/**
	 * @brief Returns the error of a required parameter that received no argument.
	 * @param parameter The resolved parameter.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string missingArgumentError (const Parameter & parameter) noexcept;

	/**
	 * @brief Returns the error of an argument that does not convert to its parameter's type.
	 * @param parameter The resolved parameter.
	 * @param argument The offending argument.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string conversionError (const Parameter & parameter, const Argument & argument) noexcept;

	/**
	 * @brief Returns whether an argument converts to a parameter type.
	 * @param argument The argument.
	 * @param type The parameter type.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool isConvertible (const Argument & argument, ParameterType type) noexcept;

	/**
	 * @brief Converts an argument to a boolean parameter: true/false, or the integers 1/0.
	 * @param argument The argument.
	 * @param value Receives the value.
	 * @return bool False when the argument does not convert.
	 */
	[[nodiscard]]
	EMEN_API bool convertArgument (const Argument & argument, bool & value) noexcept;

	/**
	 * @brief Converts an argument to an integer parameter: an integer, or a float with no fractional part.
	 * @param argument The argument.
	 * @param value Receives the value.
	 * @return bool False when the argument does not convert.
	 */
	[[nodiscard]]
	EMEN_API bool convertArgument (const Argument & argument, int32_t & value) noexcept;

	/**
	 * @brief Converts an argument to a float parameter: a float or an integer.
	 * @param argument The argument.
	 * @param value Receives the value.
	 * @return bool False when the argument does not convert.
	 */
	[[nodiscard]]
	EMEN_API bool convertArgument (const Argument & argument, float & value) noexcept;

	/**
	 * @brief Converts an argument to a string parameter: a string, else the text it was parsed from,
	 * else its value written out. Never fails.
	 * @param argument The argument.
	 * @param value Receives the value.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool convertArgument (const Argument & argument, std::string & value) noexcept;

	/**
	 * @brief Passes an argument to a parameter declared as a raw Argument (any scalar). Never fails.
	 * @param argument The argument.
	 * @param value Receives a copy.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool convertArgument (const Argument & argument, Argument & value) noexcept;
}
