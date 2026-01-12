/*
 * src/Libs/Math/Space2D/Collisions/PointTriangle.hpp
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
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a triangle is colliding with a Circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & points = triangle.points();

		/* NOTE: This method checks if the point is on the same side of each edge. The side is determined by the sign of the 2D cross product. */
		const precision_t crossAB = Vector<2, precision_t>::crossProduct(points[1] - points[0], point - points[0]);
		const precision_t crossBC = Vector<2, precision_t>::crossProduct(points[2] - points[1], point - points[1]);
		const precision_t crossCA = Vector<2, precision_t>::crossProduct(points[0] - points[2], point - points[2]);

		/* NOTE: If the point is inside, the three cross products will have the same sign (or will be zero if the point is on an edge).
		 * We check both directions (clockwise and counterclockwise) to be robust. */
		const bool allNonNegative = (crossAB >= 0) && (crossBC >= 0) && (crossCA >= 0);
		const bool allNonPositive = (crossAB <= 0) && (crossBC <= 0) && (crossCA <= 0);

		return allNonNegative || allNonPositive;
	}

	/**
	 * @brief Checks if a triangle is colliding with a Circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param triangle A reference to a triangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Triangle< precision_t > & triangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !isColliding(point, triangle) )
		{
			return false;
		}

		precision_t min_dist_sq = std::numeric_limits< precision_t >::max();

		for ( size_t index = 0; index < 3; ++index )
		{
			const auto & p1 = triangle.points()[index];
			const auto & p2 = triangle.points()[(index + 1) % 3];
			
			const auto edge = p2 - p1;
			const auto point_vec = point - p1;
			
			precision_t t = Vector< 2, precision_t >::dotProduct(point_vec, edge) / edge.lengthSquared();
			t = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));
			
			const auto closest_point = p1 + edge * t;
			const auto dist_sq = (point - closest_point).lengthSquared();

			if ( dist_sq < min_dist_sq )
			{
				min_dist_sq = dist_sq;
				minimumTranslationVector = point - closest_point;
			}
		}
		
		// The MTV should push the point out of the triangle.
		// We need to determine the correct direction.
		// A simple way is to check the vector from the triangle's centroid to the point.
		auto centroid = (triangle.points()[0] + triangle.points()[1] + triangle.points()[2]) / 3.0;
		auto direction = point - centroid;

		if ( Vector< 2, precision_t >::dotProduct(direction, minimumTranslationVector) < 0 )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Point< precision_t > &, const Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Point< precision_t > &, const Triangle< precision_t > &, Vector< 2, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Point< precision_t > & point, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const bool result = isColliding(point, triangle, minimumTranslationVector);

		if ( result )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return result;
	}
}
