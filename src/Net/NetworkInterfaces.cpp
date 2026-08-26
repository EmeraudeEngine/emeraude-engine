/*
 * src/Net/NetworkInterfaces.cpp
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

#include "NetworkInterfaces.hpp"

/* STL inclusions. */
#include <array>
#include <utility>

/* Third-party inclusions. */
/* NOTE: unlike SerialPort and WiFiScanner, this unit is NOT split per OS: getifaddrs()
 * covers Linux and macOS identically, so a single translation unit with one Windows
 * branch is enough. Same layout as UDPClient.cpp. */
#ifdef _WIN32
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <iphlpapi.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
#endif

namespace EmEn::Net::NetworkInterfaces
{
#ifdef _WIN32
	static std::string
	toUTF8 (const wchar_t * value) noexcept
	{
		if ( value == nullptr )
		{
			return {};
		}

		const auto length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);

		/* A length of 1 is the terminating null alone, i.e. an empty name. */
		if ( length <= 1 )
		{
			return {};
		}

		std::string output(static_cast< size_t >(length) - 1, '\0');

		WideCharToMultiByte(CP_UTF8, 0, value, -1, output.data(), length, nullptr, nullptr);

		return output;
	}

	static std::string
	prefixToNetmask (uint8_t prefixLength) noexcept
	{
		if ( prefixLength > 32 )
		{
			return {};
		}

		/* Shifting a 32-bit value by 32 is undefined, hence the explicit zero case. */
		const uint32_t mask = prefixLength == 0 ? 0 : 0xFFFFFFFFU << (32U - prefixLength);

		struct in_addr address{};
		address.s_addr = htonl(mask);

		std::array< char, INET_ADDRSTRLEN > buffer{};

		if ( inet_ntop(AF_INET, &address, buffer.data(), buffer.size()) == nullptr )
		{
			return {};
		}

		return buffer.data();
	}
#else
	static std::string
	toDottedDecimal (const struct sockaddr * address) noexcept
	{
		if ( address == nullptr || address->sa_family != AF_INET )
		{
			return {};
		}

		const auto * inetAddress = reinterpret_cast< const struct sockaddr_in * >(address);

		std::array< char, INET_ADDRSTRLEN > buffer{};

		if ( inet_ntop(AF_INET, &inetAddress->sin_addr, buffer.data(), buffer.size()) == nullptr )
		{
			return {};
		}

		return buffer.data();
	}
#endif

	std::vector< Interface >
	enumerate () noexcept
	{
		std::vector< Interface > interfaces;

#ifdef _WIN32
		/* GetAdaptersAddresses() writes into a caller-provided buffer. 16 KiB covers the
		 * vast majority of hosts; the table can also grow between two calls, hence the
		 * bounded retry loop rather than a single size query. */
		ULONG size = 16 * 1024;
		ULONG result = ERROR_BUFFER_OVERFLOW;
		std::vector< uint8_t > buffer;

		constexpr ULONG Flags{GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER};

		for ( int attempt = 0; attempt < 3 && result == ERROR_BUFFER_OVERFLOW; ++attempt )
		{
			buffer.resize(size);

			result = GetAdaptersAddresses(AF_INET, Flags, nullptr, reinterpret_cast< IP_ADAPTER_ADDRESSES * >(buffer.data()), &size);
		}

		if ( result != NO_ERROR )
		{
			return interfaces;
		}

		for ( const auto * adapter = reinterpret_cast< const IP_ADAPTER_ADDRESSES * >(buffer.data()); adapter != nullptr; adapter = adapter->Next )
		{
			for ( const auto * unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next )
			{
				if ( unicast->Address.lpSockaddr == nullptr || unicast->Address.lpSockaddr->sa_family != AF_INET )
				{
					continue;
				}

				const auto * inetAddress = reinterpret_cast< const struct sockaddr_in * >(unicast->Address.lpSockaddr);

				std::array< char, INET_ADDRSTRLEN > addressBuffer{};

				if ( inet_ntop(AF_INET, &inetAddress->sin_addr, addressBuffer.data(), addressBuffer.size()) == nullptr )
				{
					continue;
				}

				Interface item;
				item.name = toUTF8(adapter->FriendlyName);
				item.address = addressBuffer.data();
				item.netmask = prefixToNetmask(unicast->OnLinkPrefixLength);
				item.index = adapter->IfIndex;
				item.loopback = adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
				item.up = adapter->OperStatus == IfOperStatusUp;
				item.multicastCapable = ( adapter->Flags & IP_ADAPTER_NO_MULTICAST ) == 0;

				interfaces.emplace_back(std::move(item));
			}
		}
#else
		struct ifaddrs * list = nullptr;

		if ( getifaddrs(&list) != 0 )
		{
			return interfaces;
		}

		for ( const auto * entry = list; entry != nullptr; entry = entry->ifa_next )
		{
			if ( entry->ifa_addr == nullptr || entry->ifa_addr->sa_family != AF_INET )
			{
				continue;
			}

			auto address = toDottedDecimal(entry->ifa_addr);

			if ( address.empty() )
			{
				continue;
			}

			Interface item;

			if ( entry->ifa_name != nullptr )
			{
				item.name = entry->ifa_name;
				item.index = if_nametoindex(entry->ifa_name);
			}

			item.address = std::move(address);
			item.netmask = toDottedDecimal(entry->ifa_netmask);
			item.loopback = ( entry->ifa_flags & IFF_LOOPBACK ) != 0;
			/* IFF_UP is the administrative state, IFF_RUNNING the operational one. A NIC
			 * with no cable is up but not running, and joining a group on it buys nothing. */
			item.up = ( entry->ifa_flags & IFF_UP ) != 0 && ( entry->ifa_flags & IFF_RUNNING ) != 0;
			item.multicastCapable = ( entry->ifa_flags & IFF_MULTICAST ) != 0;

			interfaces.emplace_back(std::move(item));
		}

		freeifaddrs(list);
#endif

		return interfaces;
	}

	std::vector< Interface >
	enumerateMulticastCapable () noexcept
	{
		auto interfaces = enumerate();

		std::erase_if(interfaces, [] (const Interface & item) noexcept {
			return !item.up || !item.multicastCapable || item.address.empty();
		});

		return interfaces;
	}
}
