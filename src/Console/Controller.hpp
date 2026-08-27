/*
 * src/Console/Controller.hpp
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
#include <optional>
#include <string>
#include <utility>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ObservableTrait.hpp"
#include "Time/EventTrait.hpp"

/* Local inclusions for usages. */
#include "ControllableTrait.hpp"
#include "Output.hpp"
#include "RemoteListener.hpp"

/* Forward declarations */
namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn::Console
{
	/**
	 * @brief The console controller service class.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObservableTrait This is a service is observable.
	 * @extends EmEn::Base::Time::EventTrait This service needs to delay some behavior.
	 */
	class EMEN_API Controller final : public ServiceInterface, public Base::ObservableTrait, private Base::Time::EventTrait< uint32_t, std::milli >
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConsoleControllerService"};

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
			{
				Exit,
				HardExit,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the console controller.
			 * @param primaryServices A reference to primary services.
			 */
			explicit
			Controller (PrimaryServices & primaryServices) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices}
			{
				if ( s_instance != nullptr )
				{
					std::cerr << "Controller::Controller(), constructor called twice !" "\n";

					std::terminate();
				}

				s_instance = this;
			}

			/**
			 * @brief Destructs the console controller.
			 */
			~Controller () override
			{
				s_instance = nullptr;
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Starts the remote listener on an endpoint, replacing a running one.
			 * @note The boot-time policy is Core/Console/EnableRemoteListener; this is the live
			 * path (Shift+F10 dialog, restartRemoteConsole console command). Nothing is written to
			 * the settings: the console is closed again at the next launch.
			 * @param address The bind address ("127.0.0.1", "0.0.0.0", "::" ...).
			 * @param port The TCP port.
			 * @return bool True when the listener is up on that endpoint.
			 */
			bool startRemoteListener (const std::string & address, uint16_t port) noexcept;

			/**
			 * @brief Stops the remote listener, if any. Clients are disconnected.
			 * @return void
			 */
			void stopRemoteListener () noexcept;

			/**
			 * @brief Returns whether the remote listener is up.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRemoteListenerRunning () const noexcept
			{
				return m_remoteListener != nullptr && m_remoteListener->isRunning();
			}

			/**
			 * @brief Returns the endpoint the remote listener is bound to, "address:port", empty when down.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string remoteListenerEndpoint () const noexcept;

			/**
			 * @brief Returns the bind address configured in the settings (Core/Console/RemoteListenerAddress).
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			defaultRemoteListenerAddress () const noexcept
			{
				return m_remoteListenerAddress;
			}

			/**
			 * @brief Returns the port configured in the settings (Core/Console/RemoteListenerPort).
			 * @return uint16_t
			 */
			[[nodiscard]]
			uint16_t
			defaultRemoteListenerPort () const noexcept
			{
				return m_remoteListenerPort;
			}

			/**
			 * @brief Schedules a restart of the remote listener for the beginning of the next poll().
			 * @note For a command executed BY the listener: replacing it while poll() is draining its
			 * queue would destroy the socket the response goes to. The response is sent on the current
			 * endpoint, the switch happens one cycle later.
			 * @param address The bind address.
			 * @param port The TCP port.
			 * @return void
			 */
			void requestRemoteListenerRestart (const std::string & address, uint16_t port) noexcept;

			/**
			 * @brief Parses a user-typed endpoint: "7777", "0.0.0.0:7777" or "[::1]:7777".
			 * @param input The typed text.
			 * @param defaultAddress The address used when the input holds a bare port.
			 * @param address [out] The bind address.
			 * @param port [out] The port (1-65535).
			 * @return bool False when the text is not a valid endpoint.
			 */
			[[nodiscard]]
			static bool parseEndpoint (const std::string & input, const std::string & defaultAddress, std::string & address, uint16_t & port) noexcept;

			/**
			 * @brief Adds a controllable object to the console.
			 * @param controllable A reference to the controllable object to add.
			 * @return bool
			 */
			bool add (ControllableTrait & controllable) noexcept;

			/**
			 * @brief Removes a controllable object from the console using the identifier.
			 * @param identifier The identifier of the controllable object.
			 * @return bool
			 */
			bool remove (const std::string & identifier) noexcept;

			/**
			 * @brief Removes a controllable object from the console.
			 * @param pointer A reference to the controllable object to remove.
			 * @return bool
			 */
			bool remove (const ControllableTrait & pointer) noexcept;

			/**
			 * @brief Tries to guess the next term in the console input.
			 * @param input A writable reference to a string.
			 * @return void
			 */
			void complete (std::string & input) const noexcept;

			/**
			 * @brief Executes a command.
			 * @param fullCommand A reference to a string holding the raw command.
			 * @return void
			 */
			void executeCommand (const std::string & fullCommand) noexcept;

			/**
			 * @brief Executes a command and collects outputs.
			 * @param command A reference to a string holding the raw command.
			 * @param outputs A writable reference to a vector of console outputs.
			 * @return bool
			 */
			bool executeCommand (const std::string & command, Outputs & outputs) noexcept;

			/** @brief Callback type for JSON scene descriptions received via TCP. */
			using JsonHandler = std::function< bool (const std::string &, Outputs &) >;

			/**
			 * @brief Polls pending remote console commands.
			 */
			void poll () noexcept;

			/**
			 * @brief Sets a handler for JSON input received via TCP (lines starting with '{').
			 * @param handler The callback function.
			 */
			void
			setJsonHandler (JsonHandler handler) noexcept
			{
				m_jsonHandler = std::move(handler);
			}

			/**
			 * @brief Returns the instance of the console controller.
			 * @todo This method must be removed!
			 * @return Controller *
			 */
			//[[deprecated("This method must be removed !")]]
			[[nodiscard]]
			static
			Controller *
			instance () noexcept
			{
				return s_instance; // FIXME: Remove this
			}

			/**
			 * @brief Loop over object names.
			 * FIXME Checks this method, this should be non-static.
			 * @param objects The list of register objects to the console.
			 * @param expression A writable reference to a console expression.
			 * @param identifier A writable reference to a string.
			 * @param suggestions A writable reference to a string list to set found suggestions.
			 * @return bool
			 */
			static bool loopOverObjectsName (const std::map< std::string, ControllableTrait * > & objects, Expression & expression, std::string & identifier, std::vector< std::string > & suggestions) noexcept;

			/**
			 * @brief Recursively appends the help tree of a controllable to an output stream.
			 * @note Public so ControllableTrait::checkBuiltInCommands can delegate scoped `.help()` calls here.
			 * @param controllable A reference to the controllable to dump.
			 * @param out A writable reference to the output string stream.
			 * @param path The dotted path accumulated so far (e.g. "Core.RendererService").
			 */
			static void dumpControllable (const ControllableTrait & controllable, std::stringstream & out, const std::string & path) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Checks and execute built-in console commands such as "help".
			 * @param command A reference to a string holding the raw command.
			 * @param outputs A writable reference to a vector of console outputs.
			 * @return bool
			 */
			[[nodiscard]]
			bool executeBuiltInCommand (const std::string & command, Outputs & outputs) noexcept;

			static constexpr auto InputTextName{"Input"};
			static constexpr auto OutputTextName{"Output"};

			static Controller * s_instance;

			PrimaryServices & m_primaryServices;
			std::map< std::string, ControllableTrait * > m_consoleObjects;
			std::vector< std::string > m_history;
			std::unique_ptr< RemoteListener > m_remoteListener;
			std::string m_remoteListenerAddress;
			std::optional< std::pair< std::string, uint16_t > > m_pendingRemoteListenerRestart;
			JsonHandler m_jsonHandler;
			uint16_t m_remoteListenerPort{0};
			bool m_directInputWasEnabled{false};
			bool m_pointerWasLocked{false};
	};
}
