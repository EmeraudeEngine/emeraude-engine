/*
 * src/Libs/Math/Space2D/Collisions/TriangleCircle.hpp
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
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/Point.hpp"
#include "PointTriangle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	namespace details
	{
		template< typename precision_t >
		Vector< 2, precision_t >
		closestPointOnTriangle (const Vector< 2, precision_t > & p, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
		{
			const auto & a = triangle.points()[0];
			const auto & b = triangle.points()[1];
			const auto & c = triangle.points()[2];

			/* Check if p is in region of vertex A. */
			const auto ab = b - a;
			const auto ac = c - a;
			const auto ap = p - a;
			precision_t d1 = Vector< 2, precision_t >::dotProduct(ab, ap);
			precision_t d2 = Vector< 2, precision_t >::dotProduct(ac, ap);
			
			if ( d1 <= 0 && d2 <= 0 )
			{
				return a;
			}

			/* Check if p is in region of vertex B. */
			const auto bp = p - b;
			const auto bc = c - b;
			precision_t d3 = Vector< 2, precision_t >::dotProduct(ab, bp);
			precision_t d4 = Vector< 2, precision_t >::dotProduct(ac, bp);
			
			if ( d3 >= 0 && d4 <= d3 )
			{
				return b;
			}

			/* Check if p is in region of edge AB. */
			precision_t vc = d1 * d4 - d3 * d2;
			
			if ( vc <= 0 && d1 >= 0 && d3 <= 0 )
			{
				precision_t v = d1 / (d1 - d3);

				return a + ab * v;
			}

			/* Check if p is in region of vertex C. */
			const auto cp = p - c;
			precision_t d5 = Vector< 2, precision_t >::dotProduct(ab, cp);
			precision_t d6 = Vector< 2, precision_t >::dotProduct(ac, cp);
			
			if ( d6 >= 0 && d5 <= d6 )
			{
				return c;
			}

			/* Check if p is in region of edge AC. */
			precision_t vb = d5 * d2 - d1 * d6;
			
			if ( vb <= 0 && d2 >= 0 && d6 <= 0 )
			{
				precision_t w = d2 / (d2 - d6);

				return a + ac * w;
			}

			/* Check if p is in region of edge BC. */
			precision_t va = d3 * d6 - d5 * d4;
			
			if ( va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0 )
			{
				precision_t w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

				return b + (c - b) * w;
			}

			/* P is inside face region. */
			return p;
		}
	}

	/**
	 * @brief Checks if a triangle is colliding with a circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param circle A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( isColliding(Point< precision_t >{circle.position()}, triangle) )
		{
			return true;
		}

		Vector< 2, precision_t > closest = details::closestPointOnTriangle(circle.position(), triangle);
		
		return (circle.position() - closest).lengthSquared() <= circle.radius() * circle.radius();
	}

	/**
	 * @brief Checks if a triangle is colliding with a circle and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param circle A reference to a circle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Circle< precision_t > & circle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		Vector< 2, precision_t > closest = details::closestPointOnTriangle(circle.position(), triangle);
		Vector< 2, precision_t > delta = circle.position() - closest;
		precision_t distSq = delta.lengthSquared();

		if ( distSq > circle.radius() * circle.radius() )
		{
			return false;
		}

		if ( isColliding(Point< precision_t >{circle.position()}, triangle) )
		{
			/* Circle center is inside triangle, find MTV to push it out. */
			precision_t min_dist_sq = std::numeric_limits< precision_t >::max();
			
			for ( size_t index = 0; index < 3; ++index )
			{
				const auto & p1 = triangle.points()[index];
				const auto & p2 = triangle.points()[(index + 1) % 3];

				const auto edge_normal = (p2 - p1).perpendicular().normalized();
				const auto dist = Vector< 2, precision_t >::dotProduct(circle.position() - p1, edge_normal);

				if ( dist * dist < min_dist_sq )
				{
					min_dist_sq = dist * dist;
					minimumTranslationVector = edge_normal * (circle.radius() + dist);
				}
			}
		}
		else
		{
			/* Circle center is outside, MTV is from the closest point. */
			precision_t distance = std::sqrt(distSq);
			precision_t overlap = circle.radius() - distance;
			minimumTranslationVector = (delta / distance) * overlap;
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const Circle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, circle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const Circle< precision_t > &, Vector< 2, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const Triangle< precision_t > & triangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const bool result = isColliding(triangle, circle, minimumTranslationVector);

		if ( result )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return result;
	}
}
