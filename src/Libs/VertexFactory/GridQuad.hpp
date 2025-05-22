/*
 * src/Libs/VertexFactory/GridQuad.hpp
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
#include <sstream>
#include <string>
#include <type_traits>

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief Describe a grid cell.
	 * @tparam index_data_t The precision type of index data. Default uint32_t.
	 * @note A cell is a square.
	 */
	template< typename index_data_t = uint32_t >
	requires (std::is_unsigned_v< index_data_t > )
	class GridQuad final
	{
		public:

			/**
			 * @brief Constructs a default grid quad.
			 */
			GridQuad () noexcept = default;

			/**
			 * @brief Constructs a grid quad.
			 * @param topLeftIndex
			 * @param bottomLeftIndex
			 * @param topRightIndex
			 * @param bottomRightIndex
			 */
			GridQuad (index_data_t topLeftIndex, index_data_t bottomLeftIndex, index_data_t topRightIndex, index_data_t bottomRightIndex) noexcept
				: m_topLeftIndex(topLeftIndex),
				m_bottomLeftIndex(bottomLeftIndex),
				m_topRightIndex(topRightIndex),
				m_bottomRightIndex(bottomRightIndex)
			{

			}

			/**
			 * @brief Returns whether the quad is valid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_topLeftIndex != 0 || m_bottomLeftIndex != 0 || m_topRightIndex != 0 || m_bottomRightIndex != 0;
			}

			/**
			 * @brief Sets the quad top left to vertex index.
			 * @param index The vertex index from the geometry data.
			 * @return void
			 */
			void
			setTopLeftIndex (index_data_t index) noexcept
			{
				m_topLeftIndex = index;
			}

			/**
			 * @brief Sets the quad bottom left to vertex index.
			 * @param index The vertex index from the geometry data.
			 * @return void
			 */
			void
			setBottomLeftIndex (index_data_t index) noexcept
			{
				m_bottomLeftIndex = index;
			}

			/**
			 * @brief Sets the quad top right to vertex index.
			 * @param index The vertex index from the geometry data.
			 * @return void
			 */
			void
			setTopRightIndex (index_data_t index) noexcept
			{
				m_topRightIndex = index;
			}

			/**
			 * @brief Sets the quad bottom right to vertex index.
			 * @param index The vertex index from the geometry data.
			 * @return void
			 */
			void
			setBottomRightIndex (index_data_t index) noexcept
			{
				m_bottomRightIndex = index;
			}

			/**
			 * @brief Returns the quad top left index to the geometry data.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			topLeftIndex () const noexcept
			{
				return m_topLeftIndex;
			}

			/**
			 * @brief Returns the quad bottom left index to the geometry data.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			bottomLeftIndex () const noexcept
			{
				return m_bottomLeftIndex;
			}

			/**
			 * @brief Returns the quad top right index to the geometry data.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			topRightIndex () const noexcept
			{
				return m_topRightIndex;
			}

			/**
			 * @brief Returns the quad bottom right index to the geometry data.
			 * @return index_data_t
			 */
			[[nodiscard]]
			index_data_t
			bottomRightIndex () const noexcept
			{
				return m_bottomRightIndex;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const GridQuad & obj) noexcept
			{
				return out << "Grid quad data :\n"
					"Top-left index : " << obj.m_topLeftIndex << "\n"
					"Bottom-left index : " << obj.m_bottomLeftIndex << "\n"
					"Top-Right index : " << obj.m_topRightIndex << "\n"
					"Bottom-Right index : " << obj.m_bottomRightIndex << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const GridQuad & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			index_data_t m_topLeftIndex{0};
			index_data_t m_bottomLeftIndex{0};
			index_data_t m_topRightIndex{0};
			index_data_t m_bottomRightIndex{0};
	};
}
