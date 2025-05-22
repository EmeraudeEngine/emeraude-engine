/*
* src/Libs/Math/Space3D/Collisions.hpp
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
#include "Libs/Math/Space3D/Triangle.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a triangle is colliding with a sphere.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param sphere A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() || !sphere.isValid() )
		{
			return false;
		}

		/* NOTE: Utility function to find the nearest point on a line segment. */
		auto closestPointOnSegment = [] (const Point< precision_t > & point, const Point< precision_t > & a, const Point< precision_t > & b)
		{
			const auto ab = b - a;
			const auto ap = point - a;
			const precision_t t = Vector< 3, precision_t >::dotProduct(ap, ab) / ab.lengthSquared();
			const precision_t clampedT = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));

			return a + (ab * clampedT);
		};

	    const auto & triPoints = triangle.points();
	    const auto & A = triPoints[0];
	    const auto & B = triPoints[1];
	    const auto & C = triPoints[2];
	    const auto & S = sphere.position();

	    /* NOTE: Find the closest point on the plane of the triangle. */
	    const auto normal = Vector< 3, precision_t >::normal(A, B, C);
	    const auto distanceToPlane = Vector< 3, precision_t >::dotProduct(S - A, normal);
	    const auto pointOnPlane = S - (normal * distanceToPlane);

		/* NOTE: Check if this point is inside the triangle (using the "same side" technique with the vector product). */
		const auto c1 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(B - A, pointOnPlane - A), normal);
		const auto c2 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(C - B, pointOnPlane - B), normal);
		const auto c3 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(A - C, pointOnPlane - C), normal);

		Point< precision_t > closestPoint;

		if ( c1 >= 0 && c2 >= 0 && c3 >= 0 )
		{
			/* NOTE: The projection is inside the triangle. This is the closest point. */
			closestPoint = pointOnPlane;
		}
		else
		{
			/* NOTE: The projection is outward. The closest point is on one of the edges. */
			const auto pAB = closestPointOnSegment(S, A, B);
			const auto pBC = closestPointOnSegment(S, B, C);
			const auto pCA = closestPointOnSegment(S, C, A);

			const auto dAB = Vector< 3, precision_t >::distanceSquared(S, pAB);
			const auto dBC = Vector< 3, precision_t >::distanceSquared(S, pBC);
			const auto dCA = Vector< 3, precision_t >::distanceSquared(S, pCA);

			if ( dAB < dBC && dAB < dCA )
			{
				closestPoint = pAB;
			}
			else if ( dBC < dCA )
			{
				closestPoint = pBC;
			}
			else
			{
				closestPoint = pCA;
			}
		}

	    /* NOTE: Compare the final distance to the radius of the sphere. */
	    return Vector< 3, precision_t >::distanceSquared(S, closestPoint) <= sphere.squaredRadius();
	}

	/**
	 * @brief Checks if a triangle is colliding with a sphere and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param sphere A reference to a sphere.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Sphere< precision_t > & sphere, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() || !sphere.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: The logic to find the closest point is the same as the previous function. */
		auto closestPointOnSegment = [] (const Point< precision_t > & point, const Point< precision_t > & a, const Point< precision_t > & b)
		{
		    const auto ab = b - a;
		    const auto ap = point - a;

		    /* NOTE: Handle the case where the segment has zero lengths. */
		    const auto lenSq = ab.lengthSquared();

		    if ( lenSq < std::numeric_limits< precision_t >::epsilon() )
		    {
				return a;
			}

			const precision_t t = Vector< 3, precision_t >::dotProduct(ap, ab) / lenSq;
		    const precision_t clampedT = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));
			
		    return a + (ab * clampedT);
		};

		const auto & triPoints = triangle.points();
		const auto & A = triPoints[0];
		const auto & B = triPoints[1];
		const auto & C = triPoints[2];
		const auto & S = sphere.position();

		const auto normal = Vector< 3, precision_t >::normal(A, B, C);
		const auto distanceToPlane = Vector< 3, precision_t >::dotProduct(S - A, normal);
		const auto pointOnPlane = S - (normal * distanceToPlane);

		const auto c1 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(B - A, pointOnPlane - A), normal);
		const auto c2 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(C - B, pointOnPlane - B), normal);
		const auto c3 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(A - C, pointOnPlane - C), normal);

		Point< precision_t > closestPoint;

		if ( c1 >= 0 && c2 >= 0 && c3 >= 0 )
		{
		    closestPoint = pointOnPlane;
		}
		else
		{
		    const auto pAB = closestPointOnSegment(S, A, B);
		    const auto pBC = closestPointOnSegment(S, B, C);
		    const auto pCA = closestPointOnSegment(S, C, A);

		    const auto dAB = Vector< 3, precision_t >::distanceSquared(S, pAB);
		    const auto dBC = Vector< 3, precision_t >::distanceSquared(S, pBC);
		    const auto dCA = Vector< 3, precision_t >::distanceSquared(S, pCA);

		    if ( dAB < dBC && dAB < dCA )
		    {
		        closestPoint = pAB;
		    }
			else if ( dBC < dCA )
			{
		        closestPoint = pBC;
		    }
			else
			{
		        closestPoint = pCA;
		    }
		}

		/* NOTE: End of search for the nearest point.
		 * Calculate distance and check collision */
		const auto delta = S - closestPoint;
		const auto distanceSq = delta.lengthSquared();

		if ( distanceSq > sphere.squaredRadius() )
		{
			/* No collision. */
		    minimumTranslationVector.reset();

		    return false;
		}

		/* NOTE: Collision confirmed, MTV calculated. */
		const auto distance = std::sqrt(distanceSq);
		const auto overlap = sphere.radius() - distance;

		if ( distance > std::numeric_limits< precision_t >::epsilon() )
		{
		    /* NOTE: The direction of the MTV is that of the delta vector (from the nearest point to the center of the sphere). */
		    minimumTranslationVector = (delta / distance) * overlap;
		}
		else
		{
		    /* NOTE: The center of the sphere is on the triangle. We push along the normal of the triangle. */
		    minimumTranslationVector = normal * sphere.radius();
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Triangle< precision_t > &, const Sphere< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, sphere);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Triangle< precision_t > &, const Sphere< precision_t > &, Vector< 3, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Triangle< precision_t > & triangle, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, sphere, minimumTranslationVector);
	}
}
