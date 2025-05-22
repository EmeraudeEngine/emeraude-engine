/*
* src/Libs/Math/Space3D/Intersections.hpp
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
#include "Libs/Math/Space3D/Line.hpp"
#include "Libs/Math/Space3D/Triangle.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a line is intersecting a triangle.
	 * @todo Same as segment-triangle except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() )
		{
			return false;
		}

		const auto & origin = line.origin();
		const auto & direction = line.direction();
		const auto & p0 = triangle.pointA();
		const auto & p1 = triangle.pointB();
		const auto & p2 = triangle.pointC();

		const auto edge1 = p1 - p0;
		const auto edge2 = p2 - p0;

		/* NOTE: Calculate the determinant. If close to zero, the line is parallel to the plane of the triangle. */
		const auto pVec = Vector< 3, precision_t >::crossProduct(direction, edge2);
		const auto det = Vector< 3, precision_t >::dotProduct(edge1, pVec);

		if ( std::abs(det) < std::numeric_limits< precision_t >::epsilon() )
		{
			return false;
		}

		const auto invDet = static_cast< precision_t >(1.0) / det;
		const auto tVec = origin - p0;

		/* NOTE: Calculate the barycentric coordinate 'u' and check its bounds. */
		const auto u = Vector< 3, precision_t >::dotProduct(tVec, pVec) * invDet;

		if ( u < 0 || u > 1 )
		{
			return false;
		}

		/* NOTE: Calculate the barycentric coordinate 'v' and check its bounds. */
		const auto qVec = Vector< 3, precision_t >::crossProduct(tVec, edge1);
		const auto v = Vector< 3, precision_t >::dotProduct(direction, qVec) * invDet;

		if ( v < 0 || u + v > 1 )
		{
			return false;
		}

		/* NOTE: If the barycentric coordinates are valid, the infinite line intersects the triangle. */
		return true;
	}

	/**
	 * @brief Checks if a line is intersecting a triangle and gives the intersection point.
	 * @todo Same as segment-triangle except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param triangle A reference to a triangle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Triangle< precision_t > & triangle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & origin = line.origin();
		const auto & direction = line.direction();
		const auto & p0 = triangle.pointA();
		const auto & p1 = triangle.pointB();
		const auto & p2 = triangle.pointC();

		const auto edge1 = p1 - p0;
		const auto edge2 = p2 - p0;

		/* NOTE: Calculate the determinant. If close to zero, the line is parallel to the plane of the triangle. */
		const auto pVec = Vector< 3, precision_t >::crossProduct(direction, edge2);
		const auto det = Vector< 3, precision_t >::dotProduct(edge1, pVec);
    
		if ( std::abs(det) < std::numeric_limits< precision_t >::epsilon() )
		{
			intersection.reset();

			return false;
		}

		const auto invDet = static_cast< precision_t >(1.0) / det;
		const auto tVec = origin - p0;

		/* NOTE: Calculate the barycentric coordinate 'u' and check its bounds */
		const auto u = Vector< 3, precision_t >::dotProduct(tVec, pVec) * invDet;

		if ( u < 0 || u > 1 )
		{
			intersection.reset();

			return false;
		}

		/* NOTE: Calculate the barycentric coordinate 'v' and check its bounds. */
		const auto qVec = Vector< 3, precision_t >::crossProduct(tVec, edge1);
		const auto v = Vector< 3, precision_t >::dotProduct(direction, qVec) * invDet;

		if ( v < 0 || u + v > 1 )
		{
			intersection.reset();

			return false;
		}

		/* NOTE: An intersection exists. We calculate 't' to find the point. */
		const auto t = Vector< 3, precision_t >::dotProduct(edge2, qVec) * invDet;
    
		/* NOTE: Calculate the coordinates of the intersection point. */
		intersection = origin + direction * t;
    
		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, Triangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Line< precision_t > & line, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, triangle, intersection);
	}
}
