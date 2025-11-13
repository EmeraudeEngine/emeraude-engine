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
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/StaticVector.hpp"
#include "SegmentSegment.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a line is intersecting a rectangle and gives the intersection points.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param rectangle A reference to a rectangle.
	 * @param intersections A writable container of intersection points.
	 * @return int The number of intersection points.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	int
	isIntersecting (const Line< precision_t > & line, const AARectangle< precision_t > & rectangle, StaticVector< Point< precision_t >, 4 > & intersections) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		intersections.clear();

		const auto vertices = rectangle.points();
		constexpr precision_t epsilon = precision_t{1e-4};

		for ( size_t index = 0; index < 4; ++index )
		{
			Segment< precision_t > edge(vertices[index], vertices[(index + 1) % 4]);
			Point< precision_t > intersection;

			// Convert segment to line for intersection test
			Line< precision_t > edgeLine(edge.startPoint(), (edge.endPoint() - edge.startPoint()).normalized());

			if ( isIntersecting(line, edgeLine, intersection) )
			{
				// Check if intersection point is actually on the segment
				const auto t = Vector< 2, precision_t >::dotProduct(intersection - edge.startPoint(), edge.endPoint() - edge.startPoint()) /
				               (edge.endPoint() - edge.startPoint()).lengthSquared();

				if ( t >= 0 && t <= 1 )
				{
					// Check for duplicate points (avoid adding same corner twice)
					bool isDuplicate = false;
					const precision_t epsilonSq = epsilon * epsilon;
					for ( const auto & existing : intersections )
					{
						if ( (intersection - existing).lengthSquared() < epsilonSq )
						{
							isDuplicate = true;
							break;
						}
					}

					if ( !isDuplicate )
					{
						intersections.push_back(intersection);
					}
				}
			}
		}

		return intersections.size();
	}

	/**
	 * @brief Checks if a line is intersecting a rectangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param rectangle A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Line< precision_t > & line, const AARectangle< precision_t > & rectangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		StaticVector< Point< precision_t >, 4 > intersections;

		return isIntersecting(line, rectangle, intersections) > 0;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, const AARectangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const AARectangle< precision_t > & rectangle, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, const AARectangle< precision_t > &, StaticVector< Point< precision_t >, 4 > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	int
	isIntersecting (const AARectangle< precision_t > & rectangle, const Line< precision_t > & line, StaticVector< Point< precision_t >, 4 > & intersections) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, rectangle, intersections);
	}
}
