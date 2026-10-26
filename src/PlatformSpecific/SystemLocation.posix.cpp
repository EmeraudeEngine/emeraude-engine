/*
 * src/PlatformSpecific/SystemLocation.posix.cpp
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

#include "SystemLocation.hpp"

/* STL inclusions. */
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

/* Local inclusions. */
#include "IO/IO.hpp"
#include "PlatformSpecific/Helpers.hpp"

namespace
{
	/**
	 * @brief Returns the system's IANA time zone: TZ ("Europe/Brussels", ":Europe/Brussels", a path into a zoneinfo
	 * directory), else the /etc/localtime link (Linux: /usr/share/zoneinfo/Europe/Brussels; macOS:
	 * /var/db/timezone/zoneinfo/Europe/Brussels) — what follows "zoneinfo/".
	 * @return std::string Empty when unknown.
	 */
	[[nodiscard]]
	std::string
	systemZone () noexcept
	{
		const auto afterZoneinfo = [] (const std::string & path) {
			constexpr std::string_view Marker{"zoneinfo/"};
			const auto position = path.rfind(Marker);

			return position == std::string::npos ? path : path.substr(position + Marker.size());
		};

		/* A READ through the platform helper: the cascade writes the environment only before any thread
		 * (Helpers.hpp, readEnvironmentVariable()). */
		if ( auto zone = EmEn::PlatformSpecific::readEnvironmentVariable("TZ"); !zone.empty() )
		{
			if ( zone.front() == ':' )
			{
				zone.erase(0, 1);
			}

			return afterZoneinfo(zone);
		}

		std::error_code error;
		const auto target = std::filesystem::read_symlink("/etc/localtime", error);

		if ( error )
		{
			return {};
		}

		return afterZoneinfo(target.generic_string());
	}
}

namespace EmEn::PlatformSpecific
{
	std::optional< ApproximateLocation >
	approximateLocation () noexcept
	{
		const auto zone = systemZone();

		if ( zone.empty() )
		{
			return std::nullopt;
		}

		/* zone1970.tab first (the current zones), zone.tab for an older name it merged away. */
		for ( const auto * tablePath : std::array< const char *, 2 >{"/usr/share/zoneinfo/zone1970.tab", "/usr/share/zoneinfo/zone.tab"} )
		{
			std::ifstream table;

			if ( !Base::IO::openInputFile(tablePath, table, std::ios::in) )
			{
				continue;
			}

			if ( auto location = findZone(table, zone) )
			{
				return location;
			}
		}

		return std::nullopt;
	}
}
