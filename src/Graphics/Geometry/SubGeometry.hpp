/*
 * src/Graphics/Geometry/SubGeometry.hpp
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
#include <array>
#include <cstdint>

/* Local inclusions for inheritances */
#include "Libs/FlagTrait.hpp"
#include "Helpers.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Defines how to use an IBO or a VBO to draw a specific geometry or sub-geometry.
	 * @extends EmEn::Libs::FlagTrait
	 */
	class SubGeometry final : public Libs::FlagTrait< uint32_t >
	{
		public:

			/**
			 * @brief Constructs a sub geometry.
			 * @param offset Sets where to execute in the VBO or IBO.
			 * @param length Sets the length in VBO or IBO.
			 * @param enableDoubleSidedFaces Enable double sided faces. Default false.
			 */
			explicit
			SubGeometry (uint32_t offset, uint32_t length, bool enableDoubleSidedFaces = false) noexcept
				: FlagTrait{enableDoubleSidedFaces ? EnableDoubleSidedFaces : Regular},
				m_range{offset, length}
			{

			}

			/**
			 * @brief Returns where to execute in the VBO or IBO to draw the geometry.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			offset () const noexcept
			{
				return m_range[0];
			}

			/**
			 * @brief Returns where the length from offset in the VBO or IBO to draw the geometry.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			length () const noexcept
			{
				return m_range[1];
			}

			/**
			 * @brief Returns the sub-geometry range [offset, length].
			 * @return const std::array< uint32_t, 2 > &
			 */
			[[nodiscard]]
			const std::array< uint32_t, 2 > &
			range () const noexcept
			{
				return m_range;
			}

		private:

			std::array< uint32_t, 2 > m_range;
	};
}
