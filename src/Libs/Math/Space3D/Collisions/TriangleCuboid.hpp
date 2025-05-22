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
#include "Libs/Math/Space3D/AACuboid.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a triangle is colliding with a cuboid.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param cuboid A reference to a cuboid.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const AACuboid< precision_t > & cuboid) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() || !cuboid.isValid() )
		{
			return false;
		}

	    const auto & triPoints = triangle.points();
	    const auto cuboidPoints = cuboid.points();

	    /* NOTE: Utility function to project a set of points onto an axis. */
	    auto project = [] (const auto & points, const Vector< 3, precision_t > & axis)
		{
	        precision_t min = Vector< 3, precision_t >::dotProduct(points[0], axis);
	        precision_t max = min;

	        for ( size_t i = 1; i < points.size(); ++i )
	        {
	            precision_t p = Vector< 3, precision_t >::dotProduct(points[i], axis);

	            min = std::min(min, p);
	            max = std::max(max, p);
	        }

	        return std::make_pair(min, max);
	    };

		/* NOTE: Define all axes to be tested (13 in total). */
	    std::array< Vector< 3, precision_t >, 13 > axes;

		/* NOTE: 3 axes of the cuboid normals (X, Y, Z). */
	    axes[0] = Vector< 3, precision_t >::positiveX();
	    axes[1] = Vector< 3, precision_t >::positiveY();
	    axes[2] = Vector< 3, precision_t >::positiveZ();

		/* NOTE: 1 axis of the triangle normal. */
	    const auto & p0 = triPoints[0];
	    const auto & p1 = triPoints[1];
	    const auto & p2 = triPoints[2];
	    axes[3] = Vector< 3, precision_t >::normal(p0, p1, p2);

	    /* NOTE: 9 axes from the vector products between the edges of the two shapes. */
		std::array< Vector< 3, precision_t >, 3 > triEdges{
	    	p1 - p0,
	    	p2 - p1,
	    	p0 - p2
	    };

	    size_t axisIndex = 4;
	    for ( const auto & edge : triEdges )
	    {
	        axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[0]);
	        axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[1]);
	        axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[2]);
	    }

	    /* Test the separation on each axis */
	    for ( const auto & axis : axes )
	    {
	        /* NOTE: We ignore the zero axes that may result from a vector product */
	        if ( axis.isZero() )
	        {
	            continue;
	        }

	        const auto projA = project(triPoints, axis);
	        const auto projB = project(cuboidPoints, axis);

	        /* NOTE: If the projections do not overlap, a separation axis has been found. */
	        if ( projA.second < projB.first || projB.second < projA.first )
	        {
	            return false;
	        }
	    }

	    return true;
	}

	/**
	 * @brief Checks if a triangle is colliding with a cuboid and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param cuboid A reference to a cuboid.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const AACuboid< precision_t > & cuboid, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() || !cuboid.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto & triPoints = triangle.points();
		const auto cuboidPoints = cuboid.points();

		precision_t overlap = std::numeric_limits< precision_t >::max();
		Vector< 3, precision_t > smallestAxis;

		/* NOTE: Utility function to project a set of points onto an axis. */
		auto project = [] (const auto & points, const Vector< 3, precision_t > & axis)
		{
		    precision_t min = Vector< 3, precision_t >::dotProduct(points[0], axis);
		    precision_t max = min;

		    for ( size_t i = 1; i < points.size(); ++i )
		    {
		        precision_t p = Vector< 3, precision_t >::dotProduct(points[i], axis);

		        min = std::min(min, p);
		        max = std::max(max, p);
		    }

		    return std::make_pair(min, max);
		};

		/* NOTE: Define all axes to be tested (13 in total). */
		const auto & p0 = triPoints[0];
		const auto & p1 = triPoints[1];
		const auto & p2 = triPoints[2];

		std::array< Vector< 3, precision_t >, 13 > axes;
		axes[0] = Vector< 3, precision_t >::positiveX();
		axes[1] = Vector< 3, precision_t >::positiveY();
		axes[2] = Vector< 3, precision_t >::positiveZ();
		axes[3] = Vector< 3, precision_t >::normal(p0, p1, p2);

		std::array< Vector< 3, precision_t >, 3 > triEdges{
			p1 - p0,
			p2 - p1,
			p0 - p2
		};

		size_t axisIndex = 4;
		
		for ( const auto & edge : triEdges )
		{
		    axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[0]).normalize();
		    axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[1]).normalize();
		    axes[axisIndex++] = Vector< 3, precision_t >::crossProduct(edge, axes[2]).normalize();
		}

		/* NOTE: Test separation and overlap on each axis. */
		for ( const auto & axis : axes )
		{
		    if ( axis.isZero() )
			{
		    	continue;
			}

			const auto projA = project(triPoints, axis);
		    const auto projB = project(cuboidPoints, axis);

		    if ( projA.second < projB.first || projB.second < projA.first )
		    {
		        minimumTranslationVector.reset();

		    	/* NOTE: Separation axis found, no collision */
		        return false;
		    }

			/* NOTE: Calculate the overlap and update it if it is the smallest. */
		    const precision_t currentOverlap = std::min(projA.second, projB.second) - std::max(projA.first, projB.first);

		    if ( currentOverlap < overlap )
		    {
		        overlap = currentOverlap;
		        smallestAxis = axis;
		    }
		}

		/* NOTE: Collision confirmed, we build the MTV. Make sure the axle is pointing in the right direction for thrust. */
		const auto centerA = (p0 + p1 + p2) / static_cast<precision_t>(3);
		const auto centerB = cuboid.centroid();

		if ( Vector< 3, precision_t >::dotProduct(centerB - centerA, smallestAxis) < 0 )
		{
		    smallestAxis = -smallestAxis;
		}

		minimumTranslationVector = smallestAxis * overlap;

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Triangle< precision_t > &, const AACuboid< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, cuboid);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Triangle< precision_t > &, const AACuboid< precision_t > &, Vector< 3, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AACuboid< precision_t > & cuboid, const Triangle< precision_t > & triangle, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, cuboid, minimumTranslationVector);
	}
}
