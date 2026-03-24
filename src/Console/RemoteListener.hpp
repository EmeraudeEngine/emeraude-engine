/*
 * src/Console/RemoteListener.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#ifdef ASIO_ENABLED

/* STL inclusions. */
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

/* ASIO inclusions. */
#include "Libs/Network/asio_throw_exception.hpp"
#include <asio.hpp>

/* Local inclusions. */
#include "SettingKeys.hpp"

namespace EmEn::Console
{
	/**
	 * @brief A TCP listener service for remote AI console control.
	 * @details Starts a server on a specified port (default 7777).
	 * Accepts incoming commands (line by line) and stores them in a thread-safe queue.
	 * Also acts as a hub to broadcast messages back to all connected clients.
	 */
	class RemoteListener final
	{
		friend class Session;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RemoteListener"};

			/** @brief Maximum number of pending commands in the queue before dropping new ones. */
			static constexpr size_t MaxPendingCommands{256};

			/**
			 * @brief Constructs the remote listener service.
			 * @param port The TCP port to listen on. Default 7777.
			 */
			explicit RemoteListener (uint16_t port = DefaultConsoleRemoteListenerPort) noexcept;

			/**
			 * @brief Destructs the remote listener service.
			 */
			~RemoteListener ();

			[[nodiscard]]
			bool
			isRunning () const noexcept
			{
				return m_running;
			}

			/** @brief A pending command with its originating client socket. */
			struct PendingCommand
			{
				std::string command;
				std::shared_ptr< asio::ip::tcp::socket > client;
			};

			/**
			 * @brief Pops the oldest command from the queue.
			 * @param outCommand Will contain the command and client socket if successful.
			 * @return true if a command was available, false otherwise.
			 */
			[[nodiscard]]
			bool popCommand (PendingCommand & outCommand) noexcept;

			/**
			 * @brief Sends a response directly to a specific client (clean, no Tracer prefix).
			 * @param client The client socket to respond to.
			 * @param message The response message.
			 */
			void respond (const std::shared_ptr< asio::ip::tcp::socket > & client, const std::string & message) noexcept;

			/**
			 * @brief Broadcasts a message string to all connected clients.
			 * @param message A reference to the message to broadcast.
			 */
			void broadcast (const std::string & message) noexcept;

		private:

			/**
			 * @brief Starts accepting new TCP connections asynchronously.
			 */
			void accept () noexcept;

			/**
			 * @brief Enqueues a command received from a client.
			 * @param command A reference to the command string.
			 * @param client The socket of the client that sent the command.
			 */
			void enqueueCommand (const std::string & command, const std::shared_ptr< asio::ip::tcp::socket > & client) noexcept;

			/**
			 * @brief Removes a disconnected client from the set.
			 * @param socket The shared pointer to the client socket.
			 */
			void removeClient (const std::shared_ptr< asio::ip::tcp::socket > & socket) noexcept;

			uint16_t m_port;
			asio::io_context m_ioContext;
			std::unique_ptr< asio::ip::tcp::acceptor > m_acceptor;
			std::thread m_networkThread;
			std::mutex m_clientsMutex;
			std::set< std::shared_ptr< asio::ip::tcp::socket > > m_clients;
			std::mutex m_queueMutex;
			std::queue< PendingCommand > m_commandsQueue;
			std::atomic< bool > m_running{false};
	};
}

#endif // ASIO_ENABLED
