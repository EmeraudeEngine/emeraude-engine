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
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a sphere is colliding with a cuboid.
	 * @tparam precision_t The data precision. Default float.
	 * @param sphere A reference to a sphere.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() || !cuboid.isValid() )
		{
			return false;
		}

		const auto & spherePosition = sphere.position();
		const auto & minimum = cuboid.minimum();
		const auto & maximum = cuboid.maximum();

		const Point< precision_t > closestPoint{
			std::max(minimum[X], std::min(spherePosition[X], maximum[X])),
			std::max(minimum[Y], std::min(spherePosition[Y], maximum[Y])),
			std::max(minimum[Z], std::min(spherePosition[Z], maximum[Z]))
		};

		const Point< precision_t > delta = sphere.position() - closestPoint;

		return delta.lengthSquared() <= sphere.radius() * sphere.radius();
	}

	/**
	 * @brief Checks if a sphere is colliding with a cuboid and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param sphere A reference to a sphere.
	 * @param cuboid A reference to a cuboid.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Sphere< precision_t > & sphere, const AACuboid< precision_t > & cuboid, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() || !cuboid.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

	    const auto & spherePos = sphere.position();
	    const auto & min = cuboid.minimum();
	    const auto & max = cuboid.maximum();

	    /* NOTE: Find the point on the cuboid closest to the center of the sphere. */
	    const Point< precision_t > closestPoint{
	        std::max(min[X], std::min(spherePos[X], max[X])),
	        std::max(min[Y], std::min(spherePos[Y], max[Y])),
	        std::max(min[Z], std::min(spherePos[Z], max[Z]))
	    };

	    /* NOTE: Check collision using distance squared. */
	    const auto delta = spherePos - closestPoint;
	    const auto distanceSq = delta.lengthSquared();

	    if ( distanceSq > sphere.squaredRadius() )
	    {
	        minimumTranslationVector.reset();

	        return false;
	    }

		/* NOTE: The center of the sphere is outside the cuboid. */
	    if ( distanceSq > std::numeric_limits<precision_t>::epsilon() )
	    {
	        const auto distance = std::sqrt(distanceSq);
	        const auto overlap = sphere.radius() - distance;
	        minimumTranslationVector = (delta / distance) * overlap;
	    }
	    /* NOTE: The center of the sphere is inside the cuboid */
	    else
	    {
	        /* NOTE: We need to find the closest face to "push" the sphere out. */
	    	std::array< precision_t, 6 > overlaps = {
	            (max[X] - spherePos[X]) + sphere.radius(),
	            (spherePos[X] - min[X]) + sphere.radius(),
	            (max[Y] - spherePos[Y]) + sphere.radius(),
	            (spherePos[Y] - min[Y]) + sphere.radius(),
	            (max[Z] - spherePos[Z]) + sphere.radius(),
	            (spherePos[Z] - min[Z]) + sphere.radius()
	        };

	        /* NOTE: We are looking for the smallest push. */
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

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Sphere< precision_t > &, const AACuboid< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(sphere, cuboid);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Sphere< precision_t > &, const AACuboid< precision_t > &, Vector< 3, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Sphere< precision_t > & sphere, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(sphere, cuboid, minimumTranslationVector);
	}
}
