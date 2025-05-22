/*
 * src/Libs/VertexFactory/ShapeEdge.hpp
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
#include <limits>
#include <type_traits>

#if IS_WINDOWS
#undef max
#endif

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief The shape edge class.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 */
	template< typename index_data_t = uint32_t >
	requires (std::is_unsigned_v< index_data_t > )
	class ShapeEdge final
	{
		public:

			/**
			 * @brief Constructs a default edge.
			 */
			ShapeEdge () noexcept = default;

			/**
			 * @brief Constructs an edge from vertex indices.
			 * @param vertexIndexA The index of the first point for the edge.
			 * @param vertexIndexB The index of the last point for the edge.
			 */
			constexpr ShapeEdge (index_data_t vertexIndexA, index_data_t vertexIndexB) noexcept
				: m_vertexIndexA(vertexIndexA),
				m_vertexIndexB(vertexIndexB)
			{

			}

			/**
			 * @brief Equality operator.
			 * @param operand A reference to a ShapeEdge.
			 * @return bool
			 */
			bool
			operator== (const ShapeEdge & operand) const noexcept
			{
				if ( this != &operand )
				{
					if ( m_vertexIndexA == operand.m_vertexIndexA && m_vertexIndexB == operand.m_vertexIndexB )
					{
						return true;
					}

					if ( m_vertexIndexA == operand.m_vertexIndexB && m_vertexIndexB == operand.m_vertexIndexA )
					{
						return true;
					}
				}

				return false;
			}

			/**
			 * @brief Inequality operator.
			 * @param operand A reference to a ShapeEdge.
			 * @return bool
			 */
			bool
			operator!= (const ShapeEdge & operand) const noexcept
			{
				return !this->operator==(operand);
			}

			/**
			 * @brief Sets the edge index shared with this one.
			 * @param index The index of the shared edge.
			 * @return void
			 */
			void
			setSharedIndex (index_data_t index) noexcept
			{
				m_sharedEdgeIndex = index;
			}

			/**
			 * @brief Returns whether the edge is valid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_vertexIndexA != m_vertexIndexB;
			}

			/**
			 * @brief Returns the index of the first point for the edge.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			vertexIndexA () const noexcept
			{
				return m_vertexIndexA;
			}

			/**
			 * @brief Returns the index of the last point for the edge.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			vertexIndexB () const noexcept
			{
				return m_vertexIndexB;
			}

			/**
			 * @brief Returns the shared index.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			sharedIndex () const noexcept
			{
				return m_sharedEdgeIndex;
			}

			/**
			 * @brief Checks if the point is equals to this edge.
			 * @param vertexIndexA The index of the first point for the edge.
			 * @param vertexIndexB The index of the last point for the edge.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			same (index_data_t vertexIndexA, index_data_t vertexIndexB) const noexcept
			{
				if ( vertexIndexA == m_vertexIndexA && vertexIndexB == m_vertexIndexB )
				{
					return true;
				}

				if ( vertexIndexA == m_vertexIndexB && vertexIndexB == m_vertexIndexA )
				{
					return true;
				}

				return false;
			}

			/**
			 * @brief Returns whether this edge is shared.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShared () const noexcept
			{
				return m_sharedEdgeIndex < std::numeric_limits< index_data_t >::max();
			}

		private:

			index_data_t m_vertexIndexA{0};
			index_data_t m_vertexIndexB{0};
			index_data_t m_sharedEdgeIndex{std::numeric_limits< index_data_t >::max()};
	};
}
