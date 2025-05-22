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

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if lines are intersecting.
	 * @todo Same as segment-segment except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param lineA A reference to a line.
	 * @param lineB A reference to a line.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & lineA, const Line< precision_t > & lineB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & o1 = lineA.origin();
		const auto & d1 = lineA.direction();
		const auto & o2 = lineB.origin();
		const auto & d2 = lineB.direction();

		/* NOTE: A slightly larger epsilon is used for geometric comparisons. */
		constexpr precision_t epsilon = std::numeric_limits< precision_t >::epsilon() * 100;

		/* NOTE: Check if the lines are parallel or coincident. */
		const auto dirCross = Vector< 3, precision_t >::crossProduct(d1, d2);

		if ( dirCross.lengthSquared() < epsilon )
		{
			/* NOTE: The directions are parallel. The lines are either coincident or parallel and distinct. */
			const auto originVec = o2 - o1;
			const auto originCross = Vector< 3, precision_t >::crossProduct(originVec, d1);

			/* NOTE: If the vector between the origins is also parallel to the direction,
			 * the lines are on the same support: they are coincident. */
			if ( originCross.lengthSquared() < epsilon )
			{
				/* Coincident lines (they "intersect" everywhere) */
				return true;
			}

			/* Otherwise, they are parallel and distinct. */
			return false;
		}

		/* NOTE: Check whether the lines are intersecting or crooked.
		 * Non-parallel lines intersect if and only if they are coplanar.
		 * This is verified with the scalar triple product. */
		const auto originVec = o2 - o1;
		const auto tripleProduct = Vector< 3, precision_t >::dotProduct(originVec, dirCross);

		if ( std::abs(tripleProduct) < epsilon )
		{
			return true;
		}

		/* NOTE: If the mixed product is not zero, the lines are not coplanar. */
		return false;
	}

	/**
	 * @brief Checks if lines are intersecting and gives the intersection point.
	 * @todo Same as segment-segment except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param lineA A reference to a line.
	 * @param lineB A reference to a line.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & lineA, const Line< precision_t > & lineB, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & o1 = lineA.origin();
		const auto & d1 = lineA.direction();
		const auto & o2 = lineB.origin();
		const auto & d2 = lineB.direction();

		constexpr precision_t epsilon = std::numeric_limits<precision_t>::epsilon() * 100;

		/* Check that the lines are not parallel */
		const auto dirCross = Vector< 3, precision_t >::crossProduct(d1, d2);
		const auto dirCrossLenSq = dirCross.lengthSquared();

		if ( dirCrossLenSq < epsilon )
		{
			/* Parallel or coincident lines, no single intersection. */
			intersection.reset();

			return false;
		}

		/* NOTE: Check that the lines are coplanar (intersecting) and not crooked. */
		const auto originVec = o2 - o1;
		const auto tripleProduct = Vector< 3, precision_t >::dotProduct(originVec, dirCross);

		if ( std::abs(tripleProduct) >= epsilon )
		{
			/* NOTE: Not coplanar, therefore, left. */
			intersection.reset();

			return false;
		}

		/* NOTE: The lines are intersecting, we calculate the point of intersection.
		 * We solve the system to find the parameter 't' for line A: P(t) = o1 + t * d1 */
		const auto t = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(originVec, d2), dirCross) / dirCrossLenSq;
		intersection = o1 + d1 * t;

		return true;
	}
}
