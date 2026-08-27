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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "UDPClient.hpp"

/* STL inclusions. */
#include <mutex>
#include <shared_mutex>

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <sstream>
#include <utility>

/* Third-party inclusions. */
#ifdef _WIN32
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <MSWSock.h>

	using SocketType = SOCKET;
	static constexpr SocketType InvalidSocket = INVALID_SOCKET;
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>

	/* BSD-derived stacks report the receiving interface as a link-level address. */
	#ifndef __linux__
		#include <net/if_dl.h>
	#endif

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

	/*
	 * WSARecvMsg() has no import library: it must be fetched from the provider through
	 * WSAIoctl(). The pointer is provider-wide, so resolving it once is enough.
	 */
	static LPFN_WSARECVMSG
	resolveWSARecvMsg (SocketType sock) noexcept
	{
		static LPFN_WSARECVMSG function = nullptr;

		if ( function != nullptr )
		{
			return function;
		}

		GUID guid = WSAID_WSARECVMSG;
		DWORD bytesReturned = 0;

		if ( WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &function, sizeof(function), &bytesReturned, nullptr, nullptr) != 0 )
		{
			function = nullptr;
		}

		return function;
	}
#endif

	/* ---- Socket helpers (kept private to this TU) ---- */

	/* Only WinSock types the option value as a char pointer, hence the single wrapper. */
	static bool
	setSocketOption (SocketType sock, int level, int optionName, const void * value, size_t size) noexcept
	{
		return setsockopt(sock, level, optionName, reinterpret_cast< const char * >(value), static_cast< socklen_t >(size)) == 0;
	}

	static std::string
	toDottedDecimal (const struct in_addr & address) noexcept
	{
		std::array< char, INET_ADDRSTRLEN > buffer{};

		if ( inet_ntop(AF_INET, &address, buffer.data(), buffer.size()) == nullptr )
		{
			return {};
		}

		return buffer.data();
	}

	static bool
	isMulticastAddress (const struct in_addr & address) noexcept
	{
		/* Class D, 224.0.0.0/4. */
		return ( ntohl(address.s_addr) & 0xF0000000U ) == 0xE0000000U;
	}

	/* Returns true when the socket has data to read.
	 * ⚠️ timeoutMs == 0 means NON-BLOCKING, as the header states: a zero timeval polls and
	 * returns immediately. It used to return true without looking, so the following recvfrom()
	 * on a blocking socket parked the calling thread forever with no way to wake it. */
	static bool
	waitReadable (SocketType sock, uint32_t timeoutMs) noexcept
	{
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(sock, &readFds);

		struct timeval tv{};
		tv.tv_sec = static_cast< long >(timeoutMs / 1000);
		tv.tv_usec = static_cast< long >((timeoutMs % 1000) * 1000);

		return select(static_cast< int >(sock) + 1, &readFds, nullptr, nullptr, &tv) > 0;
	}

	/* ---- SSDP helpers (kept private to this TU) ---- */

	static constexpr auto SSDPMulticastAddress{"239.255.255.250"};
	static constexpr uint16_t SSDPMulticastPort{1900};
	static constexpr int MaxMX{3};

	/* The search target is interpolated into a request line: anything that could start a new
	 * header (CR, LF, NUL, or any control character) makes the datagram attacker-shaped. */
	static bool
	isValidSearchTarget (const std::string & searchTarget) noexcept
	{
		if ( searchTarget.empty() || searchTarget.size() > 256 )
		{
			return false;
		}

		return std::ranges::none_of(searchTarget, [] (char character) {
			return static_cast< unsigned char >(character) < 0x20 || static_cast< unsigned char >(character) == 0x7F;
		});
	}

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
		: m_handleMutex(std::move(other.m_handleMutex)),
		m_multicastMemberships(std::move(other.m_multicastMemberships)),
#ifdef _WIN32
		m_socket(std::exchange(other.m_socket, ~uintptr_t{0})),
#else
		m_fd(std::exchange(other.m_fd, -1)),
#endif
		m_datagramInfoEnabled(std::exchange(other.m_datagramInfoEnabled, false))
	{
		/* A moved-from vector is valid but unspecified: the source must not keep claiming
		 * memberships that now belong to this socket. */
		other.m_multicastMemberships.clear();
	}

	UDPClient &
	UDPClient::operator= (UDPClient && other) noexcept
	{
		if ( this != &other )
		{
			close();

			m_handleMutex = std::move(other.m_handleMutex);
			m_multicastMemberships = std::move(other.m_multicastMemberships);
			other.m_multicastMemberships.clear();

			m_datagramInfoEnabled = std::exchange(other.m_datagramInfoEnabled, false);

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

		/* Allow address reuse. Both options are read by bind() and ignored afterwards:
		 * they MUST be set here, at creation time, never next to the bind() call. */
		int reuse = 1;

#ifdef _WIN32
		setsockopt(static_cast< SocketType >(m_socket), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char * >(&reuse), sizeof(reuse));
#else
		setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	#ifdef SO_REUSEPORT
		/* POSIX only, WinSock has no equivalent. Required to share a multicast service
		 * port with a system daemon already holding it (an mDNS responder on 5353, for
		 * instance): on macOS/BSD, SO_REUSEADDR alone does not grant that. Both processes
		 * must set the same options before bind() for the sharing to be allowed.
		 * WARNING: this also relaxes unicast. Two sockets bound to the same unicast port
		 * no longer collide with EADDRINUSE; the kernel spreads incoming datagrams
		 * between them, turning a loud conflict into silent packet loss. */
		setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	#endif
#endif

		/* Best-effort, and deliberately NOT deferred to the first receive(DatagramInfo):
		 * the kernel builds the ancillary data from what it recorded when the datagram was
		 * queued. Arming the option later still yields the destination address, which sits
		 * in the IP header, but the receiving interface index comes back as 0 — a plausible
		 * structure carrying a silently wrong index. Measured on Linux 6.x: index 2 when
		 * armed here, 0 when armed after the datagram was queued.
		 * The cost for consumers using the plain receive() is nil, recvfrom() discards the
		 * ancillary data. */
		enableDatagramInfo();

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
		/* The kernel releases the memberships with the socket, and the next open() starts
		 * from a clean slate: keeping them recorded would make join() a silent no-op on a
		 * group this socket is no longer part of. */
		m_multicastMemberships.clear();

		m_datagramInfoEnabled = false;

		/* Phase 1 — shared lock: shutdown() wakes a thread parked in recvfrom()/select() on this
		 * descriptor, while the handle is still valid. */
		{
			const std::shared_lock< std::shared_mutex > wakeLock{*m_handleMutex};

#ifdef _WIN32
			if ( m_socket != ~uintptr_t{0} )
			{
				::shutdown(static_cast< SocketType >(m_socket), SD_BOTH);
			}
#else
			if ( m_fd != -1 )
			{
				::shutdown(m_fd, SHUT_RDWR);
			}
#endif
		}

		/* Phase 2 — exclusive lock: waits for every in-flight call to release its shared lock,
		 * then invalidates the handle. */
		const std::unique_lock< std::shared_mutex > handleLock{*m_handleMutex};

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

	bool
	UDPClient::getLocalAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		struct sockaddr_in localAddr{};
		auto addrLen = static_cast< socklen_t >(sizeof(localAddr));

#ifdef _WIN32
		if ( getsockname(static_cast< SocketType >(m_socket), reinterpret_cast< struct sockaddr * >(&localAddr), &addrLen) != 0 )
#else
		if ( getsockname(m_fd, reinterpret_cast< struct sockaddr * >(&localAddr), &addrLen) != 0 )
#endif
		{
			return false;
		}

		std::array< char, INET_ADDRSTRLEN > addrStr{};
		inet_ntop(AF_INET, &localAddr.sin_addr, addrStr.data(), addrStr.size());

		address = addrStr.data();
		port = ntohs(localAddr.sin_port);

		return true;
	}

	/* ---- Send / Receive ---- */

	int
	UDPClient::send (const std::string & host, uint16_t port, const void * data, size_t length) noexcept
	{
		const std::shared_lock< std::shared_mutex > handleLock{*m_handleMutex};

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
		/* Shared lock for the WHOLE call: close() cannot invalidate the descriptor between
		 * the readiness test and the recv. */
		const std::shared_lock< std::shared_mutex > handleLock{*m_handleMutex};

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
		if ( !waitReadable(sock, timeoutMs) )
		{
			return 0; /* Timeout or error. */
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

	bool
	UDPClient::enableDatagramInfo () noexcept
	{
		if ( m_datagramInfoEnabled )
		{
			return true;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);

		const DWORD enable = 1;

		if ( !setSocketOption(sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable)) )
		{
			return false;
		}
#elif defined(__linux__)
		const int enable = 1;

		if ( !setSocketOption(m_fd, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable)) )
		{
			return false;
		}
#else
		const int enable = 1;

		/* BSD-derived stacks split the information across two options. */
		if ( !setSocketOption(m_fd, IPPROTO_IP, IP_RECVDSTADDR, &enable, sizeof(enable)) )
		{
			return false;
		}

		/* The interface index is a bonus: a stack refusing IP_RECVIF still yields the
		 * destination address, which is what separates multicast from unicast. */
		setSocketOption(m_fd, IPPROTO_IP, IP_RECVIF, &enable, sizeof(enable));
#endif

		m_datagramInfoEnabled = true;

		return true;
	}

	int
	UDPClient::receive (void * buffer, size_t maxLength, DatagramInfo & info, uint32_t timeoutMs) noexcept
	{
		const std::shared_lock< std::shared_mutex > handleLock{*m_handleMutex};

		if ( !isOpen() || !enableDatagramInfo() )
		{
			return -1;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
#else
		const auto sock = m_fd;
#endif

		if ( !waitReadable(sock, timeoutMs) )
		{
			return 0; /* Timeout or error. */
		}

		info = {};

		struct sockaddr_in sender{};
		struct in_addr destination{};
		bool destinationKnown = false;

		/* Ancillary data is small (a pktinfo, at most a link-level address); 512 bytes
		 * leave room for whatever else the stack decides to attach. */
		std::array< char, 512 > control{};

#ifdef _WIN32
		const auto recvMsg = resolveWSARecvMsg(sock);

		if ( recvMsg == nullptr )
		{
			return -1;
		}

		WSABUF data{};
		data.buf = static_cast< char * >(buffer);
		data.len = static_cast< ULONG >(maxLength);

		WSAMSG message{};
		message.name = reinterpret_cast< LPSOCKADDR >(&sender);
		message.namelen = sizeof(sender);
		message.lpBuffers = &data;
		message.dwBufferCount = 1;
		message.Control.buf = control.data();
		message.Control.len = static_cast< ULONG >(control.size());

		DWORD received = 0;

		if ( recvMsg(sock, &message, &received, nullptr, nullptr) != 0 )
		{
			return -1;
		}

		for ( auto * header = WSA_CMSG_FIRSTHDR(&message); header != nullptr; header = WSA_CMSG_NXTHDR(&message, header) )
		{
			if ( header->cmsg_level != IPPROTO_IP || header->cmsg_type != IP_PKTINFO )
			{
				continue;
			}

			IN_PKTINFO packetInfo{};
			std::memcpy(&packetInfo, WSA_CMSG_DATA(header), sizeof(packetInfo));

			destination = packetInfo.ipi_addr;
			destinationKnown = true;

			info.interfaceIndex = static_cast< uint32_t >(packetInfo.ipi_ifindex);
		}

		const auto bytesRead = static_cast< int >(received);
#else
		struct iovec data{};
		data.iov_base = buffer;
		data.iov_len = maxLength;

		struct msghdr message{};
		message.msg_name = &sender;
		message.msg_namelen = sizeof(sender);
		message.msg_iov = &data;
		message.msg_iovlen = 1;
		message.msg_control = control.data();
		message.msg_controllen = control.size();

		const auto received = recvmsg(sock, &message, 0);

		if ( received < 0 )
		{
			return -1;
		}

		for ( auto * header = CMSG_FIRSTHDR(&message); header != nullptr; header = CMSG_NXTHDR(&message, header) )
		{
			if ( header->cmsg_level != IPPROTO_IP )
			{
				continue;
			}

	#ifdef __linux__
			if ( header->cmsg_type == IP_PKTINFO )
			{
				struct in_pktinfo packetInfo{};
				std::memcpy(&packetInfo, CMSG_DATA(header), sizeof(packetInfo));

				destination = packetInfo.ipi_addr;
				destinationKnown = true;

				info.interfaceIndex = static_cast< uint32_t >(packetInfo.ipi_ifindex);
			}
	#else
			if ( header->cmsg_type == IP_RECVDSTADDR )
			{
				std::memcpy(&destination, CMSG_DATA(header), sizeof(destination));

				destinationKnown = true;
			}
			else if ( header->cmsg_type == IP_RECVIF )
			{
				struct sockaddr_dl link{};
				std::memcpy(&link, CMSG_DATA(header), sizeof(link));

				info.interfaceIndex = static_cast< uint32_t >(link.sdl_index);
			}
	#endif
		}

		const auto bytesRead = static_cast< int >(received);
#endif

		if ( bytesRead > 0 )
		{
			info.senderAddress = toDottedDecimal(sender.sin_addr);
			info.senderPort = ntohs(sender.sin_port);
		}

		if ( destinationKnown )
		{
			info.destinationAddress = toDottedDecimal(destination);
			info.multicast = isMulticastAddress(destination);
		}

		return bytesRead;
	}

	/* ---- IPv4 multicast ---- */

	bool
	UDPClient::joinMulticastGroup (const std::string & groupAddress, const std::string & interfaceAddress) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		struct ip_mreq request{};

		if ( inet_pton(AF_INET, groupAddress.c_str(), &request.imr_multiaddr) != 1 )
		{
			return false;
		}

		if ( interfaceAddress.empty() )
		{
			request.imr_interface.s_addr = INADDR_ANY;
		}
		else if ( inet_pton(AF_INET, interfaceAddress.c_str(), &request.imr_interface) != 1 )
		{
			return false;
		}

		const auto membership = std::make_pair(
			static_cast< uint32_t >(request.imr_multiaddr.s_addr),
			static_cast< uint32_t >(request.imr_interface.s_addr)
		);

		/* Already a member: the kernel would answer EADDRINUSE, which the consumer's
		 * periodic interface re-walk must not read as a failure. */
		if ( std::ranges::find(m_multicastMemberships, membership) != m_multicastMemberships.end() )
		{
			return true;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
#else
		const auto sock = m_fd;
#endif

		if ( !setSocketOption(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &request, sizeof(request)) )
		{
			return false;
		}

		m_multicastMemberships.emplace_back(membership);

		return true;
	}

	bool
	UDPClient::leaveMulticastGroup (const std::string & groupAddress, const std::string & interfaceAddress) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		struct ip_mreq request{};

		if ( inet_pton(AF_INET, groupAddress.c_str(), &request.imr_multiaddr) != 1 )
		{
			return false;
		}

		if ( interfaceAddress.empty() )
		{
			request.imr_interface.s_addr = INADDR_ANY;
		}
		else if ( inet_pton(AF_INET, interfaceAddress.c_str(), &request.imr_interface) != 1 )
		{
			return false;
		}

		const auto membership = std::make_pair(
			static_cast< uint32_t >(request.imr_multiaddr.s_addr),
			static_cast< uint32_t >(request.imr_interface.s_addr)
		);

		const auto membershipIt = std::ranges::find(m_multicastMemberships, membership);

		/* Never joined. Teardown paths call this blindly, it is not an error. */
		if ( membershipIt == m_multicastMemberships.end() )
		{
			return true;
		}

		/* Forget it whatever the kernel answers: a failing IP_DROP_MEMBERSHIP means the
		 * kernel does not hold us as a member either. */
		m_multicastMemberships.erase(membershipIt);

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
#else
		const auto sock = m_fd;
#endif

		return setSocketOption(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &request, sizeof(request));
	}

	bool
	UDPClient::setMulticastTTL (uint8_t ttl) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
		const DWORD value = ttl;
#else
		const auto sock = m_fd;
		/* BSD-derived stacks expect a single byte here, not an int. */
		const unsigned char value = ttl;
#endif

		return setSocketOption(sock, IPPROTO_IP, IP_MULTICAST_TTL, &value, sizeof(value));
	}

	bool
	UDPClient::setMulticastLoopback (bool enable) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
		const DWORD value = enable ? 1 : 0;
#else
		const auto sock = m_fd;
		/* Same one-byte expectation as IP_MULTICAST_TTL. */
		const unsigned char value = enable ? 1 : 0;
#endif

		return setSocketOption(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &value, sizeof(value));
	}

	bool
	UDPClient::setMulticastInterface (const std::string & interfaceAddress) noexcept
	{
		if ( !isOpen() )
		{
			return false;
		}

		struct in_addr address{};

		if ( interfaceAddress.empty() )
		{
			address.s_addr = INADDR_ANY;
		}
		else if ( inet_pton(AF_INET, interfaceAddress.c_str(), &address) != 1 )
		{
			return false;
		}

#ifdef _WIN32
		const auto sock = static_cast< SocketType >(m_socket);
#else
		const auto sock = m_fd;
#endif

		return setSocketOption(sock, IPPROTO_IP, IP_MULTICAST_IF, &address, sizeof(address));
	}

	/* ---- SSDP Discovery (static, self-contained) ---- */

	std::vector< SSDPDevice >
	UDPClient::ssdpDiscover (const std::string & searchTarget, int timeoutSeconds) noexcept
	{
		/* ⚠️ The search target is interpolated into the M-SEARCH request line: a CR or LF in it
		 * would let the caller append arbitrary headers to a datagram sent from this host. */
		if ( !isValidSearchTarget(searchTarget) )
		{
			return {};
		}

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
		/* ⚠️ BSD-derived stacks (macOS included) expect a single byte here, not an int —
		 * setMulticastTTL() below says so and uses the right type. Passing an int makes the
		 * option fail with EINVAL, the TTL stays 1, and discovery silently misses every device
		 * one hop away. */
#ifdef _WIN32
		const DWORD ttl = 2;
#else
		const unsigned char ttl = 2;
#endif
		/* NOTE: this unit reports through return values only — it deliberately depends on nothing
		 * but emeraude_export.hpp, so there is no tracer here. A refused TTL only costs the
		 * one-hop-away devices. */
		static_cast< void >(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast< const char * >(&ttl), sizeof(ttl)));

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
