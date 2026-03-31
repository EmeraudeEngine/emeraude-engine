/*
* src/Net/SSDPDiscovery.hpp
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
#include <string>
#include <vector>
#include <map>

namespace EmEn::Net::SSDPDiscovery
{
	static constexpr auto MulticastAddress{"239.255.255.250"};
	static constexpr uint16_t MulticastPort{1900};
	static constexpr int DefaultTimeoutSeconds{5};

	/**
	 * @brief Represents a single SSDP device response.
	 */
	struct Device
	{
		std::string address;
		uint16_t port{0};
		std::map< std::string, std::string > headers;
	};

	/**
	 * @brief Performs an SSDP M-SEARCH and collects responses.
	 * @note Uses raw UDP multicast sockets (BSD sockets on Linux/macOS, Winsock on Windows).
	 * @param searchTarget The ST header value (e.g., "ssdp:all", "urn:schemas-upnp-org:device:Printer:1").
	 * @param timeoutSeconds How long to listen for responses.
	 * @return std::vector< Device > The discovered devices.
	 */
	[[nodiscard]]
	std::vector< Device > discover (const std::string & searchTarget, int timeoutSeconds = DefaultTimeoutSeconds) noexcept;
}
