/*
 * src/Console/ControllableTrait.hpp
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
#include <cstddef>
#include <map>
#include <string>
#include <utility>

/* Local inclusions for usages. */
#include "Command.hpp"
#include "CommandResult.hpp"
#include "Expression.hpp"
#include "Parameter.hpp"
#include "TypedBinding.hpp"

namespace EmEn::Console
{
	/**
	 * @brief Interface to register an object controllable with the console.
	 */
	class EMEN_API ControllableTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ControllableTrait (const ControllableTrait & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ControllableTrait (ControllableTrait && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ControllableTrait &
			 */
			ControllableTrait & operator= (const ControllableTrait & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ControllableTrait &
			 */
			ControllableTrait & operator= (ControllableTrait && copy) noexcept = default;

			/**
			 * @brief The destructor auto removes this object from the console.
			 */
			virtual ~ControllableTrait ();

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
			 * @brief Returns the commands bound on this controllable.
			 * @note Used by the `help` built-in to walk the console tree.
			 * @return const std::map< std::string, Command > &
			 */
			[[nodiscard]]
			const std::map< std::string, Command > &
			commands () const noexcept
			{
				return m_commands;
			}

			/**
			 * @brief Returns the sub-objects registered under this controllable.
			 * @note Used by the `help` built-in to walk the console tree.
			 * @return const std::map< std::string, ControllableTrait * > &
			 */
			[[nodiscard]]
			const std::map< std::string, ControllableTrait * > &
			subObjects () const noexcept
			{
				return m_consoleObjects;
			}

			/**
			 * @brief Register this controllable object below another one.
			 * @param object A reference to the parent object.
			 * @return bool
			 */
			bool registerToObject (ControllableTrait & object) noexcept;

			/**
			 * @brief Removes this controllable object from its parent object, if any.
			 * @note Called automatically on destruction. Call it explicitly to detach
			 * a still-alive object from the console tree (e.g. a deactivated game act),
			 * so another object can register under the same identifier.
			 * @return void
			 */
			void unregisterFromParent () noexcept;

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
			explicit
			ControllableTrait (std::string consoleIdentifier) noexcept
				: m_identifier{std::move(consoleIdentifier)}
			{

			}

			/**
			 * @brief Registers a command: its parameters are deduced from the callable (the only form — every
			 * command is typed).
			 * @note The callable is a non-generic lambda taking bool, int32_t, float, std::string or
			 * Console::Argument (by value or const reference), std::optional of those for an omittable
			 * argument, and a trailing std::vector of those for a variadic one; it returns a CommandResult.
			 * Declare one Parameter per argument, in order — a different count does not compile.
			 * The arguments are validated and converted before the callable runs: a wrong type or a
			 * missing argument answers an error naming the parameter, and the callable never sees it.
			 * The help line is the description followed by a generated "Usage: name(a, b [, c])".
			 * @code
			 * this->bindCommand("resize", "Resizes the window.",
			 * 	{{"width", "Width in screen coordinates."}, {"height", "Height in screen coordinates."}},
			 * 	[this] (int32_t width, int32_t height) {
			 * 		...
			 * 		return Console::CommandResult::success("Window resized.");
			 * 	}, Console::CommandHint::Idempotent);
			 * @endcode
			 * @tparam ParameterCount Deduced from the Parameter list.
			 * @tparam Callable Deduced from the lambda.
			 * @param commandNames The way of calling the command (comma-separated aliases supported).
			 * @param description What the command does, one or two sentences ending with a period.
			 * @param parameters One declaration per argument of the callable, in order.
			 * @param callable The code to run [std::move].
			 * @param hints The behaviour hints. Default none.
			 */
			template< size_t ParameterCount, typename Callable >
			void
			bindCommand (const std::string & commandNames, const std::string & description, const Parameter (& parameters)[ParameterCount], Callable callable, CommandHint hints = CommandHint::None) noexcept
			{
				if constexpr ( !TypedBinding::TypedCommandCallable< Callable > )
				{
					static_assert(TypedBinding::AlwaysFalse< Callable >, "bindCommand(): the callable must be a non-generic lambda returning Console::CommandResult.");
				}
				else if constexpr ( TypedBinding::CallableTraits< Callable >::Arity != ParameterCount )
				{
					static_assert(TypedBinding::AlwaysFalse< Callable >, "bindCommand(): declare exactly one Parameter per argument of the callable, in order.");
				}
				else
				{
					using ArgumentTypes = typename TypedBinding::CallableTraits< Callable >::ArgumentTypes;

					static_assert(TypedBinding::variadicIsLast< ArgumentTypes >(std::make_index_sequence< ParameterCount >{}), "bindCommand(): only the last parameter may be a std::vector (variadic).");

					CommandSignature signature{TypedBinding::resolveParameters< ArgumentTypes >(parameters, std::make_index_sequence< ParameterCount >{}), hints};
					auto binding = TypedBinding::makeBinding(std::move(callable), signature);

					this->bindTypedCommand(commandNames, description, std::move(binding), std::move(signature));
				}
			}

			/**
			 * @brief Registers a command that takes no argument.
			 * @note Same contract as the overload with parameters; the callable takes nothing.
			 * @tparam Callable Deduced from the lambda.
			 * @param commandNames The way of calling the command (comma-separated aliases supported).
			 * @param description What the command does, one or two sentences ending with a period.
			 * @param callable The code to run [std::move].
			 * @param hints The behaviour hints. Default none.
			 */
			template< typename Callable >
			void
			bindCommand (const std::string & commandNames, const std::string & description, Callable callable, CommandHint hints = CommandHint::None) noexcept
			{
				if constexpr ( !TypedBinding::TypedCommandCallable< Callable > )
				{
					static_assert(TypedBinding::AlwaysFalse< Callable >, "bindCommand(): the callable must be a non-generic lambda returning Console::CommandResult.");
				}
				else if constexpr ( TypedBinding::CallableTraits< Callable >::Arity != 0 )
				{
					static_assert(TypedBinding::AlwaysFalse< Callable >, "bindCommand(): a callable that takes arguments needs its Parameter list.");
				}
				else
				{
					CommandSignature signature{{}, hints};
					auto binding = TypedBinding::makeBinding(std::move(callable), signature);

					this->bindTypedCommand(commandNames, description, std::move(binding), std::move(signature));
				}
			}

			/**
			 * @brief Removes a command from the console.
			 * @param commandNames The way of calling the command inside the console.
			 */
			void unbindCommand (const std::string & commandNames) noexcept;

			/**
			 * @brief Register this controllable object directly in the console.
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
			 * @brief Registers a typed command once its binding and signature are built.
			 * @note Refuses (traced error, command not bound) a signature whose declaration is invalid.
			 * @param commandNames The way of calling the command (comma-separated aliases supported).
			 * @param description What the command does.
			 * @param binding The validating binding [std::move].
			 * @param signature The resolved signature [std::move].
			 * @return void
			 */
			void bindTypedCommand (const std::string & commandNames, const std::string & description, Binding binding, CommandSignature signature) noexcept;

			/**
			 * @brief Method to override to bind commands.
			 * @return void
			 */
			virtual void onRegisterToConsole () noexcept = 0;

			std::string m_identifier;
			std::map< std::string, Command > m_commands;
			std::map< std::string, ControllableTrait * > m_consoleObjects;
			/** @brief The parent object this controllable is registered under (raw back-pointer,
			 * cleared in both directions on destruction so the console tree never holds a
			 * dangling entry — a stale entry means every command dispatched to that identifier
			 * executes on freed memory). */
			ControllableTrait * m_parentObject{nullptr};
			/** @brief Whether onRegisterToConsole() already ran: commands are bound once for
			 * the object's lifetime, not per registration (an object can unregister and
			 * re-register, e.g. a game act cycling active/inactive). */
			bool m_commandsBound{false};
	};
}
