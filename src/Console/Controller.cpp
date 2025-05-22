/*
 * src/Console/Controller.cpp
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

#include "Controller.hpp"

/* Local inclusions. */
#include "Libs/String.hpp"
#include "PrimaryServices.hpp"
#include "Controllable.hpp"
#include "Expression.hpp"

namespace EmEn::Console
{
	using namespace EmEn::Libs;

	const size_t Controller::ClassUID{getClassUID(ClassId)};
	Controller * Controller::s_instance{nullptr};

	Controller::Controller (PrimaryServices & primaryServices) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices)
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;
	}

	Controller::~Controller ()
	{
		s_instance = nullptr;
	}

	bool
	Controller::usable () const noexcept
	{
		if ( !m_flags[ServiceInitialized] )
		{
			return false;
		}

		return !m_consoleObjects.empty();
	}

	bool
	Controller::onInitialize () noexcept
	{
		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Controller::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		m_consoleObjects.clear();

		return true;
	}

	bool
	Controller::add (Controllable & controllable) noexcept
	{
		if ( m_consoleObjects.contains(controllable.identifier()) )
		{
			TraceError{ClassId} << "Console object named '" << controllable.identifier() << "' already exists !";

			return false;
		}

		return m_consoleObjects.emplace(controllable.identifier(), &controllable).second;
	}

	bool
	Controller::remove (const std::string & identifier) noexcept
	{
		const auto objectIt = m_consoleObjects.find(identifier);

		if ( objectIt == m_consoleObjects.cend() )
		{
			return false;
		}

		m_consoleObjects.erase(objectIt);

		return true;
	}


	bool
	Controller::remove (const Controllable & pointer) noexcept
	{
		for ( auto it = m_consoleObjects.begin(); it != m_consoleObjects.end(); ++it )
		{
			if ( it->second == &pointer )
			{
				m_consoleObjects.erase(it);

				return true;
			}
		}

		return false;
	}

	void
	Controller::complete (std::string & input) const noexcept
	{
		if ( input.empty() )
		{
			return;
		}

		/* Checks form multiple commands. */
		const auto commandsList = String::explode(input, ';', false);

		/* We only check the last term. */
		Expression expression(*commandsList.crbegin());

		if ( expression.isValid() )
		{
			return;
		}

		auto identifier = expression.nextIdentifier();

		std::vector< std::string > terms;

		/* For each register object in the console. */
		loopOverObjectsName(m_consoleObjects, expression, identifier, terms);

		/* Shows the result */
		switch ( terms.size() )
		{
			case 1 :
				input += String::right(*terms.begin(), identifier);

			break;

			case 0 :

				Tracer::warning(ClassId, "No match found !");

			break;

			default:

				for ( auto & term : terms )
				{
					Tracer::info(ClassId, term);
				}

			break;
		}
	}

	bool
	Controller::executeCommand (const std::string & command, Outputs & outputs) noexcept
	{
		TraceInfo{ClassId} << "Executing command: " << command;

		/* Checks for built-in command first ! */
		if ( command.find('.') == std::string::npos )
		{
			return this->executeBuiltInCommand(command, outputs);
		}

		/* Parse the command expression. */
		Expression expression(command);

		if ( !expression.isValid() )
		{
			outputs.emplace_back(Severity::Error, "Invalid command !");

			return false;
		}

		/* Gets the first identifier.
		 * NOTE: It is useless to check if it's empty here. */
		const auto identifier = expression.nextIdentifier();

		/* Retrieve the base object. */
		const auto objectIt = m_consoleObjects.find(identifier);

		if ( objectIt == m_consoleObjects.cend() )
		{
			outputs.emplace_back(Severity::Error, std::stringstream{} << "Console object '" << identifier << "' not found !");

			return false;
		}

		/* Send the command to this object. */
		return objectIt->second->execute(expression, outputs);
	}

	bool
	Controller::loopOverObjectsName (const std::map< std::string, Controllable * > & objects, Expression & expression, std::string & identifier, std::vector< std::string > & suggestions) noexcept
	{
		for ( const auto & [name, controllable] : objects )
		{
			/* Object name is smaller than input, skip it. */
			if ( identifier.length() > name.length() )
			{
				continue;
			}

			/* Perfect match. */
			if ( identifier == name )
			{
				/* As we check a new depth, we don't want old suggestions */
				suggestions.clear();

				controllable->complete(expression, identifier, suggestions);

				return true;
			}

			/* Checks for mismatching. */
			auto mismatch = false;

			for ( size_t chr = 0; chr < identifier.length(); chr++ )
			{
				/* Object name mismatch characters from input. */
				if ( name[chr] != identifier[chr] )
				{
					mismatch = true;

					break;
				}
			}

			if ( !mismatch )
			{
				suggestions.emplace_back(name);
			}
		}

		return false;
	}

	bool
	Controller::executeBuiltInCommand (const std::string & command, Outputs & outputs) noexcept
	{
		if ( command == "help" || command == "lsfunc()" )
		{
			outputs.emplace_back(Severity::Info, "Help command invoked !");

			return true;
		}

		if ( command == "listObjects" || command == "lsobj()" )
		{
			std::stringstream message;

			for ( const auto & objectName : std::ranges::views::keys(m_consoleObjects) )
			{
				message << "'" << objectName << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, message);

			return true;
		}

		if ( command == "printArguments" )
		{
			outputs.emplace_back(Severity::Info, std::stringstream{} << m_primaryServices.arguments());

			return true;
		}

		if ( command == "printFileSystem" )
		{
			outputs.emplace_back(Severity::Info, std::stringstream{} << m_primaryServices.fileSystem());

			return true;
		}

		if ( command == "printCoreSettings" )
		{
			outputs.emplace_back(Severity::Info, std::stringstream{} << m_primaryServices.settings());

			return true;
		}

		if ( command == "exit" || command == "quit" || command == "shutdown" )
		{
			outputs.emplace_back(Severity::Success, "Goodbye !");

			/* FIXME: Launch a notification */
			/*this->fire([] (double) {
				Core::instance()->stop();
			}, 3000.0);*/

			return true;
		}

		if ( command == "hardExit" )
		{
			outputs.emplace_back(Severity::Warning, "Wild exit command invoked !");

			this->fire([] (Time::TimerID /*timerID*/) {
				std::terminate();

				return true;
			}, 3000);

			return true;
		}

		outputs.emplace_back(Severity::Error, "Invalid command !");

		return false;
	}
}
