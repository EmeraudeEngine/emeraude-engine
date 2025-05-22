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
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a point is colliding with a cuboid.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !cuboid.isValid() )
		{
			return false;
		}

		const auto & maximum = cuboid.maximum();
		const auto & minimum = cuboid.minimum();

		if ( point[X] < minimum[X] || point[X] > maximum[X] )
		{
			return false;
		}

		if ( point[Y] < minimum[Y] || point[Y] > maximum[Y] )
		{
			return false;
		}

		if ( point[Z] < minimum[Z] || point[Z] > maximum[Z] )
		{
			return false;
		}

		return true;
	}

	/**
	 * @brief Checks if a point is colliding with a cuboid and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param cuboid A reference to a cuboid.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const AACuboid< precision_t > & cuboid, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !cuboid.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto & max = cuboid.maximum();
		const auto & min = cuboid.minimum();

		/* NOTE: Check if the point is outside the cuboid. */
		if ( point[X] < min[X] || point[X] > max[X] || point[Y] < min[Y] || point[Y] > max[Y] || point[Z] < min[Z] || point[Z] > max[Z])
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: Collision confirmed, we calculate the MTV. We calculate the distances to the 6 faces of the cuboid. */
		const precision_t distToPosX = max[X] - point[X];
		const precision_t distToNegX = point[X] - min[X];
		const precision_t distToPosY = max[Y] - point[Y];
		const precision_t distToNegY = point[Y] - min[Y];
		const precision_t distToPosZ = max[Z] - point[Z];
		const precision_t distToNegZ = point[Z] - min[Z];

		/* NOTE: The smallest of these 6 distances is found to determine the minimum thrust. */
		precision_t minOverlap = distToPosX;
		minimumTranslationVector = Vector< 3, precision_t >::positiveX(minOverlap);

		if ( distToNegX < minOverlap )
		{
			minOverlap = distToNegX;
			minimumTranslationVector = Vector< 3, precision_t >::negativeX(minOverlap);
		}
		
		if ( distToPosY < minOverlap )
		{
			minOverlap = distToPosY;
			minimumTranslationVector = Vector< 3, precision_t >::positiveY(minOverlap);
		}
		
		if ( distToNegY < minOverlap )
		{
			minOverlap = distToNegY;
			minimumTranslationVector = Vector< 3, precision_t >::negativeY(minOverlap);
		}
		
		if ( distToPosZ < minOverlap )
		{
			minOverlap = distToPosZ;
			minimumTranslationVector = Vector< 3, precision_t >::positiveZ(minOverlap);
		}
		
		if ( distToNegZ < minOverlap )
		{
			minOverlap = distToNegZ;
			minimumTranslationVector = Vector< 3, precision_t >::negativeZ(minOverlap);
		}
    
		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const AACuboid< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, cuboid);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const AACuboid< precision_t > &, Vector< 3, precision_t >) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Point< precision_t > & point, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, cuboid);
	}
}
