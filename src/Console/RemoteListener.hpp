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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

/* Third-party inclusions. */
#include "asio.hpp"
#include "Network/asio_throw_exception.hpp"
#include "Network/GracefulCloser.hpp"

/* Local inclusions for usages. */
#include "SettingKeys.hpp"

namespace EmEn::Console
{
	/**
	 * @brief A TCP listener service for remote AI console control.
	 * @details Starts a server on a specified port (default 7777).
	 * Accepts incoming commands (line by line) and stores them in a thread-safe queue.
	 * Every request gets exactly one response line (RemoteProtocol); nothing is sent unsolicited.
	 */
	class EMEN_API RemoteListener final
	{
		friend class Session;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RemoteListener"};

			/** @brief Maximum number of pending commands in the queue, all clients together. */
			static constexpr size_t MaxPendingCommands{256};

			/** @brief Maximum length of a single command line; a longer line disconnects the client. */
			static constexpr size_t MaxLineLength{8192};

			/** @brief Maximum number of simultaneous clients; further connections are refused. */
			static constexpr size_t MaxClients{8};

			/**
			 * @brief Maximum number of pending commands of ONE client; exceeding it disconnects that client.
			 * @note A share of the global queue, so a flooding client can never make another client's
			 * command overflow it. Disconnecting (after a last error line) rather than answering and
			 * carrying on keeps the one-response-per-request order: an answer to request N+1 must never
			 * overtake the answer to request N that is still queued.
			 */
			static constexpr size_t MaxPendingCommandsPerClient{MaxPendingCommands / MaxClients};

			/** @brief Send timeout, so a peer that stops reading cannot block a write forever. */
			static constexpr uint32_t SendTimeoutMilliseconds{2000};

			/**
			 * @brief Constructs the remote listener service.
			 * @note The listener is an unauthenticated command channel. It binds to
			 * loopback by default; an address that does not parse is reported and
			 * replaced by loopback — never by the any-address.
			 * @param address The IP address to bind to. Default "127.0.0.1".
			 * @param port The TCP port to listen on. Default 7777.
			 */
			explicit RemoteListener (std::string address = DefaultConsoleRemoteListenerAddress, uint16_t port = DefaultConsoleRemoteListenerPort) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			RemoteListener (const RemoteListener & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			RemoteListener (RemoteListener && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return RemoteListener &
			 */
			RemoteListener & operator= (const RemoteListener & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return RemoteListener &
			 */
			RemoteListener & operator= (RemoteListener && copy) noexcept = delete;

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

			/**
			 * @brief Returns the address the listener is bound to.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			address () const noexcept
			{
				return m_address;
			}

			/**
			 * @brief Returns the port the listener is bound to.
			 * @return uint16_t
			 */
			[[nodiscard]]
			uint16_t
			port () const noexcept
			{
				return m_port;
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
			 * @brief Sends one response line to a specific client.
			 * @note The line must come from RemoteProtocol (one JSON object, no newline inside): this
			 * method only appends the terminating newline. Nothing is ever sent unsolicited — a
			 * client matches each line to its request in order, so a broadcast would desynchronize it.
			 * @param client The client socket to respond to.
			 * @param line A serialized response, without its terminating newline.
			 */
			void respond (const std::shared_ptr< asio::ip::tcp::socket > & client, const std::string & line) noexcept;

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

			/**
			 * @brief Returns whether a socket is still a client (not removed nor disconnected).
			 * @param socket The shared pointer to the client socket.
			 * @return bool
			 */
			[[nodiscard]]
			bool isClient (const std::shared_ptr< asio::ip::tcp::socket > & socket) noexcept;

			/**
			 * @brief Sends a last response line to a client, then closes its connection GRACEFULLY.
			 * @note Called from the network thread for a transport failure (line too long, too many
			 * pending commands). Everything queued for that client afterwards is dropped by respond().
			 * The client is removed from the set under the write lock, then its socket goes to the
			 * GracefulCloser (FIN after the line, bounded drain): closing over the bytes the client still
			 * sends was a RST, which on Windows discarded the last line (docs/subsystems/console/11-critical-points.md).
			 * @pre On the network thread (the GracefulCloser runs there).
			 * @param client The client socket.
			 * @param line The last line, from RemoteProtocol, without its terminating newline.
			 */
			void disconnect (const std::shared_ptr< asio::ip::tcp::socket > & client, const std::string & line) noexcept;

			std::string m_address;
			uint16_t m_port;
			asio::io_context m_ioContext;
			/** @brief The disconnected clients' graceful closes (network thread only). Declared after m_ioContext:
			 * destroyed first, it holds weak references only; the lingering sockets die with the context's handlers. */
			Base::Network::GracefulCloser m_gracefulCloser;
			std::unique_ptr< asio::ip::tcp::acceptor > m_acceptor;
			std::thread m_networkThread;
			std::mutex m_clientsMutex;
			std::set< std::shared_ptr< asio::ip::tcp::socket > > m_clients;
			/** @brief Serializes every write and close on the client sockets: the network thread (transport
			 * errors) and the main thread (responses) both write, and two interleaved writes would corrupt
			 * a response line. */
			std::mutex m_writeMutex;
			std::mutex m_queueMutex;
			std::queue< PendingCommand > m_commandsQueue;
			/** @brief Pending commands per client, guarded by m_queueMutex. */
			std::map< const asio::ip::tcp::socket *, size_t > m_pendingPerClient;
			std::atomic< bool > m_running{false};
	};
}
