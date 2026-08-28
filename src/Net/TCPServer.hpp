/*
 * src/Net/TCPServer.hpp
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
#include <memory>
#include <optional>
#include <string>
#include <system_error>

/* Local inclusions for usages. */
#include "TCPClient.hpp"

namespace EmEn::Net
{
	/**
	 * @brief Cross-platform TCP server built on Asio.
	 * @details Exposes a simple blocking-with-timeout accept() API on top of
	 * Asio. Accepted clients are returned as fully-owned TCPClient instances,
	 * each with its own io_context, so the TCPServer's lifetime is independent
	 * from the lifetime of the clients it produced.
	 * @note The expected usage is to spawn a dedicated thread that loops on
	 * accept() (with a short timeout to allow graceful shutdown) and either
	 * services each accepted client in-place or hands it off to another
	 * thread / connection manager. This pattern fits both the WebModule
	 * polling consumer and a LAN-game accept loop.
	 * @note Asio never reaches this header: the io_context and the acceptor live
	 * in a private implementation defined in the translation unit, so a consumer
	 * pays neither the asio.hpp parse nor its exception-hook prerequisites.
	 */
	class EMEN_LEAN_API TCPServer final
	{
		public:

			/** @brief Default listen backlog (kernel-defined maximum). */
			static const int DefaultBacklog;

			/** @brief Constructs a server that is not yet listening. */
			TCPServer () noexcept;

			/** @brief Destructor closes the acceptor if still open. */
			~TCPServer () noexcept;

			/** @brief Non-copyable. */
			TCPServer (const TCPServer &) = delete;
			TCPServer & operator= (const TCPServer &) = delete;

			/** @brief Movable. */
			TCPServer (TCPServer && other) noexcept;
			TCPServer & operator= (TCPServer && other) noexcept;

			/**
			 * @brief Starts listening on a port.
			 * @note When @a port is 0, the OS chooses a free ephemeral port;
			 * use getLocalAddress() to recover it. SO_REUSEADDR is enabled on
			 * a best-effort basis.
			 * @param port The local port to listen on (0 to let the OS pick).
			 * @param backlog Maximum number of pending connections in the
			 * accept queue.
			 * @param address The local address to bind to (empty or "0.0.0.0"
			 * for any interface, IPv4). Pass an explicit IPv6 address
			 * (e.g. "::") to listen in IPv6.
			 * @note Binding the IPv6 any-address ("::") requests a DUAL-STACK
			 * socket explicitly (IPV6_V6ONLY off), so IPv4 peers are accepted
			 * on every platform rather than only on those whose kernel happens
			 * to default that way. If the stack refuses, the listen FAILS
			 * instead of coming up IPv6-only and silently refusing half the
			 * network; bind "0.0.0.0" for a deliberate IPv4-only listen.
			 * @return bool True if the server is listening.
			 */
			bool listen (uint16_t port, int backlog = DefaultBacklog, const std::string & address = {}) noexcept;

			/**
			 * @brief Stops the server and closes the acceptor.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the server is currently listening.
			 * @return bool
			 */
			[[nodiscard]]
			bool isListening () const noexcept;

			/**
			 * @brief Accepts a new incoming connection.
			 * @note The returned TCPClient owns its own io_context — moving it
			 * around or destroying the TCPServer afterwards is safe.
			 * @param timeoutMs Accept timeout in milliseconds (0 = block indefinitely).
			 * @return std::optional< TCPClient > A fully-connected client on
			 * success, std::nullopt on timeout or error.
			 */
			std::optional< TCPClient > accept (uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Returns whether the last accept() returned empty because its deadline expired.
			 * @note accept() answers std::nullopt for a timeout and for a failure alike; this tells
			 * the two apart, so a polling loop does not report a timeout as an error. Meaningful
			 * right after an empty accept().
			 * @return bool
			 */
			[[nodiscard]]
			bool
			lastAcceptTimedOut () const noexcept
			{
				return m_lastAcceptTimedOut;
			}

			/**
			 * @brief Retrieves the local address and port the server is bound to.
			 * @param address [out] The bound IP address.
			 * @param port [out] The bound port (resolved if listen() was called with port = 0).
			 * @return bool True on success.
			 */
			bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

			/**
			 * @brief Returns the last error code recorded by the server.
			 * @return std::error_code
			 */
			[[nodiscard]]
			std::error_code
			lastError () const noexcept
			{
				return m_lastError;
			}

		private:

			/** @brief Asio-backed state (io_context + acceptor), confined to the TU. */
			struct Impl;

			std::unique_ptr< Impl > m_impl;
			std::error_code m_lastError;
			bool m_lastAcceptTimedOut{false};
	};
}
