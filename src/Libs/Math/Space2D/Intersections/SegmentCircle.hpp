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
#include "Libs/Math/Space2D/Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
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
	bool
	isIntersecting (const Segment< precision_t > & segment, const Circle< precision_t > & circle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto d = segment.endPoint() - segment.startPoint();
		const auto f = segment.startPoint() - circle.position();

		const precision_t a = Vector< 2, precision_t >::dotProduct(d, d);
		const precision_t b = 2 * Vector< 2, precision_t >::dotProduct(f, d);
		const precision_t c = Vector< 2, precision_t >::dotProduct(f, f) - circle.radius() * circle.radius();

		precision_t discriminant = b * b - 4 * a * c;

		if ( discriminant < 0 )
		{
			/* No intersection with circle boundary. Check if segment is completely inside. */
			const precision_t distStartSq = (segment.startPoint() - circle.position()).lengthSquared();
			const precision_t distEndSq = (segment.endPoint() - circle.position()).lengthSquared();
			const precision_t radiusSq = circle.radius() * circle.radius();

			/* If both endpoints are inside the circle, the segment is inside and "intersects" */
			if ( distStartSq <= radiusSq && distEndSq <= radiusSq )
			{
				/* Set intersection to the start point of the segment */
				intersection = segment.startPoint();
				return true;
			}

			return false;
		}

		discriminant = std::sqrt(discriminant);
		const precision_t t1 = (-b - discriminant) / (2 * a);
		const precision_t t2 = (-b + discriminant) / (2 * a);

		if ( t1 >= 0 && t1 <= 1 )
		{
			intersection = segment.startPoint() + d * t1;

			return true;
		}

		if ( t2 >= 0 && t2 <= 1 )
		{
			intersection = segment.startPoint() + d * t2;

			return true;
		}

		/* No intersection on segment boundaries. Check if segment is completely inside circle. */
		const precision_t distStartSq = (segment.startPoint() - circle.position()).lengthSquared();
		const precision_t radiusSq = circle.radius() * circle.radius();

		if ( distStartSq <= radiusSq )
		{
			/* Start point is inside, so the whole segment must be inside */
			intersection = segment.startPoint();
			return true;
		}

		return false;
	}

	/**
	 * @brief Checks if a segment intersects a circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param circle A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Segment< precision_t > & segment, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		Point< precision_t > dummy;

		return isIntersecting(segment, circle, dummy);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, const Circle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Circle< precision_t > & circle, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, circle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, const Circle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Circle< precision_t > & circle, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, circle, intersection);
	}
}
