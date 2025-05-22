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

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if lines are intersecting.
	 * @tparam precision_t The data precision. Default float.
	 * @param lineA A reference to a line.
	 * @param lineB A reference to a line.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & lineA, const Line< precision_t > & lineB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO...

		return false;
	}

	/**
	 * @brief Checks if lines are intersecting and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param lineA A reference to a line.
	 * @param lineB A reference to a line.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & lineA, const Line< precision_t > & lineB, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO...

		return false;
	}

	/**
	 * @brief Finds the intersection between two lines using the "Cramer" method.
	 * @tparam precision_t The data precision. Default float.
	 * @param ax
	 * @param ay
	 * @param bx
	 * @param by
	 * @param cx
	 * @param cy
	 * @param dx
	 * @param dy
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (precision_t ax, precision_t ay, precision_t bx, precision_t by, precision_t cx, precision_t cy, precision_t dx, precision_t dy, Point< precision_t > & intersection) noexcept requires (std::is_arithmetic_v< precision_t >)
	{
		/* NOTE: Direction vectors of the lines */
		const precision_t dir_ab_x = ax - bx;
		const precision_t dir_ab_y = ay - by;
		const precision_t dir_cd_x = cx - dx;
		const precision_t dir_cd_y = cy - dy;

		/* NOTE: Determinant of the system. If zero, the lines are parallel. */
		const precision_t det = (dir_ab_x * dir_cd_y) - (dir_ab_y * dir_cd_x);

		if ( std::abs(det) < std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: Parallel lines, no single intersection. */
			return false;
		}

		/* NOTE: Constant terms of line equations (form Ax + By = C). */
		const precision_t c1 = (ax * by) - (ay * bx);
		const precision_t c2 = (cx * dy) - (cy * dx);

		/* NOTE: Application of Cramer's method to find the point of intersection. */
		intersection[X] = (c1 * dir_cd_x - dir_ab_x * c2) / det;
		intersection[Y] = (c1 * dir_cd_y - dir_ab_y * c2) / det;

		return true;
	}
}
