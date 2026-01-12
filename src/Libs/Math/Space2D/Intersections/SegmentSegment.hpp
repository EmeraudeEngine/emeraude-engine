/*
 * src/Libs/Math/Space2D/Intersections/SegmentSegment.hpp
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

/* Local inclusions. */
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
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
	bool
	isIntersecting (const Segment< precision_t > & segmentA, const Segment< precision_t > & segmentB, Point< precision_t > & intersection) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: Direction vectors for each segment. */
		const auto directionA = segmentA.endPoint() - segmentA.startPoint();
		const auto directionB = segmentB.endPoint() - segmentB.startPoint();

		/* NOTE: The denominator is the 2D cross-product of the direction vectors. If it is zero, the lines are parallel or collinear. */
		const precision_t denominator = Vector< 2, precision_t >::crossProduct(directionA, directionB);

		/* NOTE: Check for parallel lines (denominator close to zero). */
		if ( std::abs(denominator) < std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: Lines are parallel. Check if they are collinear and overlap. */
			const auto startDiff = segmentB.startPoint() - segmentA.startPoint();
			const auto crossCheck = Vector< 2, precision_t >::crossProduct(directionA, startDiff);

			/* NOTE: If not collinear, no intersection. */
			if ( std::abs(crossCheck) > std::numeric_limits< precision_t >::epsilon() )
			{
				return false;
			}

			/* NOTE: Segments are collinear. Check for overlap by projecting onto segment A's direction. */
			const precision_t lengthSqA = directionA.lengthSquared();
			if ( lengthSqA < std::numeric_limits< precision_t >::epsilon() )
			{
				/* Segment A is a point */
				return false;
			}

			/* Project segment B's endpoints onto segment A's line */
			const precision_t t0 = Vector< 2, precision_t >::dotProduct(startDiff, directionA) / lengthSqA;
			const precision_t t1 = Vector< 2, precision_t >::dotProduct(segmentB.endPoint() - segmentA.startPoint(), directionA) / lengthSqA;

			/* Check if intervals [0,1] and [t0,t1] overlap */
			const precision_t minT = std::min(t0, t1);
			const precision_t maxT = std::max(t0, t1);

			/* Overlap exists if max(0, minT) <= min(1, maxT) */
			if ( std::max(static_cast<precision_t>(0), minT) <= std::min(static_cast<precision_t>(1), maxT) )
			{
				/* Segments overlap. Set intersection to the first overlap point. */
				const precision_t overlapStart = std::max(static_cast<precision_t>(0), minT);
				intersection = segmentA.startPoint() + directionA * overlapStart;
				return true;
			}

			return false;
		}

		/* NOTE: The vector between the starting points of the two segments. */
		const auto startDiff = segmentB.startPoint() - segmentA.startPoint();

		/* NOTE: Computes the parameters t and u. 't' is the parameter for segment A, 'u' for segment B.
		 * The intersection occurs at `segmentAStart + t * dirA` and also at `segmentBStart + u * dirB`. */
		const precision_t t = Vector< 2, precision_t >::crossProduct(startDiff, directionB) / denominator;
		const precision_t u = Vector< 2, precision_t >::crossProduct(startDiff, directionA) / denominator;

		/* NOTE: The intersection is only valid if it lies ON both segments.
		 * For this, the parameters t and u must be in the interval [0, 1]. */
		if ( t >= 0 && t <= 1 && u >= 0 && u <= 1 )
		{
			/* NOTE: Calculates the intersection point and stores it in the output parameter. */
			intersection = segmentA.startPoint() + directionA * t;

			return true;
		}

		/* NOTE: The lines intersect, but outside the segments. */
		return false;
	}

	/**
	 * @brief Checks if segments are intersecting.
	 * @tparam precision_t The data precision. Default float.
	 * @param segmentA A reference to a segment.
	 * @param segmentB A reference to a segment.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Segment< precision_t > & segmentA, const Segment< precision_t > & segmentB) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		Point< precision_t > dummy;

		return isIntersecting(segmentA, segmentB, dummy);
	}
}
