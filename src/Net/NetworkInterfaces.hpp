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
	 * @brief Describes one IPv4 address carried by a local network interface.
	 * @note One entry is produced per IPv4 address: an interface holding several
	 * addresses yields several entries sharing the same name and index.
	 */
	struct EMEN_LEAN_API Interface
	{
		std::string name;			/* OS interface name ("eth0", "en0", "Ethernet 2"). Informative only. */
		std::string address;		/* IPv4 address, dotted-decimal. THIS is what the multicast API expects. */
		std::string netmask;		/* IPv4 netmask, dotted-decimal. Empty when the OS does not report one. */
		uint32_t index{0};			/* OS interface index, 0 when unknown. */
		bool loopback{false};		/* The interface is a loopback device. */
		bool up{false};				/* The interface is administratively and operationally up. */
		bool multicastCapable{false};	/* The interface can carry IP multicast. */
	};

	/**
	 * @brief Enumerates the local IPv4 interfaces.
	 * @note Platform-specific: uses getifaddrs() on Linux/macOS, GetAdaptersAddresses() on Windows.
	 * @note IPv4 only, mirroring the rest of the UDP/multicast API.
	 * @warning The result is a snapshot. Interfaces appear and disappear at runtime
	 * (VPN, hotplug, container bridges): a consumer tracking them must poll and diff.
	 * @return std::vector< Interface > The local interfaces, empty on failure.
	 */
	[[nodiscard]]
	EMEN_LEAN_API std::vector< Interface > enumerate () noexcept;

	/**
	 * @brief Enumerates the local IPv4 interfaces usable to join a multicast group.
	 * @note Keeps the entries that are up, multicast-capable and carry a valid address.
	 * Loopback is deliberately kept: it is the only way to exercise multicast on a
	 * single machine.
	 * @see enumerate() for the platform notes and the snapshot warning.
	 * @return std::vector< Interface > The multicast-capable interfaces, empty on failure.
	 */
	[[nodiscard]]
	EMEN_LEAN_API std::vector< Interface > enumerateMulticastCapable () noexcept;
}
