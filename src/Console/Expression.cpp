/*
 * src/Console/Expression.cpp
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

#include "Expression.hpp"

/* Local inclusions. */
#include "Libs/String.hpp"

namespace EmEn::Console
{
	using namespace Libs;

	Expression::Expression (const std::string & command) noexcept
	{
		std::string buffer;

		std::vector< std::string > arguments;

		bool inArgs = false;

		for ( const auto character : command )
		{
			switch ( character )
			{
				/* Identifier separator. */
				case '.' :
					if ( inArgs )
					{
						buffer += character;
					}
					else
					{
						Expression::extract(buffer, m_identifiers);
					}
					break;

				/* Start of arguments. */
				case '(' :
					inArgs = true;

					Expression::extract(buffer, m_commandName);
					break;

				/* Argument separator. */
				case ')' :
					inArgs = false;
					[[fallthrough]];
				case ',' :
					Expression::extract(buffer, arguments);
					break;

				default :
					buffer += character;
					break;
			}
		}

		/* Checks left character in 'buffer'. */
		if ( m_commandName.empty() )
		{
			Expression::extract(buffer, m_identifiers);
		}
		else
		{
			Expression::extract(buffer, arguments);
		}

		for ( auto & argument : arguments )
		{
			if ( argument.starts_with('"') || argument.starts_with('\'') )
			{
				const auto value = argument.substr(1, argument.length() - 2);

				m_arguments.emplace_back(value);

				continue;
			}

			if ( argument == "true" )
			{
				m_arguments.emplace_back(true);

				continue;
			}

			if ( argument == "false" )
			{
				m_arguments.emplace_back(true);

				continue;
			}

			if ( argument.find_first_of("0123456789") != std::string::npos )
			{
				if ( argument.find('.') != std::string::npos )
				{
					const auto value = std::stof(argument);

					m_arguments.emplace_back(value);
				}
				else
				{
					const auto value = std::stoi(argument);

					m_arguments.emplace_back(value);
				}

				continue;
			}
		}
	}

	std::string
	Expression::nextIdentifier () noexcept
	{
		/* If no identifier or current level is at the end, return an empty string. */
		if ( m_identifiers.empty() || m_identifierLevel >= m_identifiers.size() )
		{
			return {};
		}

		auto identifier = m_identifiers[m_identifierLevel];

		m_identifierLevel++;

		return identifier;
	}

	void
	Expression::extract (std::string & buffer, std::string & where) noexcept
	{
		if ( buffer.empty() )
		{
			return;
		}

		where = String::trim(buffer);

		buffer.clear();
	}

	void
	Expression::extract (std::string & buffer, std::vector< std::string > & where) noexcept
	{
		if ( buffer.empty() )
		{
			return;
		}

		auto cleanBuffer = String::trim(buffer);

		if ( !cleanBuffer.empty() )
		{
			where.emplace_back(cleanBuffer);
		}

		buffer.clear();
	}

	std::ostream &
	operator<< (std::ostream & out, const Expression & obj)
	{
		out << "Identifiers tree :\n";

		size_t num = 0;

		for ( const auto & identifier : obj.m_identifiers )
		{
			out << num++ << " : '" << identifier << "'\n";
		}

		out << "Function name : " << obj.m_commandName << "()\n";

		if ( !obj.m_arguments.empty() )
		{
			out << "Function parameters :\n";

			num = 0;

			for ( const auto & argument : obj.m_arguments )
			{
				switch ( argument.type() )
				{
					case ArgumentType::Boolean :
						out << num++ << " : " << argument.asBoolean() << '\n';
						break;

					case ArgumentType::Integer :
						out << num++ << " : " << argument.asInteger() << '\n';
						break;

					case ArgumentType::Float :
						out << num++ << " : " << argument.asFloat() << '\n';
						break;

					case ArgumentType::String :
						out << num++ << " : " << argument.asString() << '\n';
						break;

					case ArgumentType::Undefined :
					default:
						out << num++ << " : undefined" "\n";
						break;
				}
			}
		}

		return out;
	}
}
