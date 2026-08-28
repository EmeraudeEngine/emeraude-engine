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
#include <cstdio>
#include <map>
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

	/* The hardware address travels in a family of its own: AF_PACKET on Linux, AF_LINK on
	 * the BSD family (macOS included). */
	#if defined(__linux__)
		#include <netpacket/packet.h>
	#else
		#include <net/if_dl.h>
	#endif
#endif

namespace EmEn::Net::NetworkInterfaces
{
	namespace
	{
		constexpr size_t MACLength{6};

		/**
		 * @brief Formats a 48-bit hardware address as lowercase "aa:bb:cc:dd:ee:ff".
		 * @note Anything but 6 bytes (InfiniBand's 20, tunnels' 0) yields an empty string,
		 * and so does an all-zero address: Linux reports one for the loopback (AF_PACKET,
		 * sll_halen 6, every byte 0). The struct contract is "empty when unknown", never a
		 * zero placeholder.
		 */
		std::string
		formatMAC (const uint8_t * bytes, size_t length) noexcept
		{
			if ( bytes == nullptr || length != MACLength )
			{
				return {};
			}

			bool allZero = true;

			for ( size_t index = 0; index < MACLength; ++index )
			{
				if ( bytes[index] != 0 )
				{
					allZero = false;

					break;
				}
			}

			if ( allZero )
			{
				return {};
			}

			std::array< char, 18 > buffer{};

			std::snprintf(buffer.data(), buffer.size(), "%02x:%02x:%02x:%02x:%02x:%02x", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);

			return buffer.data();
		}

		/**
		 * @brief Prints an IPv4 or IPv6 address in its textual form.
		 * @param family AF_INET or AF_INET6.
		 * @param rawAddress Pointer to the in_addr / in6_addr.
		 */
		std::string
		toText (int family, const void * rawAddress) noexcept
		{
			if ( rawAddress == nullptr )
			{
				return {};
			}

			std::array< char, INET6_ADDRSTRLEN > buffer{};

			if ( inet_ntop(family, rawAddress, buffer.data(), static_cast< socklen_t >(buffer.size())) == nullptr )
			{
				return {};
			}

			return buffer.data();
		}

		/**
		 * @brief Derives the CIDR prefix length from a netmask given as raw bytes.
		 * @note A non-contiguous mask (ones after a zero) has no CIDR form and yields
		 * PrefixLengthUnknown rather than a misleading count.
		 */
		uint8_t
		prefixLengthFromMask (const uint8_t * bytes, size_t length) noexcept
		{
			uint8_t prefixLength = 0;
			bool zeroSeen = false;

			for ( size_t index = 0; index < length; ++index )
			{
				for ( int bit = 7; bit >= 0; --bit )
				{
					const bool one = ( bytes[index] >> bit & 1U ) != 0;

					if ( one )
					{
						if ( zeroSeen )
						{
							return PrefixLengthUnknown;
						}

						++prefixLength;
					}
					else
					{
						zeroSeen = true;
					}
				}
			}

			return prefixLength;
		}

		/**
		 * @brief Reads the netmask of a socket address, as text plus CIDR prefix length.
		 * @return std::pair< std::string, uint8_t > Empty text and PrefixLengthUnknown when absent.
		 */
		std::pair< std::string, uint8_t >
		netmaskOf (const struct sockaddr * mask) noexcept
		{
			if ( mask == nullptr )
			{
				return {{}, PrefixLengthUnknown};
			}

			if ( mask->sa_family == AF_INET )
			{
				const auto & inet = reinterpret_cast< const struct sockaddr_in * >(mask)->sin_addr;

				return {toText(AF_INET, &inet), prefixLengthFromMask(reinterpret_cast< const uint8_t * >(&inet), sizeof(inet))};
			}

			if ( mask->sa_family == AF_INET6 )
			{
				const auto & inet6 = reinterpret_cast< const struct sockaddr_in6 * >(mask)->sin6_addr;

				return {toText(AF_INET6, &inet6), prefixLengthFromMask(reinterpret_cast< const uint8_t * >(&inet6), sizeof(inet6))};
			}

			return {{}, PrefixLengthUnknown};
		}

		/**
		 * @brief Fills the address-dependent part of an entry from a socket address.
		 * @return bool False when the family is neither AF_INET nor AF_INET6.
		 */
		bool
		fillAddress (Interface & item, const struct sockaddr * address) noexcept
		{
			if ( address == nullptr )
			{
				return false;
			}

			if ( address->sa_family == AF_INET )
			{
				const auto * inet = reinterpret_cast< const struct sockaddr_in * >(address);

				item.family = AddressFamily::IPv4;
				item.address = toText(AF_INET, &inet->sin_addr);
				item.scopeId = 0;

				return !item.address.empty();
			}

			if ( address->sa_family == AF_INET6 )
			{
				const auto * inet6 = reinterpret_cast< const struct sockaddr_in6 * >(address);

				item.family = AddressFamily::IPv6;
				item.address = toText(AF_INET6, &inet6->sin6_addr);
				item.scopeId = inet6->sin6_scope_id;

				return !item.address.empty();
			}

			return false;
		}

#ifdef _WIN32
		std::string
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

		/**
		 * @brief Builds the textual netmask from a prefix length.
		 * @note Windows exposes only the prefix (OnLinkPrefixLength); the mask is derived
		 * so both fields are always filled, like on POSIX.
		 */
		std::string
		netmaskFromPrefix (AddressFamily family, uint8_t prefixLength) noexcept
		{
			const size_t byteCount = family == AddressFamily::IPv4 ? 4 : 16;

			if ( prefixLength > byteCount * 8 )
			{
				return {};
			}

			std::array< uint8_t, 16 > mask{};
			auto remaining = static_cast< int >(prefixLength);

			for ( size_t index = 0; index < byteCount; ++index )
			{
				if ( remaining >= 8 )
				{
					mask[index] = 0xFF;
				}
				else if ( remaining > 0 )
				{
					mask[index] = static_cast< uint8_t >(0xFFU << (8 - remaining));
				}

				remaining -= 8;
			}

			return toText(family == AddressFamily::IPv4 ? AF_INET : AF_INET6, mask.data());
		}
#endif
	}

	const char *
	to_cstring (AddressFamily family) noexcept
	{
		switch ( family )
		{
			case AddressFamily::IPv4 :
				return "IPv4";

			case AddressFamily::IPv6 :
				return "IPv6";
		}

		return "IPv4";
	}

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

			result = GetAdaptersAddresses(AF_UNSPEC, Flags, nullptr, reinterpret_cast< IP_ADAPTER_ADDRESSES * >(buffer.data()), &size);
		}

		if ( result != NO_ERROR )
		{
			return interfaces;
		}

		for ( const auto * adapter = reinterpret_cast< const IP_ADAPTER_ADDRESSES * >(buffer.data()); adapter != nullptr; adapter = adapter->Next )
		{
			const auto name = toUTF8(adapter->FriendlyName);
			const auto mac = formatMAC(adapter->PhysicalAddress, adapter->PhysicalAddressLength);

			for ( const auto * unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next )
			{
				Interface item;

				if ( !fillAddress(item, unicast->Address.lpSockaddr) )
				{
					continue;
				}

				item.name = name;
				item.mac = mac;
				item.prefixLength = unicast->OnLinkPrefixLength;
				item.netmask = netmaskFromPrefix(item.family, item.prefixLength);
				/* An adapter carries one index per family; they are equal on every stack
				 * seen so far but the API keeps them distinct, so read the right one. */
				item.index = item.family == AddressFamily::IPv6 ? adapter->Ipv6IfIndex : adapter->IfIndex;
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

		/* First pass: the hardware address lives in its own entry per interface
		 * (AF_PACKET / AF_LINK), keyed by name for the address entries to pick up. */
		std::map< std::string, std::string > macByName;

		for ( const auto * entry = list; entry != nullptr; entry = entry->ifa_next )
		{
			if ( entry->ifa_addr == nullptr || entry->ifa_name == nullptr )
			{
				continue;
			}

	#if defined(__linux__)
			if ( entry->ifa_addr->sa_family == AF_PACKET )
			{
				const auto * link = reinterpret_cast< const struct sockaddr_ll * >(entry->ifa_addr);

				macByName[entry->ifa_name] = formatMAC(link->sll_addr, link->sll_halen);
			}
	#else
			if ( entry->ifa_addr->sa_family == AF_LINK )
			{
				const auto * link = reinterpret_cast< const struct sockaddr_dl * >(entry->ifa_addr);

				macByName[entry->ifa_name] = formatMAC(reinterpret_cast< const uint8_t * >(LLADDR(link)), link->sdl_alen);
			}
	#endif
		}

		/* Second pass: one entry per IPv4/IPv6 address. */
		for ( const auto * entry = list; entry != nullptr; entry = entry->ifa_next )
		{
			Interface item;

			if ( !fillAddress(item, entry->ifa_addr) )
			{
				continue;
			}

			if ( entry->ifa_name != nullptr )
			{
				item.name = entry->ifa_name;
				item.index = if_nametoindex(entry->ifa_name);

				if ( const auto macIt = macByName.find(item.name); macIt != macByName.end() )
				{
					item.mac = macIt->second;
				}
			}

			auto [netmask, prefixLength] = netmaskOf(entry->ifa_netmask);
			item.netmask = std::move(netmask);
			item.prefixLength = prefixLength;
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

		/* ⚠️ Loopback must survive this filter — it is the only way to exercise multicast on a
		 * single machine — but IFF_MULTICAST does not say so on every platform:
		 *  - macOS/BSD sets it on 'lo0', so the flag alone keeps it. Nothing to do.
		 *  - Linux NEVER sets it on 'lo' (<LOOPBACK,UP,LOWER_UP>) although the kernel carries
		 *    multicast there perfectly well — measured 2026-08-28: IP_ADD_MEMBERSHIP and
		 *    IP_MULTICAST_IF on 127.0.0.1 are both accepted, and the datagram makes the round
		 *    trip. Requiring the flag dropped a usable interface, and on a machine with no link
		 *    it left this list EMPTY: every "join on each interface" loop then did nothing at
		 *    all, and reported no error doing it. Hence the exemption below — Linux only.
		 *  - Windows is deliberately NOT exempted: neither the flag reported on the loopback
		 *    pseudo-interface nor the outcome of a join on 127.0.0.1 has been measured there,
		 *    and an interface that cannot be joined is exactly the trap that run documented
		 *    (a dead APIPA entry aborting a naive join loop). Measure before widening this.
		 * Do NOT infer multicast support on Linux from IFF_MULTICAST alone. */
		std::erase_if(interfaces, [] (const Interface & item) noexcept {
			if ( item.family != AddressFamily::IPv4 || !item.up || item.address.empty() )
			{
				return true;
			}

#if defined(__linux__)
			if ( item.loopback )
			{
				return false;
			}
#endif

			return !item.multicastCapable;
		});

		return interfaces;
	}
}
