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
			 * @param cuboidA A reference to an orientedCuboid.
			 * @param cuboidB A reference to another orientedCuboid.
			 * @param direction A reference to a direction vector. This will output the direction of the collision.
			 * @return data_t
			 */
			static
			data_t
			isIntersecting (const OrientedCuboid< data_t > & cuboidA, const OrientedCuboid< data_t > & cuboidB, Vector< 3, data_t > & direction) noexcept
			{
				/* NOTE: The number of axes to test can be reduced by not testing parallel axes.
				 * This is why a rectangle only has two axes to test.
				 * Some shapes like a rectangle can perform faster if it has its
				 * own projection and getAxes code since a rectangle does not need to test 4 axes but really just 2.
				 * The last separation axis could be used to prime the next iteration of SAT
				 * so that the algorithm could be O(1) in non-intersection cases.
				 * SAT in 3D can end up testing LOTS of axes. */

				auto distance = std::numeric_limits< data_t >::max();

				/* Loop over the axes from box A. */
				for ( const auto & axis : cuboidA.m_normals )
				{
					/* Projects both shapes onto the axis. */
					const auto projectionA = cuboidA.project(axis);
					const auto projectionB = cuboidB.project(axis);

					/* Gets the possible overlap. */
					const auto overlap = projectionA.getOverlap(projectionB);

					/* Do the projections overlap?
					 * If not, then we can guarantee that the shapes do not overlap. */
					if ( overlap <= 0 )
					{
						return 0;
					}

					/* Checks for minimum overlapping. If minimum, we set this one as the smallest. */
					if ( overlap < distance )
					{
						distance = overlap;
						direction = axis;
					}
				}

				/* Loop over the axes from box B. */
				for ( const auto & axis : cuboidB.m_normals )
				{
					/* Projects both shapes onto the axis. */
					const auto projectionA = cuboidA.project(axis);
					const auto projectionB = cuboidB.project(axis);

					/* Gets the possible overlap. */
					const auto overlap = projectionA.getOverlap(projectionB);

					/* Do the projections overlap?
					 * If not, then we can guarantee that the shapes do not overlap. */
					if ( overlap <= 0 )
					{
						return 0;
					}

					/* Checks for minimum overlapping. If minimum, we set this one as the smallest. */
					if ( overlap < distance )
					{
						distance = overlap;
						direction = axis;
					}
				}

				/* If we get here, then we know that every axis had overlap on it,
				 * so we can guarantee an intersection. */
				return distance;
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
