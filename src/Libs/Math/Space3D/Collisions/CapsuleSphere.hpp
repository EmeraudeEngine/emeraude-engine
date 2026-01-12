/*
 * src/Libs/Math/Space3D/Collisions/CapsuleSphere.hpp
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
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a capsule is colliding with a sphere.
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param sphere A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !sphere.isValid() )
		{
			return false;
		}

		/* NOTE: Find the closest point on the capsule axis to the sphere center. */
		const auto closestOnAxis = capsule.closestPointOnAxis(sphere.position());

		/* NOTE: Check if the distance is within the sum of radii. */
		const auto distanceSq = Vector< 3, precision_t >::distanceSquared(sphere.position(), closestOnAxis);
		const auto sumRadii = capsule.radius() + sphere.radius();

		return distanceSq <= (sumRadii * sumRadii);
	}

	/**
	 * @brief Checks if a capsule is colliding with a sphere and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the capsule out of the sphere (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param capsule A reference to a capsule.
	 * @param sphere A reference to a sphere.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Capsule< precision_t > & capsule, const Sphere< precision_t > & sphere, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() || !sphere.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Find the closest point on the capsule axis to the sphere center. */
		const auto closestOnAxis = capsule.closestPointOnAxis(sphere.position());

		const auto axisToSphere = sphere.position() - closestOnAxis;
		const auto distanceSq = axisToSphere.lengthSquared();
		const auto sumRadii = capsule.radius() + sphere.radius();

		if ( distanceSq <= (sumRadii * sumRadii) )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = sumRadii - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				/* NOTE: MTV points from sphere towards capsule axis, pushing capsule away from sphere. */
				minimumTranslationVector = (-axisToSphere / distance) * overlap;
			}
			else
			{
				/* NOTE: The sphere center is exactly on the capsule axis. Push in arbitrary direction. */
				minimumTranslationVector = Vector< 3, precision_t >::negativeY(sumRadii);
			}

			return true;
		}

		minimumTranslationVector.reset();

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Capsule< precision_t > &, const Sphere< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(capsule, sphere);
	}

	/**
	 * @brief Checks if a sphere is colliding with a capsule and gives the minimum translation vector (MTV).
	 * @note The MTV pushes the sphere out of the capsule (consistent with convention: MTV pushes first arg out of second).
	 * @tparam precision_t The data precision. Default float.
	 * @param sphere A reference to a sphere.
	 * @param capsule A reference to a capsule.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Capsule< precision_t > & capsule, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* NOTE: isColliding(capsule, sphere, mtv) computes MTV to push capsule out of sphere.
		 * We need the opposite: push sphere out of capsule, so we negate the MTV. */
		if ( isColliding(capsule, sphere, minimumTranslationVector) )
		{
			minimumTranslationVector = -minimumTranslationVector;

			return true;
		}

		return false;
	}
}
