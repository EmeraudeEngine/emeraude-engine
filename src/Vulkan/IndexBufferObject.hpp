/*
 * src/Vulkan/IndexBufferObject.hpp
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
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Buffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a convenient way to build an index buffer object (IBO).
	 * @extends EmEn::Vulkan::Buffer This is a buffer.
	 */
	class IndexBufferObject final : public Buffer
	{
		public:

			/**
			 * @brief Constructs an index buffer object (IBO).
			 * @param device A reference to the device smart pointer.
			 * @param indexCount The number of indices the buffer will hold
			 */
			IndexBufferObject (const std::shared_ptr< Device > & device, uint32_t indexCount) noexcept
				: Buffer{device, 0, indexCount * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false
				},
				m_indexCount{indexCount}
			{

			}

			/**
			 * @brief Returns the number of indices in this buffer.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			indexCount () const noexcept
			{
				return m_indexCount;
			}

		private:

			uint32_t m_indexCount;
	};
}
