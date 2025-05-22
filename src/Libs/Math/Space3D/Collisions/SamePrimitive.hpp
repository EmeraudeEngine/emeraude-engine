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

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/Math/Space3D/Triangle.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"

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

		/* NOTE: Gather the axes to be tested (the normals of each side of the two triangles). */
		std::array< Vector< 2, precision_t >, 6 > axes = {
			(pA[1] - pA[0]).perpendicular(),
			(pA[2] - pA[1]).perpendicular(),
			(pA[0] - pA[2]).perpendicular(),
			(pB[1] - pB[0]).perpendicular(),
			(pB[2] - pB[1]).perpendicular(),
			(pB[0] - pB[2]).perpendicular()
		};

		/* NOTE: Utility function to project a triangle onto an axis */
		auto project = [] (const std::array< Point< precision_t >, 3 > & points, const Vector< 2, precision_t > & axis)
		{
			precision_t min = Vector< 2, precision_t >::dotProduct(points[0], axis);
			precision_t max = min;

			for ( size_t i = 1; i < 3; ++i )
			{
				precision_t p = Vector< 2, precision_t >::dotProduct(points[i], axis);

				min = std::min(min, p);
				max = std::max(max, p);
			}

			return std::make_pair(min, max);
		};

		for ( const auto & axis : axes )
		{
			const auto projectionA = project(pA, axis);
			const auto projectionB = project(pB, axis);

			/* NOTE: Check if there is a "gap" between the projections.
			 * If there is one, we have found an axis of separation, so no collision. */
			if ( projectionA.second < projectionB.first || projectionB.second < projectionA.first )
			{
				return false; // Pas de collision
			}
		}

		/* NOTE: If no separating axis was found, the triangles are in collision. */
		return true;
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

	    precision_t overlap = std::numeric_limits< precision_t >::max();
	    Vector< 2, precision_t > smallestAxis;

	    std::array< Vector< 2, precision_t >, 6 > axes = {
	        (pA[1] - pA[0]).perpendicular(),
	        (pA[2] - pA[1]).perpendicular(),
	        (pA[0] - pA[2]).perpendicular(),
	        (pB[1] - pB[0]).perpendicular(),
	        (pB[2] - pB[1]).perpendicular(),
	        (pB[0] - pB[2]).perpendicular()
	    };

	    auto project = [] (const std::array< Point< precision_t >, 3 > & points, const Vector< 2, precision_t > & axis)
		{
	        precision_t min = Vector< 2, precision_t >::dotProduct(points[0], axis);
	        precision_t max = min;

	        for ( size_t i = 1; i < 3; ++i )
	        {
	            precision_t p = Vector< 2, precision_t >::dotProduct(points[i], axis);

	            min = std::min(min, p);
	            max = std::max(max, p);
	        }

	        return std::make_pair(min, max);
	    };

	    for ( const auto & axis : axes )
	    {
	        const auto projectionA = project(pA, axis);
	        const auto projectionB = project(pB, axis);

	        if ( projectionA.second < projectionB.first || projectionB.second < projectionA.first )
	        {
	            minimumTranslationVector.reset();

	            return false;
	        }

	        const precision_t currentOverlap = std::min(projectionA.second, projectionB.second) - std::max(projectionA.first, projectionB.first);

	        if ( currentOverlap < overlap )
	        {
	            overlap = currentOverlap;
	            smallestAxis = axis;
	        }
	    }

		/* NOTE: Collision confirmed, we're building the MTV.
	     * Ensure the thrust vector is pointing in the correct direction. */
	    const Vector< 2, precision_t > centerA = (pA[0] + pA[1] + pA[2]) / 3.0f;
	    const Vector< 2, precision_t > centerB = (pB[0] + pB[1] + pB[2]) / 3.0f;
	    const auto direction = centerB - centerA;

	    if ( Vector< 2, precision_t >::dotProduct(direction, smallestAxis) < 0 )
	    {
	    	/* NOTE: Reverse the axis if necessary. */
	        smallestAxis = -smallestAxis;
	    }

	    /* NOTE: The MTV is the axis of smallest overlap multiplied by this value. The 2D result is converted to 3D for output. */
	    minimumTranslationVector = (smallestAxis * overlap).toVector3();

	    return true;
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

		std::cout << "Sphere A : " << sphereA << "\nSphereB : " << sphereB << "\n";

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

		const auto centerToCenter = sphereB.position() - sphereA.position();
		const auto sumOfRadii = sphereA.radius() + sphereB.radius();
		const auto sumOfRadiiSq = sumOfRadii * sumOfRadii;
		const auto distanceSq = centerToCenter.lengthSquared();

		if ( distanceSq <= sumOfRadiiSq )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = sumOfRadii - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				minimumTranslationVector = (centerToCenter / distance) * overlap;
			}
			else
			{
				/* NOTE: The point is at the center of the sphere. */
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

			minimumTranslationVector.set(push, 0, 0);
		}
		else if ( overlapY < overlapZ )
		{
			const precision_t push = (cuboidA.centroid()[Y] < cuboidB.centroid()[Y]) ? -overlapY : overlapY;

			minimumTranslationVector.set(0, push, 0);
		}
		else
		{
			const precision_t push = (cuboidA.centroid()[Z] < cuboidB.centroid()[Z]) ? -overlapZ : overlapZ;

			minimumTranslationVector.set(0, 0, push);
		}

		return true;
	}
}
