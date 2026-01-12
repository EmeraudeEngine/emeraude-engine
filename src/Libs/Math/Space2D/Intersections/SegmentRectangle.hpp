/*
 * src/Libs/Math/Space2D/Intersections/SegmentRectangle.hpp
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
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/Point.hpp"
#include "Libs/Math/Space2D/Collisions/PointRectangle.hpp"
#include "Libs/StaticVector.hpp"
#include "SegmentSegment.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a segment intersects a rectangle and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param rectangle A reference to a rectangle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Segment< precision_t > & segment, const AARectangle< precision_t > & rectangle, Point< precision_t > & intersection) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		if ( isColliding(Point< precision_t >{segment.startPoint()}, rectangle) )
		{
			intersection = segment.startPoint();

			return true;
		}

		StaticVector< Point< precision_t >, 4 > intersections;

		const auto vertices = rectangle.points();

		for ( size_t index = 0; index < 4; ++index )
		{
			Segment< precision_t > edge(vertices[index], vertices[(index + 1) % 4]);
			Point< precision_t > currentIntersection;

			if ( isIntersecting(segment, edge, currentIntersection) )
			{
				intersections.push_back(currentIntersection);
			}
		}

		if ( intersections.empty() )
		{
			return false;
		}

		precision_t min_dist_sq = std::numeric_limits< precision_t >::max();

		for ( const auto & p : intersections)
		{
			precision_t dist_sq = (segment.startPoint() - p).lengthSquared();

			if ( dist_sq < min_dist_sq )
			{
				min_dist_sq = dist_sq;
				intersection = p;
			}
		}

		return true;
	}

	/**
	 * @brief Checks if a segment intersects a rectangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param rectangle A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const Segment< precision_t > & segment, const AARectangle< precision_t > & rectangle) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		Point< precision_t > dummy;
		
		return isIntersecting(segment, rectangle, dummy);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, const AARectangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const AARectangle< precision_t > & rectangle, const Segment< precision_t > & segment) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Segment< precision_t > &, const AARectangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isIntersecting (const AARectangle< precision_t > & rectangle, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept
		requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, rectangle, intersection);
	}
}
