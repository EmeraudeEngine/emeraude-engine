/*
* src/Libs/Math/Space2D/Intersections.hpp
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
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Circle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a segment intersects a circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param circle A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO ...

		return false;
	}

	/**
	 * @brief Checks if a segment intersects a circle and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param circle A reference to a circle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Circle< precision_t > & circle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const Point< precision_t > segmentStart = segment.start();
		const Point< precision_t > segmentDirection = segment.end() - segmentStart;
		const Point< precision_t > centerToStart = segmentStart - circle.position();

		const precision_t a = segmentDirection.lengthSquared();
		const precision_t b = 2 * Vector<2, precision_t>::dotProduct(segmentDirection, centerToStart);
		const precision_t c = centerToStart.lengthSquared() - (circle.radius() * circle.radius());

		if ( std::abs(a) < std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: The segment is a point, it cannot intersect a circumference. */
			return false;
		}

		const precision_t discriminant = b * b - 4 * a * c;

		if ( discriminant < 0 )
		{
			/* NOTE: No real solution, the line never intersects the circle. */
			return false;
		}

		const precision_t sqrtDiscriminant = std::sqrt(discriminant);
		const precision_t inv2a = static_cast< precision_t >(1) / (2 * a);

		/* NOTE: The two potential intersection points of the infinite line with the circle. */
		const precision_t t1 = (-b - sqrtDiscriminant) * inv2a;
		const precision_t t2 = (-b + sqrtDiscriminant) * inv2a;

		precision_t t_final = std::numeric_limits< precision_t >::max();
		bool intersectionFound = false;

		/* NOTE: We are looking for the smallest valid value of 't' (between 0 and 1). */
		if ( t1 >= 0 && t1 <= 1 )
		{
			t_final = t1;
			intersectionFound = true;
		}

		/* NOTE: t2 is necessarily greater than t1. If t1 is valid, t2 cannot be a better solution.
		 * We only check t2 if t1 is not in the interval [0,1]. */
		if ( t2 >= 0 && t2 <= 1 && t2 < t_final )
		{
			t_final = t2;
			intersectionFound = true;
		}

		if ( intersectionFound )
		{
			intersection = segmentStart + segmentDirection * t_final;

			return true;
		}

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, Circle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Circle< precision_t > & circle, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, circle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, Circle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Circle< precision_t > & circle, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, circle, intersection);
	}
}
