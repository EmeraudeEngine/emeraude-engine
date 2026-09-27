/*
 * src/Console/CommandSignature.cpp
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

#include "CommandSignature.hpp"

/* STL inclusions. */
#include <cmath>
#include <sstream>
#include <unordered_set>

namespace EmEn::Console
{
	namespace
	{
		/**
		 * @brief Writes an argument value the way a user would type it back.
		 * @param argument The argument.
		 * @return std::string
		 */
		std::string
		writeArgument (const Argument & argument) noexcept
		{
			if ( const auto * text = std::get_if< std::string >(&argument.value()) )
			{
				return *text;
			}

			if ( !argument.source().empty() )
			{
				return argument.source();
			}

			if ( const auto * boolean = std::get_if< bool >(&argument.value()) )
			{
				return *boolean ? "true" : "false";
			}

			if ( const auto * integer = std::get_if< int32_t >(&argument.value()) )
			{
				return std::to_string(*integer);
			}

			if ( const auto * number = std::get_if< float >(&argument.value()) )
			{
				std::ostringstream stream;
				stream << *number;

				return stream.str();
			}

			return {};
		}
	}

	CommandSignature::CommandSignature (std::vector< Parameter > parameters, CommandHint hints) noexcept
		: m_parameters{std::move(parameters)},
		m_hints{hints}
	{

	}

	bool
	CommandSignature::validate (std::string & error) const noexcept
	{
		std::unordered_set< std::string > names;
		bool omittableSeen = false;

		for ( size_t index = 0; index < m_parameters.size(); ++index )
		{
			const auto & parameter = m_parameters[index];

			if ( parameter.name().empty() )
			{
				error = "parameter #" + std::to_string(index) + " has no name";

				return false;
			}

			if ( !names.insert(parameter.name()).second )
			{
				error = "parameter '" + parameter.name() + "' is declared twice";

				return false;
			}

			if ( parameter.arity() == ParameterArity::Variadic && index + 1 != m_parameters.size() )
			{
				error = "the variadic parameter '" + parameter.name() + "' must be the last one";

				return false;
			}

			if ( parameter.defaultValue().has_value() )
			{
				if ( parameter.arity() != ParameterArity::Required )
				{
					error = "parameter '" + parameter.name() + "' is optional or variadic and cannot declare a default value";

					return false;
				}

				if ( !isConvertible(*parameter.defaultValue(), parameter.type()) )
				{
					error = "the default value of '" + parameter.name() + "' is not a " + to_cstring(parameter.type());

					return false;
				}
			}

			/* NOTE: arguments are positional, so a required parameter after an omittable one could never
			 * be reached without supplying the omittable one as well — the "optional" would be a lie. */
			if ( parameter.isOmittable() )
			{
				omittableSeen = true;
			}
			else if ( omittableSeen )
			{
				error = "the required parameter '" + parameter.name() + "' follows an optional one";

				return false;
			}
		}

		return true;
	}

	bool
	CommandSignature::checkArgumentCount (const Arguments & arguments, std::string & error) const noexcept
	{
		if ( !m_parameters.empty() && m_parameters.back().arity() == ParameterArity::Variadic )
		{
			return true;
		}

		if ( arguments.size() > m_parameters.size() )
		{
			error = "Too many arguments: " + std::to_string(arguments.size()) + " given, at most " + std::to_string(m_parameters.size()) + " accepted.";

			return false;
		}

		return true;
	}

	std::string
	CommandSignature::usage (const std::string & commandName) const noexcept
	{
		std::string text = commandName + "(";
		size_t openBrackets = 0;
		bool first = true;

		for ( const auto & parameter : m_parameters )
		{
			if ( parameter.isOmittable() )
			{
				text += first ? "[" : " [, ";
				++openBrackets;
			}
			else if ( !first )
			{
				text += ", ";
			}

			text += parameter.name();

			if ( parameter.arity() == ParameterArity::Variadic )
			{
				text += "...";
			}
			else if ( parameter.defaultValue().has_value() )
			{
				text += " = " + writeArgument(*parameter.defaultValue());
			}

			first = false;
		}

		text.append(openBrackets, ']');
		text += ")";

		return text;
	}

	std::string
	CommandSignature::hintsText () const noexcept
	{
		std::string text;

		const auto append = [&text] (const char * name) {
			if ( !text.empty() )
			{
				text += ", ";
			}

			text += name;
		};

		if ( hasHint(m_hints, CommandHint::ReadOnly) )
		{
			append("read-only");
		}

		if ( hasHint(m_hints, CommandHint::Destructive) )
		{
			append("destructive");
		}

		if ( hasHint(m_hints, CommandHint::Idempotent) )
		{
			append("idempotent");
		}

		return text;
	}

	const Argument *
	selectArgument (const Arguments & arguments, size_t index, const Parameter & parameter) noexcept
	{
		/* NOTE: an Undefined argument is a positional HOLE — "not supplied" — left by a caller that names
		 * its arguments (MCP) and omits one before a later one; the command line never produces it. */
		if ( index < arguments.size() && arguments[index].type() != ArgumentType::Undefined )
		{
			return &arguments[index];
		}

		if ( parameter.defaultValue().has_value() )
		{
			return &parameter.defaultValue().value();
		}

		return nullptr;
	}

	std::string
	missingArgumentError (const Parameter & parameter) noexcept
	{
		return "Missing argument '" + parameter.name() + "' (" + to_cstring(parameter.type()) + ": " + parameter.description() + ")";
	}

	std::string
	conversionError (const Parameter & parameter, const Argument & argument) noexcept
	{
		return "Argument '" + parameter.name() + "' expects " + ( parameter.type() == ParameterType::Integer ? "an " : "a " ) + to_cstring(parameter.type()) + ", got '" + writeArgument(argument) + "'.";
	}

	bool
	isConvertible (const Argument & argument, ParameterType type) noexcept
	{
		switch ( type )
		{
			case ParameterType::Boolean :
			{
				bool value = false;

				return convertArgument(argument, value);
			}

			case ParameterType::Integer :
			{
				int32_t value = 0;

				return convertArgument(argument, value);
			}

			case ParameterType::Float :
			{
				float value = 0.0F;

				return convertArgument(argument, value);
			}

			case ParameterType::String :
			case ParameterType::Any :
				break;
		}

		return argument.type() != ArgumentType::Undefined;
	}

	bool
	convertArgument (const Argument & argument, bool & value) noexcept
	{
		if ( const auto * boolean = std::get_if< bool >(&argument.value()) )
		{
			value = *boolean;

			return true;
		}

		if ( const auto * integer = std::get_if< int32_t >(&argument.value()) )
		{
			/* NOTE: 1/0 are accepted because the console has always spelled switches that way
			 * (setOverflowCensus(1)); any other integer is a typo, not a truth value. */
			if ( *integer == 0 || *integer == 1 )
			{
				value = *integer == 1;

				return true;
			}
		}

		return false;
	}

	bool
	convertArgument (const Argument & argument, int32_t & value) noexcept
	{
		if ( const auto * integer = std::get_if< int32_t >(&argument.value()) )
		{
			value = *integer;

			return true;
		}

		if ( const auto * number = std::get_if< float >(&argument.value()) )
		{
			/* NOTE: a machine client may send 5.0 for 5; 5.5 is refused rather than rounded. */
			if ( std::isfinite(*number) && std::trunc(*number) == *number && std::abs(*number) <= 16777216.0F )
			{
				value = static_cast< int32_t >(*number);

				return true;
			}
		}

		return false;
	}

	bool
	convertArgument (const Argument & argument, float & value) noexcept
	{
		if ( const auto * number = std::get_if< float >(&argument.value()) )
		{
			value = *number;

			return true;
		}

		if ( const auto * integer = std::get_if< int32_t >(&argument.value()) )
		{
			value = static_cast< float >(*integer);

			return true;
		}

		return false;
	}

	bool
	convertArgument (const Argument & argument, std::string & value) noexcept
	{
		if ( argument.type() == ArgumentType::Undefined )
		{
			return false;
		}

		value = writeArgument(argument);

		return true;
	}

	bool
	convertArgument (const Argument & argument, Argument & value) noexcept
	{
		if ( argument.type() == ArgumentType::Undefined )
		{
			return false;
		}

		value = argument;

		return true;
	}
}
