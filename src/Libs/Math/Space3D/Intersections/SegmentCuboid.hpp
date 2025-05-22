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
#include "Libs/Math/Space3D/Segment.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a segment intersects a cuboid.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !cuboid.isValid() )
		{
			return false;
		}

		const auto & origin = segment.startPoint();
		const auto direction = segment.endPoint() - origin;
		const auto & min = cuboid.minimum();
		const auto & max = cuboid.maximum();

		precision_t tMin = 0.0F;
		precision_t tMax = 1.0F;

		/* NOTE: Iterate over the 3 axes (X, Y, Z). */
		for ( size_t i = 0; i < 3; ++i )
		{
			if ( std::abs(direction[i]) < std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: The segment is parallel to the slab of axis i. If it is outside the slab, there is no collision. */
				if ( origin[i] < min[i] || origin[i] > max[i] )
				{
					return false;
				}
			}
			else
			{
				/* NOTE: Calculate the intersection of the segment with the two planes of the slab. */
				precision_t t1 = (min[i] - origin[i]) / direction[i];
				precision_t t2 = (max[i] - origin[i]) / direction[i];

				/* NOTE: Ensure that t1 is the closest intersection. */
				if ( t1 > t2 )
				{
					std::swap(t1, t2);
				}

				/* NOTE: Update the global collision interval. */
				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				/* NOTE: If the collision interval has become invalid, there is no intersection. */
				if ( tMin > tMax )
				{
					return false;
				}
			}
		}

		return true;
	}

	/**
	 * @brief Checks if a segment intersects a cuboid and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param cuboid A reference to a cuboid.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const AACuboid< precision_t > & cuboid, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !cuboid.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & origin = segment.startPoint();
		const auto direction = segment.endPoint() - origin;
		const auto & min = cuboid.minimum();
		const auto & max = cuboid.maximum();

		precision_t tMin = 0.0F;
		precision_t tMax = 1.0F;

		/* NOTE: Iterate over the 3 axes (X, Y, Z). */
		for ( size_t i = 0; i < 3; ++i )
		{
			if ( std::abs(direction[i]) < std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: The segment is parallel to the slab of axis i. */
				if ( origin[i] < min[i] || origin[i] > max[i] )
				{
					intersection.reset();

					return false;
				}
			}
			else
			{
				/* NOTE: Calculate the intersection of the segment with the two planes of the slab. */
				precision_t t1 = (min[i] - origin[i]) / direction[i];
				precision_t t2 = (max[i] - origin[i]) / direction[i];

				if ( t1 > t2 )
				{
					std::swap(t1, t2);
				}

				tMin = std::max(tMin, t1);
				tMax = std::min(tMax, t2);

				if ( tMin > tMax )
				{
					intersection.reset();

					return false;
				}
			}
		}

		/* NOTE: An intersection was found.
		 * The entry point is at tMin. */
		intersection = origin + direction * tMin;

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, AACuboid< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AACuboid< precision_t > & cuboid, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, cuboid);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, AACuboid< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AACuboid< precision_t > & cuboid, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, cuboid, intersection);
	}
}
