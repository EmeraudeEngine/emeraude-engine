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
#include "Libs/Math/Space2D/Line.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a line is intersecting a circle and gives the intersection points.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param circle A reference to a circle.
	 * @param intersection1 A writable reference for the first intersection point.
	 * @param intersection2 A writable reference for the second intersection point.
	 * @return int The number of intersection points (0, 1, or 2).
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	int
	isIntersecting (const Line< precision_t > & line, const Circle< precision_t > & circle, Point< precision_t > & intersection1, Point< precision_t > & intersection2) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto d = line.direction();
		const auto f = line.origin() - circle.position();

		const precision_t a = Vector< 2, precision_t >::dotProduct(d, d);
		const precision_t b = 2 * Vector< 2, precision_t >::dotProduct(f, d);
		const precision_t c = Vector< 2, precision_t >::dotProduct(f, f) - circle.radius() * circle.radius();

		precision_t discriminant = b * b - 4 * a * c;

		if ( discriminant < 0 )
		{
			/* No intersection. */
			return 0;
		}

		discriminant = std::sqrt(discriminant);

		const precision_t t1 = (-b - discriminant) / (2 * a);
		intersection1 = line.origin() + d * t1;

		if ( discriminant < std::numeric_limits<precision_t>::epsilon() )
		{
			/* One intersection (tangent). */
			return 1;
		}

		const precision_t t2 = (-b + discriminant) / (2 * a);
		intersection2 = line.origin() + d * t2;

		/* Two intersections */
		return 2;
	}

	/**
	 * @brief Checks if a line is intersecting a circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param circle A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Line< precision_t > & line, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		Point< precision_t > dummy1, dummy2;

		return isIntersecting(line, circle, dummy1, dummy2) > 0;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, Circle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Circle< precision_t > & circle, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, circle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, Circle< precision_t > &, Point< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	int
	isIntersecting (const Circle< precision_t > & circle, const Line< precision_t > & line, Point< precision_t > & intersection1, Point< precision_t > & intersection2) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, circle, intersection1, intersection2);
	}
}
