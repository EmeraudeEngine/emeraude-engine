/*
 * src/Net/UDPClient.cpp
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

#include "UDPClient.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <sstream>
#include <utility>

/* Platform-specific socket inclusions. */
#ifdef _WIN32
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <WinSock2.h>
	#include <WS2tcpip.h>

	using SocketType = SOCKET;
	static constexpr SocketType InvalidSocket = INVALID_SOCKET;
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>

	using SocketType = int;
	static constexpr SocketType InvalidSocket = -1;
#endif

namespace EmEn::Net
{
	/* ---- Platform helpers ---- */

	static void
	platformCloseSocket (SocketType sock) noexcept
	{
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}

#ifdef _WIN32
	static bool
	ensureWsa () noexcept
	{
		static bool initialized = false;

		if ( !initialized )
		{
			WSADATA wsaData{};

			if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 )
			{
				return false;
			}

			initialized = true;
		}

		return true;
	}
#endif

	/* ---- SSDP helpers (kept private to this TU) ---- */

	static constexpr auto SSDPMulticastAddress{"239.255.255.250"};
	static constexpr uint16_t SSDPMulticastPort{1900};
	static constexpr int MaxMX{3};

	static std::string
	buildMSearchPacket (const std::string & searchTarget, int mx) noexcept
	{
		std::ostringstream packet;
		packet
			<< "M-SEARCH * HTTP/1.1\r\n"
			<< "HOST: " << SSDPMulticastAddress << ":" << SSDPMulticastPort << "\r\n"
			<< "MAN: \"ssdp:discover\"\r\n"
			<< "MX: " << mx << "\r\n"
			<< "ST: " << searchTarget << "\r\n"
			<< "\r\n";

		return packet.str();
	}

	static std::map< std::string, std::string >
	parseSSDPHeaders (const std::string & response) noexcept
	{
		std::map< std::string, std::string > headers;
		std::istringstream stream(response);
		std::string line;

		/* Skip the status line (e.g., "HTTP/1.1 200 OK"). */
		if ( std::getline(stream, line) )
		{
			if ( const auto firstSpace = line.find(' '); firstSpace != std::string::npos )
			{
				const auto secondSpace = line.find(' ', firstSpace + 1);

				headers["_STATUS"] = line.substr(firstSpace + 1, secondSpace != std::string::npos ? secondSpace - firstSpace - 1 : std::string::npos);
			}
		}

		/* Parse headers. */
		while ( std::getline(stream, line) )
		{
			/* Remove trailing \r if present. */
			if ( !line.empty() && line.back() == '\r' )
			{
				line.pop_back();
			}

			if ( line.empty() )
			{
				break;
			}

			if ( const auto colonPos = line.find(':'); colonPos != std::string::npos )
			{
				auto key = line.substr(0, colonPos);
				auto value = line.substr(colonPos + 1);

				/* Trim leading whitespace from value. */
				if ( const auto valueStart = value.find_first_not_of(' '); valueStart != std::string::npos )
				{
					value = value.substr(valueStart);
				}

				/* Convert key to uppercase for case-insensitive matching. */
				std::ranges::transform(key, key.begin(), ::toupper);

				headers[key] = value;
			}
		}

		return headers;
	}

	/* ---- UDPClient lifecycle ---- */

	UDPClient::~UDPClient () noexcept
	{
		close();
	}

	UDPClient::UDPClient (UDPClient && other) noexcept
#ifdef _WIN32
		: m_socket(std::exchange(other.m_socket, ~uintptr_t{0}))
#else
		: m_fd(std::exchange(other.m_fd, -1))
#endif
	{
	}

	UDPClient &
	UDPClient::operator= (UDPClient && other) noexcept
	{
		if ( this != &other )
		{
			close();

#ifdef _WIN32
			m_socket = std::exchange(other.m_socket, ~uintptr_t{0});
#else
			m_fd = std::exchange(other.m_fd, -1);
#endif
		}

		return *this;
	}

	bool
	UDPClient::open () noexcept
	{
		if ( isOpen() )
		{
			return true;
		}

#ifdef _WIN32
		if ( !ensureWsa() )
		{
			return false;
		}

		const auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if ( sock == INVALID_SOCKET )
		{
			return false;
		}

		m_socket = static_cast< uintptr_t >(sock);
#else
		m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if ( m_fd == -1 )
		{
			return false;
		}
#endif

		/* Allow address reuse. */
		int reuse = 1;

#ifdef _WIN32
		setsockopt(static_cast< SocketType >(m_socket), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&reuse), sizeof(reuse));
#else
		setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

		return true;
	}

	bool
	UDPClient::bind (uint16_t port, const std::string & address) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		struct sockaddr_in bindAddr{};
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(port);

		if ( address.empty() || address == "0.0.0.0" )
		{
			bindAddr.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			if ( inet_pton(AF_INET, address.c_str(), &bindAddr.sin_addr) != 1 )
			{
				return false;
			}
		}

#ifdef _WIN32
		return ::bind(static_cast< SocketType >(m_socket), reinterpret_cast< const struct sockaddr * >(&bindAddr), sizeof(bindAddr)) == 0;
#else
		return ::bind(m_fd, reinterpret_cast< const struct sockaddr * >(&bindAddr), sizeof(bindAddr)) == 0;
#endif
	}

	void
	UDPClient::close () noexcept
	{
#ifdef _WIN32
		if ( m_socket != ~uintptr_t{0} )
		{
			platformCloseSocket(static_cast< SocketType >(m_socket));
			m_socket = ~uintptr_t{0};
		}
#else
		if ( m_fd != -1 )
		{
			platformCloseSocket(m_fd);
			m_fd = -1;
		}
#endif
	}

	bool
	UDPClient::isOpen () const noexcept
	{
#ifdef _WIN32
		return m_socket != ~uintptr_t{0};
#else
		return m_fd != -1;
#endif
	}

	/* ---- Send / Receive ---- */

	int
	UDPClient::send (const std::string & host, uint16_t port, const void * data, size_t length) noexcept
	{
		if ( !isOpen() )
		{
			return -1;
		}

		struct sockaddr_in dest{};
		dest.sin_family = AF_INET;
		dest.sin_port = htons(port);

		if ( inet_pton(AF_INET, host.c_str(), &dest.sin_addr) != 1 )
		{
			return -1;
		}

#ifdef _WIN32
		const auto result = sendto(
			static_cast< SocketType >(m_socket),
			static_cast< const char * >(data),
			static_cast< int >(length),
			0,
			reinterpret_cast< const struct sockaddr * >(&dest),
			sizeof(dest)
		);
#else
		const auto result = sendto(
			m_fd,
			data,
			length,
			0,
			reinterpret_cast< const struct sockaddr * >(&dest),
			sizeof(dest)
		);
#endif

		return static_cast< int >(result);
	}

	int
	UDPClient::send (const std::string & host, uint16_t port, const std::string & data) noexcept
	{
		return send(host, port, data.data(), data.size());
	}

	int
	UDPClient::receive (void * buffer, size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs) noexcept
	{
		if ( !isOpen() )
		{
			return -1;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
#else
		const auto sock = m_fd;
#endif

		/* Apply timeout using select(). */
		if ( timeoutMs > 0 )
		{
			fd_set readFds;
			FD_ZERO(&readFds);
			FD_SET(sock, &readFds);

			struct timeval tv{};
			tv.tv_sec = static_cast< long >(timeoutMs / 1000);
			tv.tv_usec = static_cast< long >((timeoutMs % 1000) * 1000);

			const auto selectResult = select(static_cast< int >(sock) + 1, &readFds, nullptr, nullptr, &tv);

			if ( selectResult <= 0 )
			{
				return 0; /* Timeout or error. */
			}
		}

		struct sockaddr_in sender{};
		auto senderLen = static_cast< socklen_t >(sizeof(sender));

		const auto bytesRead = recvfrom(
			sock,
			static_cast< char * >(buffer),
			static_cast< int >(maxLength),
			0,
			reinterpret_cast< struct sockaddr * >(&sender),
			&senderLen
		);

		if ( bytesRead > 0 )
		{
			std::array< char, INET_ADDRSTRLEN > addrStr{};
			inet_ntop(AF_INET, &sender.sin_addr, addrStr.data(), addrStr.size());

			senderAddress = addrStr.data();
			senderPort = ntohs(sender.sin_port);
		}

		return static_cast< int >(bytesRead);
	}

	std::string
	UDPClient::receiveString (size_t maxLength, std::string & senderAddress, uint16_t & senderPort, uint32_t timeoutMs) noexcept
	{
		std::string result;
		result.resize(maxLength);

		const auto bytesRead = receive(result.data(), maxLength, senderAddress, senderPort, timeoutMs);

		if ( bytesRead > 0 )
		{
			result.resize(static_cast< size_t >(bytesRead));
		}
		else
		{
			result.clear();
		}

		return result;
	}

	bool
	UDPClient::setBroadcast (bool enable) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		int flag = enable ? 1 : 0;

#ifdef _WIN32
		return setsockopt(static_cast< SocketType >(m_socket), SOL_SOCKET, SO_BROADCAST, reinterpret_cast< const char * >(&flag), sizeof(flag)) == 0;
#else
		return setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) == 0;
#endif
	}

	/* ---- SSDP Discovery (static, self-contained) ---- */

	std::vector< SSDPDevice >
	UDPClient::ssdpDiscover (const std::string & searchTarget, int timeoutSeconds) noexcept
	{
		std::vector< SSDPDevice > devices;

#ifdef _WIN32
		if ( !ensureWsa() )
		{
			return devices;
		}
#endif

		/* Create a temporary UDP socket for multicast. */
		const auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if ( sock == InvalidSocket )
		{
			return devices;
		}

		/* Allow address reuse. */
		int reuse = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&reuse), sizeof(reuse));

		/* Set multicast TTL. */
		int ttl = 2;
		setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast< const char * >(&ttl), sizeof(ttl));

		/* Bind to any address (to receive unicast responses). */
		struct sockaddr_in bindAddr{};
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(0);
		bindAddr.sin_addr.s_addr = INADDR_ANY;

		if ( ::bind(sock, reinterpret_cast< const struct sockaddr * >(&bindAddr), sizeof(bindAddr)) < 0 )
		{
			platformCloseSocket(sock);

			return devices;
		}

		/* Build and send M-SEARCH packet. */
		const auto packet = buildMSearchPacket(searchTarget, std::min(timeoutSeconds, MaxMX));

		struct sockaddr_in dest{};
		dest.sin_family = AF_INET;
		dest.sin_port = htons(SSDPMulticastPort);
		inet_pton(AF_INET, SSDPMulticastAddress, &dest.sin_addr);

		sendto(sock, packet.c_str(), static_cast< int >(packet.size()), 0, reinterpret_cast< const struct sockaddr * >(&dest), sizeof(dest));

		/* Collect responses until timeout. */
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);

		while ( std::chrono::steady_clock::now() < deadline )
		{
			fd_set readFds;
			FD_ZERO(&readFds);
			FD_SET(sock, &readFds);

			/* Poll in 500ms intervals to check deadline. */
			struct timeval tv{};
			tv.tv_sec = 0;
			tv.tv_usec = 500000;

			const auto result = select(static_cast< int >(sock) + 1, &readFds, nullptr, nullptr, &tv);

			if ( result > 0 && FD_ISSET(sock, &readFds) )
			{
				std::array< char, 4096 > buffer{};
				struct sockaddr_in sender{};
				auto senderLen = static_cast< socklen_t >(sizeof(sender));

				const auto bytesRead = recvfrom(sock, buffer.data(), static_cast< int >(buffer.size()) - 1, 0, reinterpret_cast< struct sockaddr * >(&sender), &senderLen);

				if ( bytesRead > 0 )
				{
					buffer[static_cast< size_t >(bytesRead)] = '\0';

					SSDPDevice device;

					std::array< char, INET_ADDRSTRLEN > addrStr{};
					inet_ntop(AF_INET, &sender.sin_addr, addrStr.data(), addrStr.size());

					device.address = addrStr.data();
					device.port = ntohs(sender.sin_port);
					device.headers = parseSSDPHeaders(std::string(buffer.data(), static_cast< size_t >(bytesRead)));

					devices.emplace_back(std::move(device));
				}
			}
		}

		platformCloseSocket(sock);

		return devices;
	}
}