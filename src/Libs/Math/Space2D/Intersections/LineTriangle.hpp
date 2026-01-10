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
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/StaticVector.hpp"
#include "SegmentSegment.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a line is intersecting a triangle and gives the intersection points.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param triangle A reference to a triangle.
	 * @param intersections A writable container of intersection points.
	 * @return uint32_t The number of intersection points.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	uint32_t
	isIntersecting (const Line< precision_t > & line, const Triangle< precision_t > & triangle, StaticVector< Point< precision_t >, 4 > & intersections) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		intersections.clear();

		const auto & points = triangle.points();
		constexpr precision_t epsilon = precision_t{1e-4};

		for ( size_t index = 0; index < 3; ++index )
		{
			Segment< precision_t > edge(points[index], points[(index + 1) % 3]);
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
					// Check for duplicate points (avoid adding same vertex twice)
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

		return static_cast< uint32_t >(intersections.size());
	}

	/**
	 * @brief Checks if a line is intersecting a triangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Line< precision_t > & line, const Triangle< precision_t > & triangle) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		StaticVector< Point< precision_t >, 4 > intersections;

		return isIntersecting(line, triangle, intersections) > 0;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, const Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Triangle< precision_t > & triangle, const Line< precision_t > & line) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, const Triangle< precision_t > &, StaticVector< Point< precision_t >, 4 > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	uint32_t
	isIntersecting (const Triangle< precision_t > & triangle, const Line< precision_t > & line, StaticVector< Point< precision_t >, 4 > & intersections) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, triangle, intersections);
	}
}
