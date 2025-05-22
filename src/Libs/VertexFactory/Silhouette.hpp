/*
 * src/Libs/VertexFactory/Silhouette.hpp
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
#include <cstdint>
#include <vector>
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Shape.hpp"

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The Silhouette detection class.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename vertex_data_t = float, typename index_data_t = uint32_t >
	requires (std::is_floating_point_v< vertex_data_t > && std::is_unsigned_v< index_data_t > )
	class Silhouette final
	{
		public:

			/**
			 * @brief Constructs a default silhouette.
			 */
			Silhouette () noexcept = default;

			/**
			 * @brief Gives access to edges list.
			 * @return std::vector< ShapeEdge< index_data_t > >
			 */
			[[nodiscard]]
			const std::vector< ShapeEdge< index_data_t > > &
			edges () const noexcept
			{
				return m_edges;
			}

			/**
			 * @brief Creates a edges list from a geometry and a point of view.
			 * @param geometry The geometry from which the silhouette will be extracted.
			 * @param pointOfView The point of view to generate the silhouette.
			 * @return bool
			 */
			bool
			build (const Shape< vertex_data_t, index_data_t > & geometry, const Math::CartesianFrame< vertex_data_t > & pointOfView) noexcept
			{
				if ( geometry.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", geometry is invalid !" "\n";

					return false;
				}

				m_edges.clear();

				const auto & triangles = geometry.accessTrianglesList();
				const auto & edges = geometry.accessEdgesList();

				/* Gets the point of view direction. */
				const auto direction = pointOfView.forwardVector();

				/* For each triangle of the geometry */
				for ( const auto & triangle : triangles )
				{
					/* Checks if the triangle is facing the point of view. */
					if ( Math::Vector< 3, vertex_data_t >::dotProduct(triangle.surfaceNormal(), direction) > 0.0F )
					{
						/* If the triangle is visible, then add every edge. */
						m_edges.emplace_back(edges[triangle.edgeIndex(0)]);
						m_edges.emplace_back(edges[triangle.edgeIndex(1)]);
						m_edges.emplace_back(edges[triangle.edgeIndex(2)]);
					}
				}

				return true;
			}

		private:

			std::vector< ShapeEdge< index_data_t > > m_edges;
	};
}
