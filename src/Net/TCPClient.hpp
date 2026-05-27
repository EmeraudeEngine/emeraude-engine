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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <system_error>

namespace EmEn::Net
{
	class TCPServer;

	/**
	 * @brief Cross-platform TCP client built on raw kernel sockets.
	 * @details Exposes a simple blocking-with-timeout API on top of native
	 * sockets. Asio is used only during connect() for DNS resolution and a
	 * timed async_connect; once the connection is established, the kernel
	 * handle is detached from Asio and managed directly. This makes the
	 * client safe for full-duplex use from two threads (one send thread,
	 * one receive-polling thread) without hitting Asio's non-thread-safe
	 * userland state on win_iocp_socket_service.
	 * Suitable for printer protocols (MKS, Chitu), RTSP control channels,
	 * LAN gaming and any application that needs a stateful TCP connection.
	 * @note Multiple TCPClient instances are fully independent and safe to
	 * use concurrently from different threads.
	 */
	class TCPClient final
	{
		friend class TCPServer;

		public:

			/** @brief Default connect timeout in milliseconds. */
			static constexpr uint32_t DefaultConnectTimeoutMs{5000};

			/** @brief Platform-erased native socket handle.
			 * @note std::intptr_t fits both POSIX `int` and Windows `SOCKET`
			 * (which is a `UINT_PTR`). The actual platform-specific cast is
			 * performed at the syscall boundary inside the implementation. */
			using native_handle_type = std::intptr_t;

			/** @brief Sentinel for an invalid handle.
			 * @note `-1` matches POSIX `int -1` and Windows `INVALID_SOCKET`
			 * (which is `(SOCKET)~0`) once cast through `intptr_t`. */
			static constexpr native_handle_type InvalidHandle{-1};

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
			 * connection is closed first. On success, the underlying kernel
			 * handle is detached from Asio and switched to blocking mode.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param timeoutMs Connect timeout in milliseconds.
			 * @return bool True if the connection was established before the timeout.
			 */
			bool connect (const std::string & host, uint16_t port, uint32_t timeoutMs = DefaultConnectTimeoutMs) noexcept;

			/**
			 * @brief Closes the socket.
			 * @note Performs a two-phase close: first a `shutdown(SHUT_RDWR)`
			 * under a shared lock to wake up any thread blocked in `::recv()`
			 * or `::send()` on this handle, then an exclusive lock that waits
			 * for those threads to release the kernel handle before calling
			 * `::closesocket()`/`::close()`. This eliminates the
			 * close-during-receive race on Windows.
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
			 * applies if set; partial writes are returned on timeout.
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
			 * caller's responsibility. The per-call timeout is enforced via
			 * `SO_RCVTIMEO` (save/restore around the call).
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
			 * @note Independent of the per-call @a timeoutMs argument passed
			 * to receive() (which save/restores around its own value).
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
			 * a kernel socket handle that has already been detached from Asio
			 * and switched to blocking mode.
			 * @param handle The raw socket handle.
			 */
			explicit TCPClient (native_handle_type handle) noexcept;

			/* Per-instance shared mutex. Held shared by send/receive (which can
			 * legitimately run concurrently — the kernel handles `recv()` and
			 * `send()` atomically on the same fd) and held exclusive by close()
			 * so the handle can be invalidated only after every in-flight op
			 * has returned from its syscall. */
			std::unique_ptr< std::shared_mutex > m_handleMutex;
			native_handle_type m_handle{InvalidHandle};
			std::error_code m_lastError;
	};
}
