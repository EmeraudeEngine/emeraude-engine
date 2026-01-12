/*
 * src/Libs/Math/Space3D/Collisions/CapsulePoint.hpp
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

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a capsule is colliding with a point.
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param point A reference to a point.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() )
		{
			return false;
		}

		/* NOTE: Find the closest point on the capsule axis to the external point. */
		const auto closestOnAxis = capsule.closestPointOnAxis(point);

		/* NOTE: Check if the distance from point to axis is within the radius. */
		const auto distanceSq = Vector< 3, precision_t >::distanceSquared(point, closestOnAxis);

		return distanceSq <= capsule.squaredRadius();
	}

	/**
	 * @brief Checks if a capsule is colliding with a point and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the capsule out of the point (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param point A reference to a point.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Point< precision_t > & point, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Find the closest point on the capsule axis to the external point. */
		const auto closestOnAxis = capsule.closestPointOnAxis(point);

		const auto axisToPoint = point - closestOnAxis;
		const auto distanceSq = axisToPoint.lengthSquared();
		const auto radiusSq = capsule.squaredRadius();

		if ( distanceSq <= radiusSq )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = capsule.radius() - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: MTV points from point towards capsule axis, pushing capsule away from point. */
				minimumTranslationVector = (-axisToPoint / distance) * overlap;
			}
			else
			{
				/* NOTE: The point is exactly on the capsule axis. Push in arbitrary direction. */
				minimumTranslationVector = Vector< 3, precision_t >::negativeY(capsule.radius());
			}

			return true;
		}

		minimumTranslationVector.reset();

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Capsule< precision_t > &, const Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(capsule, point);
	}

	/**
	 * @brief Checks if a point is colliding with a capsule and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the point out of the capsule (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param capsule A reference to a capsule.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Capsule< precision_t > & capsule, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: isColliding(capsule, point, mtv) computes MTV to push capsule out of point.
		 * We need the opposite: push point out of capsule, so we negate the MTV. */
		if ( isColliding(capsule, point, minimumTranslationVector) )
		{
			minimumTranslationVector = -minimumTranslationVector;

			return true;
		}

		return false;
	}
}
