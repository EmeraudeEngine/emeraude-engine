/*
* src/Net/SSDPDiscovery.cpp
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

#include "SSDPClient.hpp"

/* STL inclusions. */
#include <array>
#include <chrono>
#include <algorithm>
#include <sstream>

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

namespace EmEn::Net::SSDPDiscovery
{
	static constexpr int MaxMX{3};

	void
	closeSocket (SocketType sock) noexcept
	{
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}

	SocketType
	createSocket () noexcept
	{
		const auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if ( sock == InvalidSocket )
		{
			return InvalidSocket;
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

		if ( bind(sock, reinterpret_cast< const struct sockaddr * >(&bindAddr), sizeof(bindAddr)) < 0 )
		{
			closeSocket(sock);

			return InvalidSocket;
		}

		return sock;
	}

	std::string
	buildMSearchPacket (const std::string & searchTarget, int mx) noexcept
	{
		std::ostringstream packet;
		packet
			<< "M-SEARCH * HTTP/1.1\r\n"
			<< "HOST: " << MulticastAddress << ":" << MulticastPort << "\r\n"
			<< "MAN: \"ssdp:discover\"\r\n"
			<< "MX: " << mx << "\r\n"
			<< "ST: " << searchTarget << "\r\n"
			<< "\r\n";

		return packet.str();
	}

	std::map< std::string, std::string >
	parseHeaders (const std::string & response) noexcept
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

	std::vector< Device >
	discover (const std::string & searchTarget, int timeoutSeconds) noexcept
	{
		std::vector< Device > devices;

#ifdef _WIN32
		WSADATA wsaData{};

		if ( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 )
		{
			return devices;
		}
#endif

		const auto sock = createSocket();

		if ( sock == InvalidSocket )
		{
#ifdef _WIN32
			WSACleanup();
#endif
			return devices;
		}

		/* Build and send M-SEARCH packet. */
		const auto packet = buildMSearchPacket(searchTarget, std::min(timeoutSeconds, MaxMX));

		struct sockaddr_in dest{};
		dest.sin_family = AF_INET;
		dest.sin_port = htons(MulticastPort);
		inet_pton(AF_INET, MulticastAddress, &dest.sin_addr);

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

					Device device;

					std::array< char, INET_ADDRSTRLEN > addrStr{};
					inet_ntop(AF_INET, &sender.sin_addr, addrStr.data(), addrStr.size());

					device.address = addrStr.data();
					device.port = ntohs(sender.sin_port);
					device.headers = parseHeaders(std::string(buffer.data(), static_cast< size_t >(bytesRead)));

					devices.emplace_back(std::move(device));
				}
			}
		}

		closeSocket(sock);

#ifdef _WIN32
		WSACleanup();
#endif

		return devices;
	}
}
