/*
* src/Libs/Math/Space3D/Intersections.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Local inclusions. */
#include "Libs/Math/Space3D/Line.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a line is intersecting a cuboid.
	 * @todo Same as segment-cuboid except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !cuboid.isValid() )
		{
			return false;
		}

		const auto & origin = line.origin();
		const auto & direction = line.direction();
		const auto & min = cuboid.minimum();
		const auto & max = cuboid.maximum();

		/* NOTE: Initializes the interval to infinity because the line is infinite. */
		precision_t tMin = -std::numeric_limits< precision_t >::max();
		precision_t tMax = std::numeric_limits< precision_t >::max();

		/* Iterate over the 3 axes (X, Y, Z) */
		for ( size_t i = 0; i < 3; ++i )
		{
			if ( std::abs(direction[i]) < std::numeric_limits<precision_t>::epsilon() )
			{
				/* NOTE: The line is parallel to the slab of axis i.
				 * If its origin is outside, it cannot intersect the cuboid. */
				if ( origin[i] < min[i] || origin[i] > max[i] )
				{
					return false;
				}
			}
			else
			{
				/* NOTE: Calculate the intersection of the line with the two planes of the slab. */
				precision_t t1 = (min[i] - origin[i]) / direction[i];
				precision_t t2 = (max[i] - origin[i]) / direction[i];

				if ( t1 > t2 )
				{
					std::swap(t1, t2);
				}

				/* NOTE: Update the global intersection interval. */
				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				/* NOTE: If the interval has become invalid, there is no intersection. */
				if ( tMin > tMax )
				{
					return false;
				}
			}
		}

		/* NOTE: If a valid intersection interval remains, there is a collision. */
		return true;
	}

	/**
	 * @brief Checks if a line is intersecting a cuboid and gives the intersection point.
	 * @todo Same as segment-cuboid except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param cuboid A reference to a cuboid.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const AACuboid< precision_t > & cuboid, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !cuboid.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & origin = line.origin();
		const auto & direction = line.direction();
		const auto & min = cuboid.minimum();
		const auto & max = cuboid.maximum();

		/* NOTE: Initializes the interval to infinity because the line is infinite. */
		precision_t tMin = -std::numeric_limits< precision_t >::max();
		precision_t tMax = std::numeric_limits< precision_t >::max();

		/* Iterate over the 3 axes (X, Y, Z) */
		for ( size_t i = 0; i < 3; ++i )
		{
			if ( std::abs(direction[i]) < std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: The line is parallel to the slab of axis i. */
				if ( origin[i] < min[i] || origin[i] > max[i] )
				{
					intersection.reset();

					return false;
				}
			}
			else
			{
				/* NOTE: Calculate the intersection of the line with the two planes of the slab. */
				precision_t t1 = (min[i] - origin[i]) / direction[i];
				precision_t t2 = (max[i] - origin[i]) / direction[i];

				if ( t1 > t2 )
				{
					std::swap(t1, t2);
				}

				/* NOTE: Update the global intersection interval. */
				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				/* NOTE: If the interval has become invalid, there is no intersection. */
				if ( tMin > tMax )
				{
					intersection.reset();

					return false;
				}
			}
		}

		/* NOTE: An intersection was found. The entry point is at tMin. */
		intersection = origin + direction * tMin;

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, AACuboid Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AACuboid< precision_t > & cuboid, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, cuboid);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, AACuboid Triangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AACuboid< precision_t > & cuboid, const Line< precision_t > & line, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, cuboid, intersection);
	}
}
