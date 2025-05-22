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
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a point is colliding with a sphere.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param sphere A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() )
		{
			return false;
		}

		const auto D = Vector< 3, precision_t >::distanceSquared(sphere.position(), point);
		const auto R2 = sphere.squaredRadius();

		return D <= R2;
	}

	/**
	 * @brief Checks if a point is colliding with a sphere and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param sphere A reference to a sphere.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Sphere< precision_t > & sphere, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto centerToPoint = point - sphere.position();
		const auto distanceSq = centerToPoint.lengthSquared();
		const auto radiusSq = sphere.squaredRadius();

		if ( distanceSq <= radiusSq )
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = sphere.radius() - distance;

			if ( distance > std::numeric_limits< precision_t >::epsilon() )
			{
				minimumTranslationVector = (centerToPoint / distance) * overlap;
			}
			else
			{
				/* NOTE: The point is at the center of the sphere. */
				minimumTranslationVector = Vector< 3, precision_t >::negativeY(sphere.radius());
			}

			return true;
		}

		minimumTranslationVector.reset();

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const Sphere< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, sphere);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const Sphere< precision_t > &, Vector< 3, precision_t >) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const Point< precision_t > & point, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, sphere, minimumTranslationVector);
	}
}
