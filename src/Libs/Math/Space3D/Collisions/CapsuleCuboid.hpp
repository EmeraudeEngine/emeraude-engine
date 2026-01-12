/*
 * src/Libs/Math/Space3D/Collisions/CapsuleCuboid.hpp
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
#include "Libs/Math/Space3D/Capsule.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Helper to clamp a point to the nearest point on/in an AABB.
	 * @tparam precision_t The data precision.
	 * @param point The point to clamp.
	 * @param cuboid The AABB.
	 * @return Point< precision_t > The clamped point.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	Point< precision_t >
	clampPointToCuboid (const Point< precision_t > & point, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & min = cuboid.minimum();
		const auto & max = cuboid.maximum();

		return Point< precision_t >{
			std::max(min[X], std::min(point[X], max[X])),
			std::max(min[Y], std::min(point[Y], max[Y])),
			std::max(min[Z], std::min(point[Z], max[Z]))
		};
	}

	/**
	 * @brief Finds the closest point on a segment to an AABB, and the closest point on the AABB to that segment point.
	 * @note This uses an iterative refinement approach for accuracy.
	 * @tparam precision_t The data precision.
	 * @param capsule The capsule.
	 * @param cuboid The AABB.
	 * @param closestOnAxis Output: closest point on capsule axis.
	 * @param closestOnCuboid Output: closest point on/in cuboid.
	 * @return void
	 */
	template< typename precision_t = float >
	void
	closestPointsCapsuleCuboid (const Capsule< precision_t > & capsule, const AACuboid< precision_t > & cuboid, Point< precision_t > & closestOnAxis, Point< precision_t > & closestOnCuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: Start with the centroid of the capsule axis. */
		closestOnAxis = capsule.centroid();
		closestOnCuboid = clampPointToCuboid(closestOnAxis, cuboid);

		/* NOTE: Iterative refinement: alternate between finding closest point on axis and closest point on cuboid.
		 * This converges quickly (usually 2-3 iterations). */
		for ( int iteration = 0; iteration < 4; ++iteration )
		{
			/* Find closest point on capsule axis to the current closest point on cuboid. */
			closestOnAxis = capsule.closestPointOnAxis(closestOnCuboid);

			/* Find closest point on cuboid to the current closest point on axis. */
			closestOnCuboid = clampPointToCuboid(closestOnAxis, cuboid);
		}
	}

	/**
	 * @brief Checks if a capsule is colliding with an axis-aligned cuboid.
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !cuboid.isValid() )
		{
			return false;
		}

		/* NOTE: Find the closest points between capsule axis and cuboid. */
		Point< precision_t > closestOnAxis, closestOnCuboid;
		closestPointsCapsuleCuboid(capsule, cuboid, closestOnAxis, closestOnCuboid);

		/* NOTE: Check if the distance is within the capsule radius. */
		const auto distanceSq = Vector< 3, precision_t >::distanceSquared(closestOnAxis, closestOnCuboid);

		return distanceSq <= capsule.squaredRadius();
	}

	/**
	 * @brief Checks if a capsule is colliding with an axis-aligned cuboid and gives the MTV.
	 * @note The MTV pushes the capsule out of the cuboid (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param cuboid A reference to a cuboid.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const AACuboid< precision_t > & cuboid, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !cuboid.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Find the closest points between capsule axis and cuboid. */
		Point< precision_t > closestOnAxis, closestOnCuboid;
		closestPointsCapsuleCuboid(capsule, cuboid, closestOnAxis, closestOnCuboid);

		const auto axisToCuboid = closestOnCuboid - closestOnAxis;
		const auto distanceSq = axisToCuboid.lengthSquared();
		const auto radiusSq = capsule.squaredRadius();

		if ( distanceSq > radiusSq )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Collision detected. Compute MTV. */
		const auto distance = std::sqrt(distanceSq);

		/* NOTE: Check if the capsule axis is outside the cuboid. */
		if ( distance > std::numeric_limits< precision_t >::epsilon() )
		{
			const auto overlap = capsule.radius() - distance;

			/* NOTE: MTV points from cuboid towards capsule axis, pushing capsule away from cuboid. */
			minimumTranslationVector = (-axisToCuboid / distance) * overlap;
		}
		/* NOTE: The closest point on the axis is inside or on the surface of the cuboid. */
		else
		{
			const auto & min = cuboid.minimum();
			const auto & max = cuboid.maximum();

			/* NOTE: Find the shortest distance to push out to a face. */
			const std::array< precision_t, 6 > overlaps = {
				(max[X] - closestOnAxis[X]) + capsule.radius(),
				(closestOnAxis[X] - min[X]) + capsule.radius(),
				(max[Y] - closestOnAxis[Y]) + capsule.radius(),
				(closestOnAxis[Y] - min[Y]) + capsule.radius(),
				(max[Z] - closestOnAxis[Z]) + capsule.radius(),
				(closestOnAxis[Z] - min[Z]) + capsule.radius()
			};

			precision_t minOverlap = overlaps[0];
			int minIndex = 0;

			for ( int i = 1; i < 6; ++i )
			{
				if ( overlaps[i] < minOverlap )
				{
					minOverlap = overlaps[i];
					minIndex = i;
				}
			}

			switch ( minIndex )
			{
				case 0:
					minimumTranslationVector = Vector< 3, precision_t >::positiveX(minOverlap);
					break;

				case 1:
					minimumTranslationVector = Vector< 3, precision_t >::negativeX(minOverlap);
					break;

				case 2:
					minimumTranslationVector = Vector< 3, precision_t >::positiveY(minOverlap);
					break;

				case 3:
					minimumTranslationVector = Vector< 3, precision_t >::negativeY(minOverlap);
					break;

				case 4:
					minimumTranslationVector = Vector< 3, precision_t >::positiveZ(minOverlap);
					break;

				case 5:
					minimumTranslationVector = Vector< 3, precision_t >::negativeZ(minOverlap);
					break;

				default:
					break;
			}
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Capsule< precision_t > &, const AACuboid< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(capsule, cuboid);
	}

	/**
	 * @brief Checks if a cuboid is colliding with a capsule and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the cuboid out of the capsule (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param cuboid A reference to a cuboid.
	 * @param capsule A reference to a capsule.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Capsule< precision_t > & capsule, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: isColliding(capsule, cuboid, mtv) computes MTV to push capsule out of cuboid.
		 * We need the opposite: push cuboid out of capsule, so we negate the MTV. */
		if ( isColliding(capsule, cuboid, minimumTranslationVector) )
		{
			minimumTranslationVector = -minimumTranslationVector;

			return true;
		}

		return false;
	}
}
