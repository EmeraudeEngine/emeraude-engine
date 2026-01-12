/*
 * src/Libs/Math/Space3D/SAT.hpp
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

namespace EmEn::Libs::Math::Space3D::SAT
{
	/**
	 * @brief Projects the vertices of a shape onto a given axis to find the minimum and maximum projection values.
	 * @tparam vertex_container_t A container type holding 3D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param vertices A container of 3D vectors representing the vertices of the shape.
	 * @param axis The normalized axis onto which the vertices will be projected.
	 * @param[out] min The minimum projection value (scalar) on the axis.
	 * @param[out] max The maximum projection value (scalar) on the axis.
	 */
	template< typename vertex_container_t, typename precision_t >
	void
	project (const vertex_container_t & vertices, const Vector< 3, precision_t > & axis, precision_t & min, precision_t & max) noexcept
		requires (std::is_arithmetic_v< precision_t > && std::is_same_v< typename vertex_container_t::value_type, Vector< 3, precision_t > >)
	{
		min = Vector< 3, precision_t >::dotProduct(vertices[0], axis);
		max = min;

		for ( size_t index = 1; index < vertices.size(); ++index )
		{
			precision_t p = Vector< 3, precision_t >::dotProduct(vertices[index], axis);

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
	 * @brief Checks for collision between two triangles using the Separating Axis Theorem (SAT) in 3D.
	 * @note For triangles, we test: (1) face normals of both triangles (2 axes)
	 * and (2) cross products of all edge pairs (9 axes). If a collision is detected,
	 * this function also computes the Minimum Translation Vector (MTV).
	 * @tparam vertex_container_a_t A container type holding 3D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam vertex_container_b_t A container type holding 3D vectors (e.g., std::vector, std::array, StaticVector).
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param verticesA A container of 3D vectors representing the vertices of the first triangle (must have 3 vertices).
	 * @param verticesB A container of 3D vectors representing the vertices of the second triangle (must have 3 vertices).
	 * @param[out] MTV The Minimum Translation Vector. If no collision occurs, its value is undefined.
	 * @return bool Returns true if a collision is detected, false otherwise.
	 */
	template< typename vertex_container_a_t, typename vertex_container_b_t, typename precision_t >
	bool
	checkCollision (const vertex_container_a_t & verticesA, const vertex_container_b_t & verticesB, Vector< 3, precision_t > & MTV) noexcept
		requires (std::is_arithmetic_v< precision_t > && std::is_same_v< typename vertex_container_a_t::value_type, Vector< 3, precision_t > > && std::is_same_v< typename vertex_container_b_t::value_type, Vector< 3, precision_t > >)
	{
		if ( verticesA.size() != 3 || verticesB.size() != 3 )
		{
			MTV.reset();

			return false;
		}

		precision_t overlap = std::numeric_limits< precision_t >::max();
		Vector< 3, precision_t > smallest_axis;

		StaticVector< Vector< 3, precision_t >, 32 > axes;

		/* Compute face normal for triangle A. */
		Vector< 3, precision_t > normalA;

		{
			Vector< 3, precision_t > edge1 = verticesA[1] - verticesA[0];
			Vector< 3, precision_t > edge2 = verticesA[2] - verticesA[0];
			normalA = Vector< 3, precision_t >::crossProduct(edge1, edge2);

			if ( normalA.lengthSquared() > std::numeric_limits< precision_t >::epsilon() )
			{
				normalA.normalize();
				axes.push_back(normalA);
			}
			else
			{
				/* Degenerate triangle A. */
				MTV.reset();

				return false;
			}
		}

		/* Compute face normal for triangle B. */
		Vector< 3, precision_t > normalB;

		{
			Vector< 3, precision_t > edge1 = verticesB[1] - verticesB[0];
			Vector< 3, precision_t > edge2 = verticesB[2] - verticesB[0];
			normalB = Vector< 3, precision_t >::crossProduct(edge1, edge2);

			if ( normalB.lengthSquared() > std::numeric_limits< precision_t >::epsilon() )
			{
				normalB.normalize();

				/* Only add if not parallel to normalA. */
				if ( std::abs(Vector< 3, precision_t >::dotProduct(normalA, normalB)) < (1.0 - std::numeric_limits< precision_t >::epsilon()) )
				{
					axes.push_back(normalB);
				}
			}
			else
			{
				/* Degenerate triangle B. */
				MTV.reset();

				return false;
			}
		}

		/* Check if triangles are coplanar. */
		const bool coplanar = std::abs(Vector< 3, precision_t >::dotProduct(normalA, normalB)) > (1.0 - std::numeric_limits< precision_t >::epsilon() * 10);

		/* Compute edge cross products (3 edges from A × 3 edges from B = 9 axes). */
		for ( size_t index = 0; index < 3; ++index )
		{
			Vector< 3, precision_t > edgeA = verticesA[(index + 1) % 3] - verticesA[index];

			for ( size_t j = 0; j < 3; ++j )
			{
				Vector< 3, precision_t > edgeB = verticesB[(j + 1) % 3] - verticesB[j];
				Vector< 3, precision_t > axis = Vector< 3, precision_t >::crossProduct(edgeA, edgeB);

				if ( axis.lengthSquared() > std::numeric_limits< precision_t >::epsilon() )
				{
					axes.push_back(axis);
				}
			}
		}

		/* For coplanar triangles, also test axes perpendicular to edges in the plane. */
		if ( coplanar )
		{
			for ( size_t index = 0; index < 3; ++index )
			{
				Vector< 3, precision_t > edge = verticesA[(index + 1) % 3] - verticesA[index];
				Vector< 3, precision_t > axis = Vector< 3, precision_t >::crossProduct(normalA, edge);

				if ( axis.lengthSquared() > std::numeric_limits< precision_t >::epsilon() )
				{
					axes.push_back(axis);
				}
			}

			for ( size_t index = 0; index < 3; ++index )
			{
				Vector< 3, precision_t > edge = verticesB[(index + 1) % 3] - verticesB[index];
				Vector< 3, precision_t > axis = Vector< 3, precision_t >::crossProduct(normalB, edge);

				if ( axis.lengthSquared() > std::numeric_limits< precision_t >::epsilon() )
				{
					axes.push_back(axis);
				}
			}
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
		Vector< 3, precision_t > centerA = (verticesA[0] + verticesA[1] + verticesA[2]) / static_cast< precision_t >(3);
		Vector< 3, precision_t > centerB = (verticesB[0] + verticesB[1] + verticesB[2]) / static_cast< precision_t >(3);
		Vector< 3, precision_t > direction = centerB - centerA;

		if ( Vector< 3, precision_t >::dotProduct(direction, MTV) < 0 )
		{
			MTV = -MTV;
		}

		return true;
	}

	/**
	 * @brief Checks if a point is inside a triangle using barycentric coordinates.
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param point The point to test.
	 * @param A First vertex of the triangle.
	 * @param B Second vertex of the triangle.
	 * @param C Third vertex of the triangle.
	 * @return bool Returns true if the point is inside the triangle, false otherwise.
	 */
	template< typename precision_t >
	bool
	pointInTriangle (const Vector< 3, precision_t > & point, const Vector< 3, precision_t > & A, const Vector< 3, precision_t > & B, const Vector< 3, precision_t > & C) noexcept requires (std::is_arithmetic_v< precision_t >)
	{
		/* Compute vectors. */
		Vector< 3, precision_t > v0 = C - A;
		Vector< 3, precision_t > v1 = B - A;
		Vector< 3, precision_t > v2 = point - A;

		/* Compute dot products. */
		precision_t dot00 = Vector< 3, precision_t >::dotProduct(v0, v0);
		precision_t dot01 = Vector< 3, precision_t >::dotProduct(v0, v1);
		precision_t dot02 = Vector< 3, precision_t >::dotProduct(v0, v2);
		precision_t dot11 = Vector< 3, precision_t >::dotProduct(v1, v1);
		precision_t dot12 = Vector< 3, precision_t >::dotProduct(v1, v2);

		/* Compute barycentric coordinates. */
		precision_t invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
		precision_t u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		precision_t v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		/* Check if point is in triangle. */
		return (u >= 0) && (v >= 0) && (u + v <= 1);
	}

	/**
	 * @brief Computes the closest point on a triangle to a given point and returns the MTV.
	 * @tparam precision_t The floating-point precision type (e.g., float, double).
	 * @param point The point to test.
	 * @param A First vertex of the triangle.
	 * @param B Second vertex of the triangle.
	 * @param C Third vertex of the triangle.
	 * @param[out] MTV The minimum translation vector to move the point out of the triangle.
	 * @return bool Returns true if the point is inside the triangle, false otherwise.
	 */
	template< typename precision_t >
	bool
	pointInTriangleWithMTV (const Vector< 3, precision_t > & point, const Vector< 3, precision_t > & A, const Vector< 3, precision_t > & B, const Vector< 3, precision_t > & C, Vector< 3, precision_t > & MTV) noexcept requires (std::is_arithmetic_v< precision_t >)
	{
		/* First check if the point is in the triangle using the plane test. */
		Vector< 3, precision_t > edge1 = B - A;
		Vector< 3, precision_t > edge2 = C - A;
		Vector< 3, precision_t > normal = Vector< 3, precision_t >::crossProduct(edge1, edge2);

		if ( normal.lengthSquared() <= std::numeric_limits< precision_t >::epsilon() )
		{
			MTV.reset();

			return false;
		}

		normal.normalize();

		/* Check if point is in triangle using barycentric coordinates. */
		if ( !pointInTriangle(point, A, B, C) )
		{
			MTV.reset();

			return false;
		}

		/* Compute distances to each edge. */
		auto distanceToEdge = [](const Vector< 3, precision_t > & p, const Vector< 3, precision_t > & edgeStart, const Vector< 3, precision_t > & edgeEnd) -> precision_t
		{
			Vector< 3, precision_t > edge = edgeEnd - edgeStart;
			Vector< 3, precision_t > toPoint = p - edgeStart;
			precision_t t = Vector< 3, precision_t >::dotProduct(toPoint, edge) / Vector< 3, precision_t >::dotProduct(edge, edge);
			t = std::max(static_cast<precision_t>(0), std::min(static_cast<precision_t>(1), t));
			Vector< 3, precision_t > closest = edgeStart + edge * t;

			return Vector< 3, precision_t >::distance(p, closest);
		};

		precision_t distAB = distanceToEdge(point, A, B);
		precision_t distBC = distanceToEdge(point, B, C);
		precision_t distCA = distanceToEdge(point, C, A);

		precision_t minDist = std::min({distAB, distBC, distCA});

		/* Compute MTV based on the closest edge. */
		if ( minDist == distAB )
		{
			Vector< 3, precision_t > edge = B - A;
			Vector< 3, precision_t > toPoint = point - A;
			precision_t t = Vector< 3, precision_t >::dotProduct(toPoint, edge) / Vector< 3, precision_t >::dotProduct(edge, edge);
			Vector< 3, precision_t > closest = A + edge * t;
			MTV = point - closest;
		}
		else if ( minDist == distBC )
		{
			Vector< 3, precision_t > edge = C - B;
			Vector< 3, precision_t > toPoint = point - B;
			precision_t t = Vector< 3, precision_t >::dotProduct(toPoint, edge) / Vector< 3, precision_t >::dotProduct(edge, edge);
			Vector< 3, precision_t > closest = B + edge * t;
			MTV = point - closest;
		}
		else
		{
			Vector< 3, precision_t > edge = A - C;
			Vector< 3, precision_t > toPoint = point - C;
			precision_t t = Vector< 3, precision_t >::dotProduct(toPoint, edge) / Vector< 3, precision_t >::dotProduct(edge, edge);
			Vector< 3, precision_t > closest = C + edge * t;
			MTV = point - closest;
		}

		return true;
	}
}
