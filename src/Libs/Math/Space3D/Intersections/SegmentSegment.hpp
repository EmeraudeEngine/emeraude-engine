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

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if segments are intersecting and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segmentA A reference to a segment.
	 * @param segmentB A reference to a segment.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segmentA, const Segment< precision_t > & segmentB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segmentA.isValid() || !segmentB.isValid() )
		{
			return false;
		}

		const auto p1 = segmentA.startPoint();
		const auto d1 = segmentA.endPoint() - p1;
		const auto p2 = segmentB.startPoint();
		const auto d2 = segmentB.endPoint() - p2;

		const auto r = p1 - p2;
		const auto a = d1.lengthSquared();
		const auto e = d2.lengthSquared();
		const auto f = Vector< 3, precision_t >::dotProduct(d2, r);

		constexpr precision_t epsilon = std::numeric_limits< precision_t >::epsilon();

		/* NOTE: the segments are parallel. */
		if ( a <= epsilon && e <= epsilon )
		{
		    return Vector< 3, precision_t >::distanceSquared(p1, p2) <= epsilon;
		}

		/* NOTE: Segment A is a point. */
		if ( a <= epsilon )
		{
		    return Point< precision_t >::distanceSquaredToLine(p1, p2, d2) <= epsilon;
		}

		/* NOTE: Segment B is a point. */
		if ( e <= epsilon )
		{
		    return Point< precision_t >::distanceSquaredToLine(p2, p1, d1) <= epsilon;
		}

		const auto c = Vector< 3, precision_t >::dotProduct(d1, r);
		const auto b = Vector< 3, precision_t >::dotProduct(d1, d2);
		const auto denom = a * e - b * b;

		precision_t s, t;

		/* NOTE: Lines are parallel if the denominator is close to zero. */
		if ( denom > -epsilon && denom < epsilon )
		{
		    /* NOTE: In this case, the segments only intersect if they are collinear.
		     * We check whether the points are aligned. */
		    const auto cross = Vector< 3, precision_t >::crossProduct(d1, r);

		    if ( cross.lengthSquared() > epsilon )
		    {
		    	/* Non-collinear */
		        return false;
		    }

		    /* NOTE: If they are collinear, there is an intersection if they overlap. */
		    const auto t0 = Vector< 3, precision_t >::dotProduct(p2 - p1, d1) / a;
		    const auto t1 = t0 + Vector< 3, precision_t >::dotProduct(d2, d1) / a;

		    return (std::max(static_cast<precision_t>(0), std::min(t0, t1)) <= std::min(static_cast<precision_t>(1), std::max(t0, t1)));
		}

		s = (b * f - c * e) / denom;
		t = (a * f - b * c) / denom;

		/* NOTE: An intersection is valid only if the closest points are on segments
		 * (s and t between 0 and 1) and if these points coincide. */
		if ( s >= 0 && s <= 1 && t >= 0 && t <= 1 )
		{
		    const auto closestPointA = p1 + d1 * s;
		    const auto closestPointB = p2 + d2 * t;

		    return Vector< 3, precision_t >::distanceSquared(closestPointA, closestPointB) <= epsilon;
		}

		return false;
	}

	/**
	 * @brief Checks if segments are intersecting and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segmentA A reference to a segment.
	 * @param segmentB A reference to a segment.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segmentA, const Segment< precision_t > & segmentB, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segmentA.isValid() || !segmentB.isValid() )
		{
			intersection.reset();
			
			return false;
		}

		const auto p1 = segmentA.startPoint();
		const auto d1 = segmentA.endPoint() - p1;
		const auto p2 = segmentB.startPoint();
		const auto d2 = segmentB.endPoint() - p2;

		const auto r = p1 - p2;
		const auto a = d1.lengthSquared();
		const auto e = d2.lengthSquared();
		const auto b = Vector< 3, precision_t >::dotProduct(d1, d2);
		const auto denom = a * e - b * b;

		constexpr precision_t epsilon = std::numeric_limits<precision_t>::epsilon();

		/* NOTE: If the denominator is close to zero, the lines are parallel. */
		if ( denom > -epsilon && denom < epsilon )
		{
			/* NOTE: The collinear case has infinitely many intersection points, not just one. So we treat it as a non-intersection for this function. */
			intersection.reset();
			
			return false;
		}

		const auto c = Vector< 3, precision_t >::dotProduct(d1, r);
		const auto f = Vector< 3, precision_t >::dotProduct(d2, r);

		const auto s = (b * f - c * e) / denom;
		const auto t = (a * f - b * c) / denom;

		/* NOTE: The intersection is valid if it lies on both segments (s and t between 0 and 1). */
		if ( s >= 0 && s <= 1 && t >= 0 && t <= 1 )
		{
			const auto closestPointA = p1 + d1 * s;
			const auto closestPointB = p2 + d2 * t;

			/* NOTE: We check that the closest points coincide well. */
			if ( Vector< 3, precision_t >::distanceSquared(closestPointA, closestPointB) <= epsilon )
			{
				intersection = closestPointA;

				return true;
			}
		}

		intersection.reset();

		return false;
	}
}
