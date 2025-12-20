/*
 * src/Libs/Math/OrientedCuboid.hpp
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

/* STL inclusions. */
#include <array>
#include <cmath>
#include <sstream>
#include <string>

/* Local inclusions for usages. */
#include "Space3D/AACuboid.hpp"
#include "CartesianFrame.hpp"
#include "Range.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief Defines a cuboid volume oriented by coordinates.
	 * @tparam data_t The type used for geometric distance and dimensions. Default float.
	 */
	template< typename data_t = float >
	requires (std::is_arithmetic_v< data_t >)
	class OrientedCuboid final
	{
		public:

			using VertexArray = std::array< Vector< 3, data_t >, 8 >;
			using NormalArray = std::array< Vector< 3, data_t >, 6 >;

			/**
			 * @brief Constructs an oriented cuboid.
			 */
			OrientedCuboid () noexcept = default;

			/**
			 * @brief Constructs an oriented cuboid from a cuboid at a specific coordinates.
			 * @param cuboid A reference to a cubic volume.
			 * @param coordinates A reference to a coordinates.
			 */
			OrientedCuboid (const Space3D::AACuboid< data_t > & cuboid, const CartesianFrame< data_t > & coordinates) noexcept
			{
				this->set(cuboid, coordinates);
			}

			/**
			 * @brief Sets the oriented box parameters from a cuboid and a coordinates.
			 * @param cuboid A reference to a cuboid.
			 * @param coordinates A reference to a coordinates to transform the cuboid.
			 * @return bool
			 */
			bool
			set (const Space3D::AACuboid< data_t > & cuboid, const CartesianFrame< data_t > & coordinates) noexcept
			{
				if ( !cuboid.isValid() )
				{
					return false;
				}

				const auto matrix = coordinates.getModelMatrix();
				const auto & max = cuboid.maximum();
				const auto & min = cuboid.minimum();

				/* Transform the new position for vertices. */
				m_vertices[PositiveXPositiveYPositiveZ] = (matrix * Vector< 4, data_t >(max[X], max[Y], max[Z], 1.0F)).toVector3();
				m_vertices[PositiveXPositiveYNegativeZ] = (matrix * Vector< 4, data_t >(max[X], max[Y], min[Z], 1.0F)).toVector3();
				m_vertices[PositiveXNegativeYPositiveZ] = (matrix * Vector< 4, data_t >(max[X], min[Y], max[Z], 1.0F)).toVector3();
				m_vertices[PositiveXNegativeYNegativeZ] = (matrix * Vector< 4, data_t >(max[X], min[Y], min[Z], 1.0F)).toVector3();
				m_vertices[NegativeXPositiveYPositiveZ] = (matrix * Vector< 4, data_t >(min[X], max[Y], max[Z], 1.0F)).toVector3();
				m_vertices[NegativeXPositiveYNegativeZ] = (matrix * Vector< 4, data_t >(min[X], max[Y], min[Z], 1.0F)).toVector3();
				m_vertices[NegativeXNegativeYPositiveZ] = (matrix * Vector< 4, data_t >(min[X], min[Y], max[Z], 1.0F)).toVector3();
				m_vertices[NegativeXNegativeYNegativeZ] = (matrix * Vector< 4, data_t >(min[X], min[Y], min[Z], 1.0F)).toVector3();

				/* Rebuild the normals from the new vertices position. */
				m_normals[PositiveX] = Vector< 3, data_t >::normal(m_vertices[PositiveXPositiveYPositiveZ], m_vertices[PositiveXNegativeYPositiveZ], m_vertices[PositiveXNegativeYNegativeZ]);
				m_normals[NegativeX] = Vector< 3, data_t >::normal(m_vertices[NegativeXPositiveYPositiveZ], m_vertices[NegativeXPositiveYNegativeZ], m_vertices[NegativeXNegativeYNegativeZ]);
				m_normals[PositiveY] = Vector< 3, data_t >::normal(m_vertices[PositiveXPositiveYPositiveZ], m_vertices[PositiveXPositiveYNegativeZ], m_vertices[NegativeXPositiveYNegativeZ]);
				m_normals[NegativeY] = Vector< 3, data_t >::normal(m_vertices[PositiveXNegativeYPositiveZ], m_vertices[NegativeXNegativeYPositiveZ], m_vertices[NegativeXNegativeYNegativeZ]);
				m_normals[PositiveZ] = Vector< 3, data_t >::normal(m_vertices[PositiveXPositiveYPositiveZ], m_vertices[NegativeXPositiveYPositiveZ], m_vertices[NegativeXNegativeYPositiveZ]);
				m_normals[NegativeZ] = Vector< 3, data_t >::normal(m_vertices[NegativeXPositiveYNegativeZ], m_vertices[PositiveXPositiveYNegativeZ], m_vertices[PositiveXNegativeYNegativeZ]);

				m_width = cuboid.width();
				m_height = cuboid.height();
				m_depth = cuboid.depth();

				return true;
			}

			/**
			 * @brief Returns the list of vertex positions to build the oriented cuboid.
			 * @return const VertexArray &
			 */
			[[nodiscard]]
			const VertexArray &
			points () const noexcept
			{
				return m_vertices;
			}

			/**
			 * @brief Returns the list of vertex normals to build the oriented cuboid.
			 * @return const NormalArray &
			 */
			[[nodiscard]]
			const NormalArray &
			normals () const noexcept
			{
				return m_normals;
			}

			/**
			 * @brief Transfer points from the oriented cuboid to a cubic volume.
			 * @param cuboid A reference to a Cuboid.
			 * @return void
			 */
			void
			merge (Space3D::AACuboid< data_t > & cuboid) noexcept
			{
				for ( const auto & vertex : m_vertices )
				{
					cuboid.merge(vertex);
				}
			}

			/**
			 * @brief Minimum Translation Vector (MTV): direction.scale(return).
			 * @note http://www.dyn4j.org/2010/01/sat/
			 * @note Convention: MTV pushes cuboidA out of cuboidB.
			 * @param cuboidA A reference to an orientedCuboid.
			 * @param cuboidB A reference to another orientedCuboid.
			 * @param direction A reference to a direction vector. This will output the MTV direction.
			 * @return data_t The penetration depth (0 if no collision).
			 */
			static
			data_t
			isIntersecting (const OrientedCuboid< data_t > & cuboidA, const OrientedCuboid< data_t > & cuboidB, Vector< 3, data_t > & direction) noexcept
			{
				/* NOTE: Full SAT for 3D OBB vs OBB requires testing 15 axes:
				 * - 3 face normals from box A
				 * - 3 face normals from box B
				 * - 9 cross products of edges (3 edges A × 3 edges B)
				 * Missing the edge cross products can cause false positives. */

				auto minOverlap = std::numeric_limits< data_t >::max();
				Vector< 3, data_t > minAxis;

				/* Helper lambda to test a single axis. Returns false if separation found. */
				auto testAxis = [&cuboidA, &cuboidB, &minOverlap, &minAxis] (const Vector< 3, data_t > & axis) -> bool
				{
					/* Skip degenerate axes (from parallel edges). */
					const auto lengthSq = axis.lengthSquared();

					if ( lengthSq < std::numeric_limits< data_t >::epsilon() )
					{
						return true;
					}

					/* Normalize the axis. */
					const auto normalizedAxis = axis / std::sqrt(lengthSq);

					/* Project both shapes onto the axis. */
					const auto projectionA = cuboidA.project(normalizedAxis);
					const auto projectionB = cuboidB.project(normalizedAxis);

					/* Get the overlap. */
					const auto overlap = projectionA.getOverlap(projectionB);

					/* If no overlap, we found a separating axis. */
					if ( overlap <= 0 )
					{
						return false;
					}

					/* Track minimum overlap for MTV. */
					if ( overlap < minOverlap )
					{
						minOverlap = overlap;
						minAxis = normalizedAxis;
					}

					return true;
				};

				/* Test the 3 face normals from box A (only need positive directions). */
				if ( !testAxis(cuboidA.m_normals[PositiveX]) ) { return 0; }
				if ( !testAxis(cuboidA.m_normals[PositiveY]) ) { return 0; }
				if ( !testAxis(cuboidA.m_normals[PositiveZ]) ) { return 0; }

				/* Test the 3 face normals from box B. */
				if ( !testAxis(cuboidB.m_normals[PositiveX]) ) { return 0; }
				if ( !testAxis(cuboidB.m_normals[PositiveY]) ) { return 0; }
				if ( !testAxis(cuboidB.m_normals[PositiveZ]) ) { return 0; }

				/* Test the 9 edge cross products.
				 * Edges of each box are aligned with their face normals. */
				const auto & axesA = cuboidA.m_normals;
				const auto & axesB = cuboidB.m_normals;

				/* A.X × B.X, A.X × B.Y, A.X × B.Z */
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveX], axesB[PositiveX])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveX], axesB[PositiveY])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveX], axesB[PositiveZ])) ) { return 0; }

				/* A.Y × B.X, A.Y × B.Y, A.Y × B.Z */
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveY], axesB[PositiveX])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveY], axesB[PositiveY])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveY], axesB[PositiveZ])) ) { return 0; }

				/* A.Z × B.X, A.Z × B.Y, A.Z × B.Z */
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveZ], axesB[PositiveX])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveZ], axesB[PositiveY])) ) { return 0; }
				if ( !testAxis(Vector< 3, data_t >::crossProduct(axesA[PositiveZ], axesB[PositiveZ])) ) { return 0; }

				/* Collision confirmed. Ensure MTV direction pushes A out of B.
				 * Calculate centers and check if minAxis points from B to A. */
				Vector< 3, data_t > centerA, centerB;

				for ( const auto & vertex : cuboidA.m_vertices )
				{
					centerA += vertex;
				}

				for ( const auto & vertex : cuboidB.m_vertices )
				{
					centerB += vertex;
				}

				centerA /= static_cast< data_t >(8);
				centerB /= static_cast< data_t >(8);

				/* If the axis points from A to B, flip it so MTV pushes A out of B. */
				if ( Vector< 3, data_t >::dotProduct(centerA - centerB, minAxis) < 0 )
				{
					minAxis = -minAxis;
				}

				direction = minAxis;

				return minOverlap;
			}

			/**
			 * @brief Constructs an axis aligned box from this oriented cuboid.
			 * @return Space3D::AACuboid< data_t >
			 */
			[[nodiscard]]
			Space3D::AACuboid< data_t >
			getAxisAlignedBox () const noexcept
			{
				Space3D::AACuboid< data_t > box;

				for ( const auto & point : m_vertices )
				{
					box.merge(point);
				}

				return box;
			}

			/**
			 * @brief Returns the width of the box (X axis).
			 * @return type_t
			 */
			[[nodiscard]]
			data_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the height of the box (Y axis).
			 * @return type_t
			 */
			[[nodiscard]]
			data_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the depth of the box (Z axis).
			 * @return type_t
			 */
			[[nodiscard]]
			data_t
			depth () const noexcept
			{
				return m_depth;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const OrientedCuboid & obj) noexcept
			{
				out << "Oriented bounding box data :\n";

				size_t index = 0;

				for ( const auto & vertex : obj.m_vertices )
				{
					out << "Vertex #" << index++ << " : " << vertex << '\n';
				}

				index = 0;

				for ( const auto & normal : obj.m_normals )
				{
					out << "Normal #" << index++ << " : " << normal << '\n';
				}

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const OrientedCuboid & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			enum VertexIndex
			{
				PositiveXPositiveYPositiveZ = 0,
				PositiveXPositiveYNegativeZ = 1,
				PositiveXNegativeYPositiveZ = 2,
				PositiveXNegativeYNegativeZ = 3,
				NegativeXPositiveYPositiveZ = 4,
				NegativeXPositiveYNegativeZ = 5,
				NegativeXNegativeYPositiveZ = 6,
				NegativeXNegativeYNegativeZ = 7
			};

			enum NormalIndex
			{
				PositiveX = 0,
				NegativeX = 1,
				PositiveY = 2,
				NegativeY = 3,
				PositiveZ = 4,
				NegativeZ = 5
			};

			/**
			 * @brief Projects the cuboid on a range.
			 * @param normal A reference to a vector.
			 * @return Range< data_t >
			 */
			[[nodiscard]]
			Range< data_t >
			project (const Vector< 3, data_t > & normal) const noexcept
			{
				Range< data_t > projection{};

				for ( const auto & vertex : m_vertices )
				{
					projection.update(Vector<3, data_t>::dotProduct(normal, vertex));
				}

				return projection;
			}

			VertexArray m_vertices{};
			NormalArray m_normals{};
			/* FIXME : Extract these from vertices ! */
			data_t m_width = 0;
			data_t m_height = 0;
			data_t m_depth = 0;
	};
}
