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
#include "Libs/Math/Space3D/Segment.hpp"
#include "Libs/Math/Space3D/Triangle.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a segment intersects a triangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !triangle.isValid() )
		{
			return false;
		}

		const auto & triP0 = triangle.pointA();
		const auto & triP1 = triangle.pointB();
		const auto & triP2 = triangle.pointC();

		const auto & segStart = segment.startPoint();
		const auto & segEnd = segment.endPoint();
		const auto segDir = segEnd - segStart;

		/* NOTE: Triangle-edge vectors. */
		const auto edge1 = triP1 - triP0;
		const auto edge2 = triP2 - triP0;

		/* NOTE: Calculate the determinant. If close to zero, the segment is parallel to the plane of the triangle. */
		const auto pVec = Vector< 3, precision_t >::crossProduct(segDir, edge2);
		const auto det = Vector< 3, precision_t >::dotProduct(edge1, pVec);

		/* NOTE: A tolerance (epsilon) is used for comparison with zero. */
		if ( std::abs(det) < std::numeric_limits< precision_t >::epsilon() )
		{
			return false;
		}

		const auto invDet = static_cast<precision_t>(1.0) / det;
		const auto tVec = segStart - triP0;

		/* NOTE: Calculates the barycentric coordinate 'u' and checks its bounds. */
		const auto u = Vector< 3, precision_t >::dotProduct(tVec, pVec) * invDet;

		if ( u < 0 || u > 1 )
		{
			return false;
		}

		/* NOTE: Calculates the barycentric coordinate 'v' and checks its bounds. */
		const auto qVec = Vector< 3, precision_t >::crossProduct(tVec, edge1);
		const auto v = Vector< 3, precision_t >::dotProduct(segDir, qVec) * invDet;

		if ( v < 0 || u + v > 1 )
		{
			return false;
		}

		/* NOTE: Calculates 't', the position of the intersection along the segment. */
		const auto t = Vector< 3, precision_t >::dotProduct(edge2, qVec) * invDet;

		/* NOTE: An intersection exists if 't' is between 0 and 1 (inclusive). */
		if ( t >= 0 && t <= 1)
		{
			return true;
		}

		return false;
	}

	/**
	 * @brief Checks if a segment intersects a triangle and gives the intersection point.
	 * @note The intersection is valid for both faces of the triangle (no backface culling).
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a 3D segment.
	 * @param triangle A reference to a triangle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Triangle< precision_t > & triangle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		/* 1. Define the vectors for the segment and edges of the triangle. */
		const Point< precision_t > segmentDirection = segment.endPoint() - segment.startPoint();
		const Point< precision_t > edge1 = triangle.pointB() - triangle.pointA();
		const Point< precision_t > edge2 = triangle.pointC() - triangle.pointA();

		/* 2. Calculate the determinant. If close to zero, the segment is parallel to the plane of the triangle. */
		const Point< precision_t > h = Point< precision_t >::crossProduct(segmentDirection, edge2);
		const auto det = Point< precision_t >::dotProduct(edge1, h);

		if ( det > -std::numeric_limits< precision_t >::epsilon() && det < std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: Segment parallel to the triangle. */
			return false;
		}

		const auto invDet = static_cast< precision_t >(1) / det;

		/* 3. Calculate the barycentric coordinate 'u' */
		const Point< precision_t > s = segment.startPoint() - triangle.pointA();
		const precision_t u = invDet * Point< precision_t >::dotProduct(s, h);

		if ( u < static_cast< precision_t >(0) || u > static_cast< precision_t >(1) )
		{
			/* NOTE: The point of intersection is outside the triangle. */
			return false;
		}

		/* 4. Calculate the barycentric coordinate 'v' */
		const Point< precision_t > q = Point< precision_t >::crossProduct(s, edge1);
		const precision_t v = invDet * Point< precision_t >::dotProduct(segmentDirection, q);

		if ( v < static_cast< precision_t >(0) || u + v > static_cast< precision_t >(1) )
		{
			/* NOTE: The point of intersection is outside the triangle. */
			return false;
		}

		/* 5. Calculate 't', the distance along the segment where the intersection occurs. */
		const precision_t t = invDet * Point< precision_t >::dotProduct(edge2, q);

		/* NOTE: We check that the intersection is located ON the segment (t between 0 and 1) */
		if ( t >= static_cast< precision_t >(0) && t <= static_cast< precision_t >(1) )
		{
			/* NOTE: The intersection is valid and lies on the segment. */
			intersection = segment.startPoint() + segmentDirection * t;

			return true;
		}

		/* NOTE: The intersection is outside the segment. */
		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, Triangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, triangle, intersection);
	}
}
