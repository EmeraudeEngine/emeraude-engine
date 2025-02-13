/*
 * src/Controllable.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <map>
#include <string>

/* Local inclusions for usages. */
#include "Command.hpp"
#include "Expression.hpp"

namespace Emeraude::Console
{
	/**
	 * @brief Interface to register an object controllable with the console.
	 */
	class Controllable
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Controllable (const Controllable & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Controllable (Controllable && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Controllable &
			 */
			Controllable & operator= (const Controllable & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Controllable &
			 */
			Controllable & operator= (Controllable && copy) noexcept = default;

			/**
			 * @brief The destructor auto remove this object from the console.
			 */
			virtual ~Controllable ();

			/**
			 * @brief Returns the identifier of this object in the console.
			 * @return std::string
			 */
			[[nodiscard]]
			const std::string &
			identifier () const noexcept
			{
				return m_identifier;
			}

			/**
			 * @brief Register this controllable object below another one.
			 * @param object A reference to the parent object.
			 * @return bool
			 */
			bool registerToObject (Controllable & object) noexcept;

			/**
			 * @brief Executes an expression from the console.
			 * @note This is a recursive method.
			 * @param expression An expression object from the console.
			 * @param outputs A writable reference to a vector of console outputs.
			 * @return bool
			 */
			bool execute (Expression & expression, Outputs & outputs) noexcept;

			/**
			 * @brief Tries to complete an expression from the console.
			 * @param expression An expression object from the console.
			 * @param identifier The identifier of the controlled object.
			 * @param suggestions List of suggestions to complete the expression.
			 * @return void
			 */
			void complete (Expression & expression, std::string & identifier, std::vector< std::string > & suggestions) const noexcept;

		protected:

			/**
			 * @brief Constructor that identifies by a name the new controllable object.
			 * @param consoleIdentifier A string to for the name of the object [std::move].
			 */
			explicit Controllable (std::string consoleIdentifier) noexcept;

			/**
			 * @brief Register a command for this console controllable object.
			 * @param commandNames The way of calling the command inside the console.
			 * @param binding The binding to execute in the command.
			 * @param help A way to explain that command. By default, "NoHelp" will be display.
			 */
			void bindCommand (const std::string & commandNames, const Binding & binding, const std::string & help = {"No help"}) noexcept;

			/**
			 * @brief Removes a command from the console.
			 * @param commandNames The way of calling the command inside the console.
			 */
			void unbindCommand (const std::string & commandNames) noexcept;

			/**
			 * @brief Register this controllable object directly to the console.
			 * @return bool
			 */
			bool registerToConsole () noexcept;

		private:

			/**
			 * @brief checkBuiltInCommands
			 * @param expression A reference to a console expression.
			 * @param outputs A writable reference to a vector of console outputs.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkBuiltInCommands (const Expression & expression, Outputs & outputs) const noexcept;

			/**
			 * @brief Method to override to bind commands.
			 * @return void
			 */
			virtual void onRegisterToConsole () noexcept = 0;

			std::string m_identifier;
			std::map< std::string, Command > m_commands;
			std::map< std::string, Controllable * > m_consoleObjects;
	};
}
