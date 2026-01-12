/*
 * src/Libs/Math/Space3D/Collisions/SamePrimitive.hpp
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

/* STL inclusions. */
#include <array>
#include <iostream>

/* Local inclusions. */
#include "Libs/Math/Space3D/Triangle.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Capsule.hpp"
#include "Libs/Math/Space3D/SAT.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if triangles are colliding.
	 * @note Using SAT.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangleA A reference to a triangle.
	 * @param triangleB A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangleA, const Triangle< precision_t > & triangleB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &triangleA == &triangleB || !triangleA.isValid() || !triangleB.isValid() )
		{
			return false;
		}

		const auto & pA = triangleA.points();
		const auto & pB = triangleB.points();

		std::array< Vector< 3, precision_t >, 3 > verticesA = {pA[0], pA[1], pA[2]};
		std::array< Vector< 3, precision_t >, 3 > verticesB = {pB[0], pB[1], pB[2]};

		Vector< 3, precision_t > mtv;

		return SAT::checkCollision(verticesA, verticesB, mtv);
	}

	/**
	 * @brief Checks if triangles are colliding and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param triangleA A reference to a triangle.
	 * @param triangleB A reference to a triangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangleA, const Triangle< precision_t > & triangleB, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &triangleA == &triangleB || !triangleA.isValid() || !triangleB.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto & pA = triangleA.points();
		const auto & pB = triangleB.points();

		std::array< Vector< 3, precision_t >, 3 > verticesA = {pA[0], pA[1], pA[2]};
		std::array< Vector< 3, precision_t >, 3 > verticesB = {pB[0], pB[1], pB[2]};

		return SAT::checkCollision(verticesA, verticesB, minimumTranslationVector);
	}

	/**
	 * @brief Checks if spheres are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param sphereA A reference to a sphere.
	 * @param sphereB A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphereA, const Sphere< precision_t > & sphereB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &sphereA == &sphereB || !sphereA.isValid() || !sphereB.isValid() )
		{
			return false;
		}

		const auto D = Vector< 3, precision_t >::distanceSquared(sphereA.position(), sphereB.position());
		const auto sumOfRadii = sphereA.radius() + sphereB.radius();
		const auto R2 = sumOfRadii * sumOfRadii;

		return D <= R2;
	}

	/**
	 * @brief Checks if spheres are colliding and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param sphereA A reference to a sphere.
	 * @param sphereB A reference to a sphere.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphereA, const Sphere< precision_t > & sphereB, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &sphereA == &sphereB || !sphereA.isValid() || !sphereB.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Direction from sphereB towards sphereA (to push A out of B). */
		const auto centerToCenter = sphereA.position() - sphereB.position();
		const auto sumOfRadii = sphereA.radius() + sphereB.radius();
		const auto sumOfRadiiSq = sumOfRadii * sumOfRadii;
		const auto distanceSq = centerToCenter.lengthSquared();

		if ( distanceSq <= sumOfRadiiSq )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = sumOfRadii - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: MTV pushes sphereA out of sphereB (consistent with other collision functions). */
				minimumTranslationVector = (centerToCenter / distance) * overlap;
			}
			else
			{
				/* NOTE: The spheres are at the same position. */
				minimumTranslationVector = Vector< 3, precision_t >::negativeY(sumOfRadii);
			}

			return true;
		}

		minimumTranslationVector.reset();

		return false;
	}

	/**
	 * @brief Checks if cuboids are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param cuboidA A reference to a cuboid.
	 * @param cuboidB A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboidA, const AACuboid< precision_t > & cuboidB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &cuboidA == &cuboidB || !cuboidA.isValid() || !cuboidB.isValid() )
		{
			return false;
		}

		const auto & maxA = cuboidA.maximum();
		const auto & minA = cuboidA.minimum();
		const auto & maxB = cuboidB.maximum();
		const auto & minB = cuboidB.minimum();

		return (maxA[X] >= minB[X] && minA[X] <= maxB[X]) && (maxA[Y] >= minB[Y] && minA[Y] <= maxB[Y]) && (maxA[Z] >= minB[Z] && minA[Z] <= maxB[Z]);
	}

	/**
	 * @brief Checks if cuboids are colliding and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param cuboidA A reference to a cuboid.
	 * @param cuboidB A reference to a cuboid.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboidA, const AACuboid< precision_t > & cuboidB, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &cuboidA == &cuboidB || !cuboidA.isValid() || !cuboidB.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto & maxA = cuboidA.maximum();
		const auto & minA = cuboidA.minimum();
		const auto & maxB = cuboidB.maximum();
		const auto & minB = cuboidB.minimum();

		const precision_t overlapX = std::min(maxA[X], maxB[X]) - std::max(minA[X], minB[X]);
		const precision_t overlapY = std::min(maxA[Y], maxB[Y]) - std::max(minA[Y], minB[Y]);
		const precision_t overlapZ = std::min(maxA[Z], maxB[Z]) - std::max(minA[Z], minB[Z]);

		if ( overlapX <= 0 || overlapY <= 0 || overlapZ <= 0 )
		{
			minimumTranslationVector.reset();

			return false;
		}

		if ( overlapX < overlapY && overlapX < overlapZ )
		{
			const precision_t push = (cuboidA.centroid()[X] < cuboidB.centroid()[X]) ? -overlapX : overlapX;

			minimumTranslationVector[X] = push;
			minimumTranslationVector[Y] = 0;
			minimumTranslationVector[Z] = 0;
		}
		else if ( overlapY < overlapZ )
		{
			const precision_t push = (cuboidA.centroid()[Y] < cuboidB.centroid()[Y]) ? -overlapY : overlapY;

			minimumTranslationVector[X] = 0;
			minimumTranslationVector[Y] = push;
			minimumTranslationVector[Z] = 0;
		}
		else
		{
			const precision_t push = (cuboidA.centroid()[Z] < cuboidB.centroid()[Z]) ? -overlapZ : overlapZ;

			minimumTranslationVector[X] = 0;
			minimumTranslationVector[Y] = 0;
			minimumTranslationVector[Z] = push;
		}

		return true;
	}

	/**
	 * @brief Computes the closest points between two line segments.
	 * @note This is a helper function for capsule-capsule collision.
	 * @tparam precision_t The data precision. Default float.
	 * @param segA The first segment.
	 * @param segB The second segment.
	 * @param closestOnA Output: closest point on segment A.
	 * @param closestOnB Output: closest point on segment B.
	 * @return void
	 */
	template< typename precision_t = float >
	void
	closestPointsBetweenSegments (const Segment< precision_t > & segA, const Segment< precision_t > & segB, Point< precision_t > & closestOnA, Point< precision_t > & closestOnB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & p1 = segA.startPoint();
		const auto & q1 = segA.endPoint();
		const auto & p2 = segB.startPoint();
		const auto & q2 = segB.endPoint();

		const auto d1 = q1 - p1;  /* Direction of segment A */
		const auto d2 = q2 - p2;  /* Direction of segment B */
		const auto r = p1 - p2;

		const precision_t a = Vector< 3, precision_t >::dotProduct(d1, d1);  /* Squared length of A */
		const precision_t e = Vector< 3, precision_t >::dotProduct(d2, d2);  /* Squared length of B */
		const precision_t f = Vector< 3, precision_t >::dotProduct(d2, r);

		constexpr precision_t epsilon = std::numeric_limits< precision_t >::epsilon();

		precision_t s, t;

		/* NOTE: Check if both segments are degenerate (points). */
		if ( a <= epsilon && e <= epsilon )
		{
			closestOnA = p1;
			closestOnB = p2;

			return;
		}

		/* NOTE: Check if segment A is degenerate (point). */
		if ( a <= epsilon )
		{
			s = 0;
			t = std::clamp(f / e, static_cast< precision_t >(0), static_cast< precision_t >(1));
		}
		else
		{
			const precision_t c = Vector< 3, precision_t >::dotProduct(d1, r);

			/* NOTE: Check if segment B is degenerate (point). */
			if ( e <= epsilon )
			{
				t = 0;
				s = std::clamp(-c / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
			}
			else
			{
				/* NOTE: General non-degenerate case. */
				const precision_t b = Vector< 3, precision_t >::dotProduct(d1, d2);
				const precision_t denom = a * e - b * b;

				/* NOTE: If segments are not parallel, compute closest points. */
				if ( denom != 0 )
				{
					s = std::clamp((b * f - c * e) / denom, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
				else
				{
					s = 0;
				}

				/* NOTE: Compute point on segment B closest to point on segment A. */
				t = (b * s + f) / e;

				/* NOTE: If t is outside [0,1], clamp and recompute s. */
				if ( t < 0 )
				{
					t = 0;
					s = std::clamp(-c / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
				else if ( t > 1 )
				{
					t = 1;
					s = std::clamp((b - c) / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
			}
		}

		closestOnA = p1 + d1 * s;
		closestOnB = p2 + d2 * t;
	}

	/**
	 * @brief Checks if two capsules are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param capsuleA A reference to the first capsule.
	 * @param capsuleB A reference to the second capsule.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsuleA, const Capsule< precision_t > & capsuleB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &capsuleA == &capsuleB || !capsuleA.isValid() || !capsuleB.isValid() )
		{
			return false;
		}

		/* NOTE: Find the closest points between the two capsule axes. */
		Point< precision_t > closestOnA, closestOnB;
		closestPointsBetweenSegments(capsuleA.axis(), capsuleB.axis(), closestOnA, closestOnB);

		/* NOTE: Check if the distance is within the sum of radii. */
		const auto distanceSq = Vector< 3, precision_t >::distanceSquared(closestOnA, closestOnB);
		const auto sumRadii = capsuleA.radius() + capsuleB.radius();

		return distanceSq <= (sumRadii * sumRadii);
	}

	/**
	 * @brief Checks if two capsules are colliding and gives the minimum translation vector (MTV).
	 * @note The MTV pushes capsuleA out of capsuleB (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param capsuleA A reference to the first capsule.
	 * @param capsuleB A reference to the second capsule.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsuleA, const Capsule< precision_t > & capsuleB, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( &capsuleA == &capsuleB || !capsuleA.isValid() || !capsuleB.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Find the closest points between the two capsule axes. */
		Point< precision_t > closestOnA, closestOnB;
		closestPointsBetweenSegments(capsuleA.axis(), capsuleB.axis(), closestOnA, closestOnB);

		const auto aToB = closestOnB - closestOnA;
		const auto distanceSq = aToB.lengthSquared();
		const auto sumRadii = capsuleA.radius() + capsuleB.radius();

		if ( distanceSq <= (sumRadii * sumRadii) )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = sumRadii - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: MTV points from B towards A, pushing A away from B. */
				minimumTranslationVector = (-aToB / distance) * overlap;
			}
			else
			{
				/* NOTE: The axes are coincident. Push in arbitrary direction. */
				minimumTranslationVector = Vector< 3, precision_t >::negativeY(sumRadii);
			}

			return true;
		}

		minimumTranslationVector.reset();

		return false;
	}
}
