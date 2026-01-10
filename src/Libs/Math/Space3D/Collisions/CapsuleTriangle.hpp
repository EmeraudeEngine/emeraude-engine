/*
 * src/Libs/Math/Space3D/Collisions/CapsuleTriangle.hpp
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
#include "Libs/Math/Space3D/Capsule.hpp"
#include "Libs/Math/Space3D/Triangle.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Helper to find the closest point on a triangle to a given point.
	 * @tparam precision_t The data precision.
	 * @param point The point.
	 * @param triangle The triangle.
	 * @return Point< precision_t > The closest point on the triangle.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	Point< precision_t >
	closestPointOnTriangle (const Point< precision_t > & point, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: Utility function to find the nearest point on a line segment. */
		auto closestPointOnSegment = [] (const Point< precision_t > & p, const Point< precision_t > & a, const Point< precision_t > & b)
		{
			const auto ab = b - a;
			const auto lengthSq = ab.lengthSquared();

			if ( lengthSq < std::numeric_limits< precision_t >::epsilon() )
			{
				return a;
			}

			const auto ap = p - a;
			const precision_t t = Vector< 3, precision_t >::dotProduct(ap, ab) / lengthSq;
			const precision_t clampedT = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));

			return a + (ab * clampedT);
		};

		const auto & triPoints = triangle.points();
		const auto & pointA = triPoints[0];
		const auto & pointB = triPoints[1];
		const auto & pointC = triPoints[2];

		/* NOTE: Find the closest point on the plane of the triangle. */
		const auto normal = Vector< 3, precision_t >::normal(pointA, pointB, pointC);

		if ( normal.isZero() )
		{
			/* NOTE: Degenerate triangle. Return the first vertex. */
			return pointA;
		}

		const auto distanceToPlane = Vector< 3, precision_t >::dotProduct(point - pointA, normal);
		const auto pointOnPlane = point - (normal * distanceToPlane);

		/* NOTE: Check if this point is inside the triangle (using the "same side" technique). */
		const auto c1 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(pointB - pointA, pointOnPlane - pointA), normal);
		const auto c2 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(pointC - pointB, pointOnPlane - pointB), normal);
		const auto c3 = Vector< 3, precision_t >::dotProduct(Vector< 3, precision_t >::crossProduct(pointA - pointC, pointOnPlane - pointC), normal);

		if ( c1 >= 0 && c2 >= 0 && c3 >= 0 )
		{
			/* NOTE: The projection is inside the triangle. */
			return pointOnPlane;
		}

		/* NOTE: The projection is outside. Find the closest point on the edges. */
		const auto pAB = closestPointOnSegment(point, pointA, pointB);
		const auto pBC = closestPointOnSegment(point, pointB, pointC);
		const auto pCA = closestPointOnSegment(point, pointC, pointA);

		const auto dAB = Vector< 3, precision_t >::distanceSquared(point, pAB);
		const auto dBC = Vector< 3, precision_t >::distanceSquared(point, pBC);
		const auto dCA = Vector< 3, precision_t >::distanceSquared(point, pCA);

		if ( dAB < dBC && dAB < dCA )
		{
			return pAB;
		}
		else if ( dBC < dCA )
		{
			return pBC;
		}

		return pCA;
	}

	/**
	 * @brief Finds the closest points between a capsule axis and a triangle.
	 * @note Uses iterative refinement for accuracy.
	 * @tparam precision_t The data precision.
	 * @param capsule The capsule.
	 * @param triangle The triangle.
	 * @param closestOnAxis Output: closest point on capsule axis.
	 * @param closestOnTriangle Output: closest point on triangle.
	 * @return void
	 */
	template< typename precision_t = float >
	void
	closestPointsCapsuleTriangle (const Capsule< precision_t > & capsule, const Triangle< precision_t > & triangle, Point< precision_t > & closestOnAxis, Point< precision_t > & closestOnTriangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: Start with the centroid of the capsule axis. */
		closestOnAxis = capsule.centroid();
		closestOnTriangle = closestPointOnTriangle(closestOnAxis, triangle);

		/* NOTE: Iterative refinement. */
		for ( int iteration = 0; iteration < 4; ++iteration )
		{
			closestOnAxis = capsule.closestPointOnAxis(closestOnTriangle);
			closestOnTriangle = closestPointOnTriangle(closestOnAxis, triangle);
		}
	}

	/**
	 * @brief Checks if a capsule is colliding with a triangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !triangle.isValid() )
		{
			return false;
		}

		/* NOTE: Find the closest points between capsule axis and triangle. */
		Point< precision_t > closestOnAxis, closestOnTriangle;
		closestPointsCapsuleTriangle(capsule, triangle, closestOnAxis, closestOnTriangle);

		/* NOTE: Check if the distance is within the capsule radius. */
		const auto distanceSq = Vector< 3, precision_t >::distanceSquared(closestOnAxis, closestOnTriangle);

		return distanceSq <= capsule.squaredRadius();
	}

	/**
	 * @brief Checks if a capsule is colliding with a triangle and gives the MTV.
	 * @note The MTV pushes the capsule out of the triangle (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param triangle A reference to a triangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Triangle< precision_t > & triangle, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !triangle.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Find the closest points between capsule axis and triangle. */
		Point< precision_t > closestOnAxis, closestOnTriangle;
		closestPointsCapsuleTriangle(capsule, triangle, closestOnAxis, closestOnTriangle);

		const auto axisToTriangle = closestOnTriangle - closestOnAxis;
		const auto distanceSq = axisToTriangle.lengthSquared();
		const auto radiusSq = capsule.squaredRadius();

		if ( distanceSq > radiusSq )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Collision detected. Compute MTV. */
		const auto distance = std::sqrt(distanceSq);

		if ( distance > std::numeric_limits< precision_t >::epsilon() )
		{
			const auto overlap = capsule.radius() - distance;

			/* NOTE: MTV points from triangle towards capsule axis, pushing capsule away from triangle. */
			minimumTranslationVector = (-axisToTriangle / distance) * overlap;
		}
		else
		{
			/* NOTE: The capsule axis intersects the triangle plane. Push along triangle normal. */
			const auto & triPoints = triangle.points();
			const auto normal = Vector< 3, precision_t >::normal(triPoints[0], triPoints[1], triPoints[2]);

			minimumTranslationVector = normal * capsule.radius();
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Capsule< precision_t > &, const Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(capsule, triangle);
	}

	/**
	 * @brief Checks if a triangle is colliding with a capsule and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the triangle out of the capsule (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param capsule A reference to a capsule.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Capsule< precision_t > & capsule, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: isColliding(capsule, triangle, mtv) computes MTV to push capsule out of triangle.
		 * We need the opposite: push triangle out of capsule, so we negate the MTV. */
		if ( isColliding(capsule, triangle, minimumTranslationVector) )
		{
			minimumTranslationVector = -minimumTranslationVector;

			return true;
		}

		return false;
	}
}
