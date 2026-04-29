/*
 * src/Net/TCPClient.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

/* Third party inclusions. */
#include "Libs/Network/asio_throw_exception.hpp"
#include "asio.hpp"

namespace EmEn::Net
{
	class TCPServer;

	/**
	 * @brief Cross-platform TCP client built on Asio.
	 * @details Exposes a simple blocking-with-timeout API on top of Asio so
	 * consumers do not have to deal with the Asio model directly. Suitable
	 * for printer protocols (MKS, Chitu), RTSP control channels, LAN gaming
	 * and any application that needs a stateful TCP connection.
	 * @note Each instance owns its own asio::io_context, which makes it safe
	 * to use multiple TCPClient instances independently and concurrently from
	 * different threads.
	 */
	class TCPClient final
	{
		friend class TCPServer;

		public:

			/** @brief Default connect timeout in milliseconds. */
			static constexpr uint32_t DefaultConnectTimeoutMs{5000};

			/** @brief Constructs a closed TCP client. */
			TCPClient () noexcept;

			/** @brief Destructor closes the socket if still connected. */
			~TCPClient () noexcept;

			/** @brief Non-copyable. */
			TCPClient (const TCPClient &) = delete;
			TCPClient & operator= (const TCPClient &) = delete;

			/** @brief Movable. */
			TCPClient (TCPClient && other) noexcept;
			TCPClient & operator= (TCPClient && other) noexcept;

			/**
			 * @brief Connects to a remote host with a timeout.
			 * @note Resolves the host through DNS (supports both IPv4 and IPv6
			 * endpoints) and tries them in order until one succeeds before the
			 * deadline. If the client is already connected, the previous
			 * connection is closed first.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param timeoutMs Connect timeout in milliseconds.
			 * @return bool True if the connection was established before the timeout.
			 */
			bool connect (const std::string & host, uint16_t port, uint32_t timeoutMs = DefaultConnectTimeoutMs) noexcept;

			/**
			 * @brief Closes the socket.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the socket is currently connected.
			 * @return bool
			 */
			[[nodiscard]]
			bool isConnected () const noexcept;

			/**
			 * @brief Sends raw data to the remote host (blocking).
			 * @note Performs a complete write — returns only when all bytes
			 * have been transmitted to the kernel send buffer or an error
			 * occurs. The kernel-level send timeout (see setSendTimeout())
			 * applies if set.
			 * @param data Pointer to the data buffer.
			 * @param length Number of bytes to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const void * data, size_t length) noexcept;

			/**
			 * @brief Sends a string to the remote host (blocking).
			 * @param data The string to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const std::string & data) noexcept;

			/**
			 * @brief Receives data from the remote host with a timeout.
			 * @note A single read of up to @a maxLength bytes. Returns as soon
			 * as any data is available — TCP is a stream, framing is the
			 * caller's responsibility.
			 * @param buffer The buffer to read into.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param timeoutMs Receive timeout in milliseconds (0 = block indefinitely).
			 * @return int Number of bytes received, 0 on timeout or peer
			 * graceful close, -1 on error.
			 */
			int receive (void * buffer, size_t maxLength, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Receives data as a string with a timeout.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param timeoutMs Receive timeout in milliseconds (0 = block indefinitely).
			 * @return std::string The data received (empty on timeout or error).
			 */
			[[nodiscard]]
			std::string receiveString (size_t maxLength, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Retrieves the local address and port the socket is bound to.
			 * @param address [out] The bound IP address.
			 * @param port [out] The bound port.
			 * @return bool True on success.
			 */
			bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

			/**
			 * @brief Retrieves the remote address and port of the connected peer.
			 * @param address [out] The peer IP address.
			 * @param port [out] The peer port.
			 * @return bool True on success.
			 */
			bool getRemoteAddress (std::string & address, uint16_t & port) const noexcept;

			/**
			 * @brief Enables or disables Nagle's algorithm on the socket (TCP_NODELAY).
			 * @note Disabling Nagle (enable = true) sends small packets immediately
			 * instead of coalescing them. Recommended for latency-sensitive
			 * traffic (game packets, RTSP control messages, short G-code commands).
			 * @param enable True to disable Nagle, false to keep it (the default).
			 * @return bool True on success.
			 */
			bool setNoDelay (bool enable) noexcept;

			/**
			 * @brief Enables or disables SO_KEEPALIVE on the socket.
			 * @note Recommended for long-lived sessions traversing NAT (printer
			 * sessions, LAN game lobbies). Detects silently dropped connections
			 * within a few minutes instead of waiting for the default TCP timeout.
			 * @param enable True to enable keep-alive probes.
			 * @param initialDelaySeconds Idle time before the first probe is sent.
			 * Best-effort — the kernel-level granularity varies between platforms.
			 * @return bool True on success.
			 */
			bool setKeepAlive (bool enable, uint32_t initialDelaySeconds = 7200) noexcept;

			/**
			 * @brief Sets the kernel-level receive timeout (SO_RCVTIMEO).
			 * @note This is independent of the per-call @a timeoutMs argument
			 * passed to receive(). It applies to underlying kernel reads when
			 * the user-space code performs a synchronous read without timeout.
			 * @param timeoutMs Receive timeout in milliseconds (0 = no timeout).
			 * @return bool True on success.
			 */
			bool setRecvTimeout (uint32_t timeoutMs) noexcept;

			/**
			 * @brief Sets the kernel-level send timeout (SO_SNDTIMEO).
			 * @param timeoutMs Send timeout in milliseconds (0 = no timeout).
			 * @return bool True on success.
			 */
			bool setSendTimeout (uint32_t timeoutMs) noexcept;

			/**
			 * @brief Returns the last error code recorded by the client.
			 * @note Useful to map low-level errors (connection refused, reset,
			 * timeout, host not found) to higher-level Node.js-like error
			 * codes in wrapping layers.
			 * @return std::error_code
			 */
			[[nodiscard]]
			std::error_code
			lastError () const noexcept
			{
				return m_lastError;
			}

		private:

			/**
			 * @brief Internal constructor used by TCPServer::accept() to wrap
			 * an accepted socket.
			 * @param ioContext An io_context that owns the socket. The socket
			 * has already been transferred to it via release()/assign().
			 * @param socket The accepted socket attached to @a ioContext.
			 */
			TCPClient (std::unique_ptr< asio::io_context > ioContext, std::unique_ptr< asio::ip::tcp::socket > socket) noexcept;

			std::unique_ptr< asio::io_context > m_ioContext;
			std::unique_ptr< asio::ip::tcp::socket > m_socket;
			std::error_code m_lastError;
	};
}
