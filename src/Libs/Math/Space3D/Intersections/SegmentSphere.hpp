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
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a segment intersects a sphere.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param sphere A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !sphere.isValid() )
		{
			return false;
		}

		const auto & startPoint = segment.startPoint();
		const auto & endPoint = segment.endPoint();
		const auto & sphereCenter = sphere.position();

		/* NOTE: Find the closest point on the segment to the center of the sphere. */
		const auto segmentVector = endPoint - startPoint;
		const auto startToCenter = sphereCenter - startPoint;

		/* NOTE: Project the startToCenter vector onto the segment vector to find the nearest point on the infinite line.
		 * The 't' parameter represents the position of this projection on the line. */
		const auto segmentLengthSq = segmentVector.lengthSquared();
		const precision_t t = Vector< 3, precision_t> ::dotProduct(startToCenter, segmentVector) / segmentLengthSq;

		/* NOTE: Limit 't' to the interval [0, 1] to ensure that the point remains on the segment. */
		const precision_t clampedT = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));

		/* NOTE: Calculate the coordinates of the nearest point on the segment. */
		const Point< precision_t > closestPoint = startPoint + (segmentVector * clampedT);

		/* NOTE: Check if the distance between this point and the center of the sphere is less than or equal to the radius.
		 * Squared distances are used for performance. */
		return Vector< 3, precision_t >::distanceSquared(sphereCenter, closestPoint) <= sphere.squaredRadius();
	}

	/**
	 * @brief Checks if a segment intersects a sphere and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param sphere A reference to a sphere.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Sphere< precision_t > & sphere, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !sphere.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & segmentStart = segment.start();
		const auto & segmentEnd = segment.end();
		const auto & center = sphere.position();
		const auto radius = sphere.radius();

		const Point< precision_t > segmentDirection = segmentEnd - segmentStart;
		/* NOTE: Vector from the center of the sphere to the start of the segment. */
		const Point< precision_t > oc = segmentStart - center;

		const precision_t a = Point< precision_t >::dotProduct(segmentDirection, segmentDirection);
		const precision_t b = static_cast< precision_t >(2) * Point< precision_t >::dotProduct(segmentDirection, oc);
		const precision_t c = Point< precision_t >::dotProduct(oc, oc) - radius * radius;

		/* NOTE: Calculation of the discriminant of the quadratic equation. */
		const precision_t discriminant = b * b - 4 * a * c;

		/* NOTE: If the discriminant is negative, the line never intersects the sphere. */
		if ( discriminant < 0 )
		{
		    return false;
		}

		/* NOTE: Solving the equation to find the intersection points t1 and t2. */
		const precision_t sqrtDiscriminant = std::sqrt(discriminant);
		const precision_t t1 = (-b - sqrtDiscriminant) / (static_cast< precision_t >(2) * a);
		const precision_t t2 = (-b + sqrtDiscriminant) / (static_cast< precision_t >(2) * a);

		/* NOTE: We look for the smallest positive t in the interval [0, 1] to find
		 * the intersection point closest to the start of the segment. */

		/* NOTE: Case 1: The first intersection point (t1) is on the segment. */
		if ( t1 >= 0 && t1 <= 1 )
		{
		    intersection = segmentStart + segmentDirection * t1;

		    return true;
		}

		/* NOTE: Case 2: The second intersection point (t2) is on the segment. This happens if the segment starts inside the sphere. */
		if ( t2 >= 0 && t2 <= 1 )
		{
		    intersection = segmentStart + segmentDirection * t2;

		    return true;
		}

		/* NOTE: Case 3: The segment is entirely contained within the sphere (t1 < 0 and t2 > 1).
		 * In this case, there is indeed an intersection (the entire volume of the segment is within the sphere).
		 * We can consider the starting point as a valid intersection point. */
		if ( t1 < 0 && t2 > 1 )
		{
		    intersection = segmentStart;

		    return true;
		}

		/* NOTE: No intersection point was found on the segment. */
		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, Sphere< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Sphere< precision_t > & sphere, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, sphere);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, Sphere< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Sphere< precision_t > & sphere, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, sphere, intersection);
	}
}
