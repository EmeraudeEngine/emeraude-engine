/*
 * src/Libs/Math/Space2D/Collisions/PointSegment.hpp
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
#include "Libs/Math/Space2D/Point.hpp"
#include "Libs/Math/Space2D/Segment.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a point is on a segment.
	 * @tparam precision_t The floating-point type for the coordinates.
	 * @param point The point to check.
	 * @param segment The segment to check against.
	 * @param epsilon A small tolerance for floating-point comparisons.
	 * @return true if the point is on the segment, false otherwise.
	 */
	template<typename precision_t>
	requires std::is_floating_point_v<precision_t>
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Segment< precision_t > & segment, precision_t epsilon = std::numeric_limits< precision_t >::epsilon()) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto & p = point.position;
		const auto & a = segment.a;
		const auto & b = segment.b;

		const auto ab = b - a;
		const auto ap = p - a;

		/* Check if the point is collinear with the segment. */
		const precision_t crossProduct = ab.x() * ap.y() - ab.y() * ap.x();

		if ( std::abs(crossProduct) > epsilon )
		{
			/* Not collinear. */
			return false;
		}

		/* Check if the point is within the segment's bounds. */
		const precision_t dotProduct = Vector<2, precision_t>::dotProduct(ap, ab);

		if ( dotProduct < 0 )
		{
			/* Before segment start. */
			return false;
		}

		const precision_t squared_length_ab = ab.lengthSquared();

		if ( dotProduct > squared_length_ab )
		{
			/* After segment end. */
			return false;
		}

		return true;
	}
}
