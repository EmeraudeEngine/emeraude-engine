/*
* src/Libs/Math/Space2D/Intersections.hpp
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
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Triangle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a segment intersects a triangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO ...

		return false;
	}

	/**
	 * @brief Checks if a segment intersects a triangle and gives the intersection point.
	 * @note The intersection is valid for both faces of the triangle (no backface culling).
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param triangle A reference to a triangle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Triangle< precision_t > & triangle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		std::vector< Point< precision_t > > intersections;
		Point< precision_t > currentIntersection;

		/* NOTE: Test the intersection with each edge and store the results. */
		if ( isIntersecting(segment.start(), segment.end(), triangle.pointA(), triangle.pointB(), currentIntersection) )
		{
			intersections.emplace_back(currentIntersection);
		}

		if ( isIntersecting(segment.start(), segment.end(), triangle.pointB(), triangle.pointC(), currentIntersection) )
		{
			intersections.emplace_back(currentIntersection);
		}

		if ( isIntersecting(segment.start(), segment.end(), triangle.pointC(), triangle.pointA(), currentIntersection) )
		{
			intersections.emplace_back(currentIntersection);
		}

		if ( !intersections.empty() )
		{
			/* NOTE: The segment crosses one or more edges. Find the closest point. */
			precision_t minDistSq = std::numeric_limits< precision_t >::max();
			Point< precision_t > closestIntersection;

			for ( const auto & point : intersections )
			{
				const precision_t distSq = Point< precision_t >::distanceSquared(segment.start(), point);

				if ( distSq < minDistSq )
				{
					minDistSq = distSq;
					closestIntersection = point;
				}
			}

			intersection = closestIntersection;

			return true;
		}

		/* NOTE: If no edge is cut, check if the segment is inside. */
		if ( contains(segment.start(), triangle) )
		{
			/* NOTE: The segment starts inside the triangle. */
			intersection = segment.start();

			return true;
		}

		/* NOTE: No intersection. */
		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, Triangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, triangle, intersection);
	}
}
