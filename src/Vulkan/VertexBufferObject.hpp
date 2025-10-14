/*
 * src/Vulkan/VertexBufferObject.hpp
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

/* Local inclusions for inheritances. */
#include "Buffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a convenient way to build a vertex buffer object (VBO).
	 * @extends EmEn::Vulkan::Buffer This is a buffer.
	 */
	class VertexBufferObject final : public Buffer
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanVertexBufferObject"};

			/**
			 * @brief Constructs a vertex buffer object (VBO).
			 * @warning The buffer assumes we will only use float value.
			 * @param device A reference to the device smart pointer.
			 * @param vertexCount The number of vertices that the buffer will hold.
			 * @param vertexElementCount The number of sub-elements that composes one vertex.
			 * @param hostVisible Defines if the VBO must be host visible.
			 */
			VertexBufferObject (const std::shared_ptr< Device > & device, uint32_t vertexCount, uint32_t vertexElementCount, bool hostVisible) noexcept
				: Buffer{
					device,
					0,
					vertexCount * vertexElementCount * sizeof(float),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					hostVisible
				},
				m_vertexCount{vertexCount},
				m_vertexElementCount{vertexElementCount}
			{

			}

			/**
			 * @brief Returns the number of vertices in the buffer.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			vertexCount () const noexcept
			{
				return m_vertexCount;
			}

			/**
			 * @brief Returns the number of elements (floats) composing one complete vertex.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			vertexElementCount () const noexcept
			{
				return m_vertexElementCount;
			}

			/**
			 * @brief Returns the number of elements (floats) in the buffer.
			 * @note Same as VertexBufferObject::vertexCount() * VertexBufferObject::vertexElementCount().
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			elementCount () const noexcept
			{
				return m_vertexCount * m_vertexElementCount;
			}

		private:

			uint32_t m_vertexCount;
			uint32_t m_vertexElementCount;
	};
}
