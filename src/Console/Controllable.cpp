/*
 * src/Console/Controllable.cpp
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

#include "Controllable.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/BlobTrait.hpp"
#include "Libs/String.hpp"
#include "Controller.hpp"
#include "Tracer.hpp"

namespace EmEn::Console
{
	using namespace EmEn::Libs;

	constexpr auto TracerTag{"Controllable"};

	Controllable::Controllable (std::string consoleIdentifier) noexcept
		: m_identifier(std::move(consoleIdentifier))
	{

	}

	Controllable::~Controllable ()
	{
		auto * console = Controller::instance();

		/* Auto remove from console. */
		if ( console != nullptr )
		{
			console->remove(*this);
		}
	}

	bool
	Controllable::execute (Expression & expression, Outputs & outputs) noexcept
	{
		/* Checks for next identifier. */
		auto identifier = expression.nextIdentifier();

		/* If we got a next identifier. */
		if ( !identifier.empty() )
		{
			/* Checks for sub object */
			const auto objectIt = std::ranges::find_if(m_consoleObjects, [identifier] (const auto & objectIt) {
				return objectIt.first == identifier;
			});

			if ( objectIt != m_consoleObjects.cend() )
			{
				/* NOTE: Execute the object command bound. */
				return objectIt->second->execute(expression, outputs);
			}

			outputs.emplace_back(Severity::Error, std::stringstream{} << "Console object '" << identifier << "' not found !");

			return false;
		}

		/* If empty we are at the right level, and we try to execute the command. */

		/* Checks for built-in function about a level. */
		if ( this->checkBuiltInCommands(expression, outputs) )
		{
			return true;
		}

		/* Checks for object commands. */
		const auto commandIt = std::ranges::find_if(m_commands, [expression] (const auto & commandIt) {
			return commandIt.first == expression.commandName();
		});

		if ( commandIt != m_commands.cend() )
		{
			const auto & binding = commandIt->second.binding();

			return binding(expression.arguments(), outputs);
		}

		outputs.emplace_back(Severity::Error, std::stringstream{} << "Object command '" << expression.commandName() << "()' not found !");

		return false;
	}

	void
	Controllable::complete (Expression & expression, std::string & identifier, std::vector< std::string > & suggestions) const noexcept
	{
		identifier = expression.nextIdentifier();

		if ( identifier.empty() )
		{
			return;
		}

		/* For each register object in the console.
		 * If we got a perfect match, we stop the search. */
		if ( Controller::loopOverObjectsName(m_consoleObjects, expression, identifier, suggestions) )
		{
			return;
		}

		/* For each possible command. */
		for ( const auto & commandIt : m_commands )
		{
			/* Object name is smaller than input, skip it. */
			if ( identifier.length() > commandIt.first.length() )
			{
				continue;
			}

			/* Perfect match. */
			if ( identifier == commandIt.first )
			{
				continue;
			}

			/* Checks for mismatching. */
			auto mismatch = false;

			for ( size_t chr = 0; chr < identifier.length(); chr++ )
			{
				/* Object name mismatch characters from input. */
				if ( commandIt.first[chr] != identifier[chr] )
				{
					mismatch = true;

					break;
				}
			}

			if ( !mismatch )
			{
				suggestions.emplace_back(commandIt.first + "()");
			}
		}
	}

	void
	Controllable::bindCommand (const std::string & commandNames, const Binding & binding, const std::string & help) noexcept
	{
		const auto commandNamesList = String::explode(commandNames, ',', false);

		for ( const auto & commandName : commandNamesList )
		{
			const auto commandIt = m_commands.find(commandName);

			if ( commandIt != m_commands.cend() )
			{
				TraceError{TracerTag} << "Command '" << commandName << "' already exists !";

				continue;
			}

			m_commands.emplace(commandName, Command{binding, help});
		}
	}

	void
	Controllable::unbindCommand (const std::string & commandNames) noexcept
	{
		const auto commandNameList = String::explode(commandNames, ',', false);

		for ( const auto & commandName : commandNameList )
		{
			const auto commandIt = m_commands.find(commandName);

			if ( commandIt != m_commands.end() )
			{
				m_commands.erase(commandIt);
			}
		}
	}

	bool
	Controllable::registerToConsole () noexcept
	{
		if ( !Controller::instance()->add(*this) )
		{
			TraceError{TracerTag} << "Unable to register this object to the console.";

			return false;
		}

		this->onRegisterToConsole();

		return true;
	}

	bool
	Controllable::registerToObject (Controllable & object) noexcept
	{
		if ( object.m_consoleObjects.contains(m_identifier) )
		{
			TraceError{TracerTag} << "Sub object named '" << m_identifier << "' already exists !";

			return false;
		}

		object.m_consoleObjects.emplace(m_identifier, this);

		this->onRegisterToConsole();

		return true;
	}

	bool
	Controllable::checkBuiltInCommands (const Expression & expression, Outputs & outputs) const noexcept
	{
		if ( expression.commandName() == "lsfunc" )
		{
			std::stringstream message;

			for ( const auto & commandIt : m_commands )
			{
				message << commandIt.first << " (" << commandIt.second.help() << ")" "\n";
			}

			outputs.emplace_back(Severity::Info, message);

			return true;
		}

		if ( expression.commandName() == "lsobj" )
		{
			std::stringstream message;

			for ( const auto & objectIt : m_consoleObjects )
			{
				message << "'" << objectIt.first << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, message);

			return true;
		}

		return false;
	}
}
