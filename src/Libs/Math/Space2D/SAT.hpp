/*
 * src/Libs/Math/Space2D/SAT.hpp
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

/* STL inclusions. */
#include <limits>

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"
#include "Libs/StaticVector.hpp"

namespace EmEn::Libs::Math::Space2D::SAT
{
	/**
	 * @brief Projects the vertices of a shape onto a given axis to find the minimum and maximum projection values.
	 * @tparam vertex_container_t A container type holding 2D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param vertices A container of 2D vectors representing the vertices of the shape.
	 * @param axis The normalized axis onto which the vertices will be projected.
	 * @param[out] min The minimum projection value (scalar) on the axis.
	 * @param[out] max The maximum projection value (scalar) on the axis.
	 */
	template< typename vertex_container_t, typename precision_t >
	void
	project (const vertex_container_t & vertices, const Vector< 2, precision_t > & axis, precision_t & min, precision_t & max) noexcept
		requires (std::is_arithmetic_v< precision_t > && std::is_same_v< typename vertex_container_t::value_type, Vector< 2, precision_t > >)
	{
		min = Vector< 2, precision_t >::dotProduct(vertices[0], axis);
		max = min;

		for ( size_t index = 1; index < vertices.size(); ++index )
		{
			precision_t p = Vector< 2, precision_t >::dotProduct(vertices[index], axis);

			if ( p < min )
			{
				min = p;
			}
			else if ( p > max )
			{
				max = p;
			}
		}
	}

	/**
	 * @brief Checks for collision between two convex polygons using the Separating Axis Theorem (SAT).
	 * @note If a collision is detected, this function also computes the Minimum Translation Vector (MTV)
	 * that can be used to resolve the collision. The MTV represents the smallest vector to move
	 * shape A out of shape B.
	 * @tparam vertex_container_a_t A container type holding 2D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam vertex_container_b_t A container type holding 2D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param verticesA A container of 2D vectors representing the vertices of the first polygon.
	 * @param verticesB A container of 2D vectors representing the vertices of the second polygon.
	 * @param[out] MTV The Minimum Translation Vector. If no collision occurs, its value is undefined.
	 * @return bool Returns true if a collision is detected, false otherwise.
	 */
	template< typename vertex_container_a_t, typename vertex_container_b_t, typename precision_t >
	bool
	checkCollision (const vertex_container_a_t & verticesA, const vertex_container_b_t & verticesB, Vector< 2, precision_t > & MTV) noexcept
		requires (std::is_arithmetic_v< precision_t > && std::is_same_v< typename vertex_container_a_t::value_type, Vector< 2, precision_t > > && std::is_same_v< typename vertex_container_b_t::value_type, Vector< 2, precision_t > >)
	{
		precision_t overlap = std::numeric_limits< precision_t >::max();
		Vector< 2, precision_t > smallest_axis;

		StaticVector< Vector< 2, precision_t >, 16 > axes;

		for ( size_t index = 0; index < verticesA.size(); ++index )
		{
			Vector< 2, precision_t > p1 = verticesA[index];
			Vector< 2, precision_t > p2 = verticesA[(index + 1) % verticesA.size()];
			Vector< 2, precision_t > edge = p2 - p1;

			axes.push_back(edge.perpendicular());
		}

		for ( size_t index = 0; index < verticesB.size(); ++index )
		{
			Vector< 2, precision_t > p1 = verticesB[index];
			Vector< 2, precision_t > p2 = verticesB[(index + 1) % verticesB.size()];
			Vector< 2, precision_t > edge = p2 - p1;

			axes.push_back(edge.perpendicular());
		}

		for ( auto & axis : axes )
		{
			axis.normalize();

			precision_t minA, maxA, minB, maxB;
			project(verticesA, axis, minA, maxA);
			project(verticesB, axis, minB, maxB);

			if ( maxA < minB || maxB < minA )
			{
				/* Found a separating axis. */
				return false;
			}

			precision_t o = std::min(maxA, maxB) - std::max(minA, minB);

			if ( o < overlap )
			{
				overlap = o;
				smallest_axis = axis;
			}
		}

		MTV = smallest_axis * overlap;

		/* Ensure MTV points from A to B. */
		Vector< 2, precision_t > centerA, centerB;

		for ( const auto & v : verticesA )
		{
			centerA += v;
		}

		for ( const auto & v : verticesB )
		{
			centerB += v;
		}

		centerA /= static_cast< precision_t >(verticesA.size());
		centerB /= static_cast< precision_t >(verticesB.size());

		const auto direction = centerB - centerA;

		if ( Vector< 2, precision_t >::dotProduct(direction, MTV) < 0 )
		{
			MTV = -MTV;
		}

		return true;
	}
}
