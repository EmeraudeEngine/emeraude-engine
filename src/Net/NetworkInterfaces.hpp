/*
 * src/Net/NetworkInterfaces.hpp
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
#include <string>
#include <vector>

namespace EmEn::Net::NetworkInterfaces
{
	/**
	 * @brief The IP family of one enumerated address.
	 */
	enum class AddressFamily : uint8_t
	{
		IPv4,
		IPv6
	};

	/**
	 * @brief Sentinel for Interface::prefixLength when the OS reports no usable netmask
	 * (or a non-contiguous one, which has no CIDR representation).
	 */
	constexpr uint8_t PrefixLengthUnknown{0xFF};

	/**
	 * @brief Describes one IP address carried by a local network interface.
	 * @note One entry is produced per address, IPv4 and IPv6 alike: an interface holding
	 * several addresses yields several entries sharing the same name, index and MAC.
	 * @note Mirrors the flat shape of Node.js os.networkInterfaces(), so a scripting bridge
	 * only has to rename fields, never to re-enumerate.
	 */
	struct EMEN_LEAN_API Interface
	{
		std::string name;			/* OS interface name ("eth0", "en0", "Ethernet 2"). Informative only. */
		std::string address;		/* Textual address: dotted-decimal (IPv4) or RFC 5952 (IPv6). THIS is what the multicast API expects for IPv4. */
		std::string netmask;		/* Textual netmask, same notation as the address. Empty when the OS does not report one. */
		std::string mac;			/* Hardware address, lowercase "aa:bb:cc:dd:ee:ff". EMPTY when the OS reports none (loopback, tunnels, some VPNs) — never a zero placeholder. */
		uint32_t index{0};			/* OS interface index, 0 when unknown. */
		uint32_t scopeId{0};		/* IPv6 scope identifier (sin6_scope_id). Always 0 for IPv4. */
		uint8_t prefixLength{PrefixLengthUnknown};	/* CIDR prefix length derived from the netmask (0-32 IPv4, 0-128 IPv6), PrefixLengthUnknown otherwise. */
		AddressFamily family{AddressFamily::IPv4};	/* The family of 'address'. */
		bool loopback{false};		/* The interface is a loopback device. */
		bool up{false};				/* The interface is administratively and operationally up. */
		bool multicastCapable{false};	/* The interface can carry IP multicast. */
	};

	/**
	 * @brief Enumerates the local IP addresses, IPv4 and IPv6.
	 * @note Platform-specific: uses getifaddrs() on Linux/macOS (AF_PACKET / AF_LINK entries
	 * supply the MAC), GetAdaptersAddresses(AF_UNSPEC) on Windows.
	 * @warning The result is a snapshot. Interfaces appear and disappear at runtime
	 * (VPN, hotplug, container bridges): a consumer tracking them must poll and diff.
	 * @return std::vector< Interface > The local addresses, empty on failure.
	 */
	[[nodiscard]]
	EMEN_LEAN_API std::vector< Interface > enumerate () noexcept;

	/**
	 * @brief Enumerates the local IPv4 addresses usable to join a multicast group.
	 * @note Keeps the IPv4 entries that are up, multicast-capable and carry a valid address.
	 * IPv4 only by construction: UDPClient's multicast API is IPv4 only. Loopback is
	 * deliberately kept: it is the only way to exercise multicast on a single machine.
	 * @see enumerate() for the platform notes and the snapshot warning.
	 * @return std::vector< Interface > The multicast-capable interfaces, empty on failure.
	 */
	[[nodiscard]]
	EMEN_LEAN_API std::vector< Interface > enumerateMulticastCapable () noexcept;

	/**
	 * @brief Returns the textual name of an address family, "IPv4" or "IPv6".
	 * @param family The family.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_LEAN_API const char * to_cstring (AddressFamily family) noexcept;
}
