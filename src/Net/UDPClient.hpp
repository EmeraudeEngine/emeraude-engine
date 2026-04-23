/*
 * src/Net/UDPClient.hpp
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
#include <map>
#include <string>
#include <vector>

namespace EmEn::Net
{
	/**
	 * @brief Represents a single SSDP device response.
	 */
	struct SSDPDevice
	{
		std::string address;
		uint16_t port{0};
		std::map< std::string, std::string > headers;
	};

	/**
	 * @brief Cross-platform UDP client for sending/receiving datagrams and SSDP discovery.
	 * @note Uses raw BSD sockets on Linux/macOS and WinSock on Windows.
	 * Provides both a stateful socket (open/bind/send/receive/close) and a
	 * self-contained static SSDP discovery method.
	 */
	class UDPClient final
	{
		public:

			static constexpr int DefaultSSDPTimeoutSeconds{5};

			UDPClient () noexcept = default;

			/** @brief Destructor closes the socket if still open. */
			~UDPClient () noexcept;

			/** @brief Non-copyable. */
			UDPClient (const UDPClient &) = delete;
			UDPClient & operator= (const UDPClient &) = delete;

			/** @brief Movable. */
			UDPClient (UDPClient && other) noexcept;
			UDPClient & operator= (UDPClient && other) noexcept;

			/**
			 * @brief Opens a UDP socket.
			 * @return bool True if the socket was created successfully.
			 */
			bool open () noexcept;

			/**
			 * @brief Binds the socket to a local port for receiving.
			 * @param port The local port to bind to.
			 * @param address The local address to bind to (empty or "0.0.0.0" for any).
			 * @return bool True if binding succeeded.
			 */
			bool bind (uint16_t port, const std::string & address = {}) noexcept;

			/**
			 * @brief Closes the socket.
			 * @return void
			 */
			void close () noexcept;

			/**
			 * @brief Returns whether the socket is currently open.
			 * @return bool
			 */
			[[nodiscard]]
			bool isOpen () const noexcept;

			/**
			 * @brief Sends raw data to a remote host.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param data Pointer to the data buffer.
			 * @param length Number of bytes to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const std::string & host, uint16_t port, const void * data, size_t length) noexcept;

			/**
			 * @brief Sends a string to a remote host.
			 * @param host The destination IP address or hostname.
			 * @param port The destination port.
			 * @param data The string to send.
			 * @return int Number of bytes sent, or -1 on error.
			 */
			int send (const std::string & host, uint16_t port, const std::string & data) noexcept;

			/**
			 * @brief Receives data from the socket.
			 * @param buffer The buffer to read into.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param senderAddress [out] The sender's IP address.
			 * @param senderPort [out] The sender's port.
			 * @param timeoutMs Receive timeout in milliseconds (0 = non-blocking).
			 * @return int Number of bytes received, 0 if timeout/no data, or -1 on error.
			 */
			int receive (void * buffer, size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Receives data as a string from the socket.
			 * @param maxLength Maximum number of bytes to receive.
			 * @param senderAddress [out] The sender's IP address.
			 * @param senderPort [out] The sender's port.
			 * @param timeoutMs Receive timeout in milliseconds (0 = non-blocking).
			 * @return std::string The data received (may be empty if no data available).
			 */
			[[nodiscard]]
			std::string receiveString (size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs = 0) noexcept;

			/**
			 * @brief Retrieves the local address and port the socket is bound to.
			 * @param address [out] The bound IP address.
			 * @param port [out] The bound port.
			 * @return bool True if the address was retrieved successfully.
			 */
			bool getLocalAddress (std::string & address, uint16_t & port) const noexcept;

			/**
			 * @brief Enables or disables the SO_BROADCAST socket option.
			 * @param enable True to enable broadcast, false to disable.
			 * @return bool True if the option was set successfully.
			 */
			bool setBroadcast (bool enable) noexcept;

			/**
			 * @brief Performs an SSDP M-SEARCH and collects responses (self-contained, no instance state needed).
			 * @note Creates a temporary socket internally. Uses raw UDP multicast.
			 * @param searchTarget The ST header value (e.g., "ssdp:all", "urn:schemas-upnp-org:device:Printer:1").
			 * @param timeoutSeconds How long to listen for responses.
			 * @return std::vector< SSDPDevice > The discovered devices.
			 */
			[[nodiscard]]
			static std::vector< SSDPDevice > ssdpDiscover (const std::string & searchTarget, int timeoutSeconds = DefaultSSDPTimeoutSeconds) noexcept;

		private:

#ifdef _WIN32
			uintptr_t m_socket{~uintptr_t{0}};  /* SOCKET (UINT_PTR), INVALID_SOCKET = ~0. */
#else
			int m_fd{-1};
#endif
	};
}
