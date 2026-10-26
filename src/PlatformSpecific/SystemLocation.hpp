/*
 * src/PlatformSpecific/SystemLocation.hpp
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
#include <istream>
#include <optional>
#include <string>
#include <string_view>

namespace EmEn::PlatformSpecific
{
	/**
	 * @brief The machine's approximate place on Earth: the reference city of its time zone (owner decision 2026-10-26 —
	 * offline, no permission, a few hundred kilometres; the engine item celestial-clock-real-date-and-place).
	 */
	struct ApproximateLocation
	{
		/** @brief The latitude, in degrees, north positive. */
		double latitude{0.0};
		/** @brief The longitude, in degrees, EAST positive. */
		double longitude{0.0};
		/** @brief Where it comes from: the IANA zone ("Europe/Brussels") or the Windows one and its territory. */
		std::string source;
	};

	/**
	 * @brief Returns the machine's approximate place: the reference city of the system's time zone.
	 * @note Linux and macOS: the zone from TZ, else the /etc/localtime link, located in the tz database's own table
	 * (/usr/share/zoneinfo/zone1970.tab, else zone.tab). Windows: its time zone key and the user's region, located in
	 * the table generated from the CLDR windowsZones mapping and the tz database (tools/windows-zones/).
	 * @note A zone without a city (UTC, Etc/GMT+n) or an unreadable system answers nothing: the caller falls back to
	 * the engine's birthplace (Identification::BirthplaceLatitude / BirthplaceLongitude).
	 * @return std::optional< ApproximateLocation >
	 */
	[[nodiscard]]
	EMEN_API std::optional< ApproximateLocation > approximateLocation () noexcept;

	/**
	 * @brief Parses the tz database's ISO 6709 coordinates: ±DDMM±DDDMM or ±DDMMSS±DDDMMSS ("+5050+00420").
	 * @param text The coordinates.
	 * @return std::optional< ApproximateLocation > Nothing for a malformed text or one out of range (the source empty).
	 */
	[[nodiscard]]
	EMEN_API std::optional< ApproximateLocation > parseZoneCoordinates (std::string_view text) noexcept;

	/**
	 * @brief Finds a zone in a tz database table (zone1970.tab / zone.tab: TAB-separated "codes, coordinates, zone").
	 * @param table A reference to the table's stream.
	 * @param zone The IANA zone name ("Europe/Brussels").
	 * @return std::optional< ApproximateLocation > Nothing when the zone is absent or its line malformed.
	 */
	[[nodiscard]]
	EMEN_API std::optional< ApproximateLocation > findZone (std::istream & table, std::string_view zone) noexcept;
}
