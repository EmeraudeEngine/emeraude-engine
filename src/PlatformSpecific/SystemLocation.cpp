/*
 * src/PlatformSpecific/SystemLocation.cpp
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
#include <cstddef>
#include <string>

namespace
{
	/**
	 * @brief Reads one ISO 6709 component: a sign, then DD(D)MM or DD(D)MMSS.
	 * @param text The component.
	 * @param degreeDigits 2 for a latitude, 3 for a longitude.
	 * @return std::optional< double > In degrees.
	 */
	[[nodiscard]]
	std::optional< double >
	readComponent (std::string_view text, size_t degreeDigits) noexcept
	{
		if ( text.size() != 1 + degreeDigits + 2 && text.size() != 1 + degreeDigits + 4 )
		{
			return std::nullopt;
		}

		const auto sign = text.front();

		if ( sign != '+' && sign != '-' )
		{
			return std::nullopt;
		}

		/* At most three ASCII digits (the size is checked above): no overflow, no locale. */
		const auto number = [text] (size_t offset, size_t count) -> std::optional< int > {
			int value = 0;

			for ( const auto digit : text.substr(offset, count) )
			{
				if ( digit < '0' || digit > '9' )
				{
					return std::nullopt;
				}

				value = (value * 10) + (digit - '0');
			}

			return value;
		};

		const auto degrees = number(1, degreeDigits);
		const auto minutes = number(1 + degreeDigits, 2);
		const auto seconds = text.size() == 1 + degreeDigits + 4 ? number(3 + degreeDigits, 2) : std::optional< int >{0};

		if ( !degrees.has_value() || !minutes.has_value() || !seconds.has_value() || minutes.value() >= 60 || seconds.value() >= 60 )
		{
			return std::nullopt;
		}

		const auto value = static_cast< double >(degrees.value()) + (static_cast< double >(minutes.value()) / 60.0) + (static_cast< double >(seconds.value()) / 3600.0);

		return sign == '-' ? -value : value;
	}
}

namespace EmEn::PlatformSpecific
{
	std::optional< ApproximateLocation >
	parseZoneCoordinates (std::string_view text) noexcept
	{
		/* The longitude starts at the second sign. */
		const auto split = text.find_first_of("+-", 1);

		if ( split == std::string_view::npos )
		{
			return std::nullopt;
		}

		const auto latitude = readComponent(text.substr(0, split), 2);
		const auto longitude = readComponent(text.substr(split), 3);

		if ( !latitude.has_value() || !longitude.has_value() || latitude.value() < -90.0 || latitude.value() > 90.0 || longitude.value() < -180.0 || longitude.value() > 180.0 )
		{
			return std::nullopt;
		}

		return ApproximateLocation{.latitude = latitude.value(), .longitude = longitude.value(), .source = {}};
	}

	std::optional< ApproximateLocation >
	findZone (std::istream & table, std::string_view zone) noexcept
	{
		if ( zone.empty() )
		{
			return std::nullopt;
		}

		std::string line;

		/* A table of a few hundred lines: the loop ends with the stream. */
		while ( std::getline(table, line) )
		{
			if ( line.empty() || line.front() == '#' )
			{
				continue;
			}

			/* codes TAB coordinates TAB zone [TAB comments] */
			const auto first = line.find('\t');
			const auto second = first == std::string::npos ? std::string::npos : line.find('\t', first + 1);

			if ( second == std::string::npos )
			{
				continue;
			}

			const auto third = line.find('\t', second + 1);
			const std::string_view row{line};
			const auto name = row.substr(second + 1, (third == std::string::npos ? line.size() : third) - second - 1);

			if ( name != zone )
			{
				continue;
			}

			auto location = parseZoneCoordinates(row.substr(first + 1, second - first - 1));

			if ( location.has_value() )
			{
				location->source = std::string{zone};
			}

			return location;
		}

		return std::nullopt;
	}
}
