/*
 * src/Net/WiFiScanner.hpp
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

namespace EmEn::Net::WiFiScanner
{
	/**
	 * @brief Represents a WiFi network (either from a scan or as a current connection).
	 */
	struct Network
	{
		std::string ssid;
		std::string bssid;
		int32_t signalLevel{0};     /* Signal strength in dBm (e.g., -50). */
		int32_t quality{0};         /* Signal quality as percentage (0-100). */
		uint32_t frequency{0};      /* Frequency in MHz (e.g., 2437, 5180). */
		int32_t channel{0};         /* Channel number. */
		std::string security;       /* Security type (e.g., "WPA2", "WPA3", "Open"). */
		std::string mode;           /* Network mode (e.g., "Infra", "Ad-Hoc"). */
	};

	/**
	 * @brief Scans for available WiFi networks.
	 * @note Platform-specific: uses nmcli (Linux), netsh (Windows), CoreWLAN (macOS).
	 * @return std::vector< Network > List of discovered networks.
	 */
	[[nodiscard]]
	std::vector< Network > scan () noexcept;

	/**
	 * @brief Returns the currently connected WiFi network(s).
	 * @note Platform-specific: uses nmcli (Linux), netsh (Windows), CoreWLAN (macOS).
	 * @return std::vector< Network > Current connection(s), empty if not connected via WiFi.
	 */
	[[nodiscard]]
	std::vector< Network > getCurrentConnections () noexcept;
}
