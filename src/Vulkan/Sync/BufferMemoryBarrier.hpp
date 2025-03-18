/*
 * src/Vulkan/Sync/BufferMemoryBarrier.hpp
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

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractObject.hpp"

namespace EmEn::Vulkan
{
	class Buffer;
}

namespace EmEn::Vulkan::Sync
{
	/**
	 * @brief The buffer memory barrier class. This is used for synchronize command buffers.
	 * @extends EmEn::Vulkan::AbstractObject This is a sample Vulkan structure.
	 */
	class BufferMemoryBarrier final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanBufferMemoryBarrier"};

			/**
			 * @brief Constructs a buffer memory barrier.
			 * @param buffer A reference to a buffer.
			 * @param srcAccessMask A bitmask of VkAccessFlagBits specifying a source access mask.
			 * @param dstAccessMask A bitmask of VkAccessFlagBits specifying a destination access mask.
			 */
			BufferMemoryBarrier (const Buffer & buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			BufferMemoryBarrier (const BufferMemoryBarrier & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			BufferMemoryBarrier (BufferMemoryBarrier && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			BufferMemoryBarrier & operator= (const BufferMemoryBarrier & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			BufferMemoryBarrier & operator= (BufferMemoryBarrier && copy) noexcept = default;

			/**
			 * @brief Destructs the buffer memory barrier.
			 */
			~BufferMemoryBarrier () override;

			/**
			 * @brief Returns a reference to the buffer memory barrier vulkan structure.
			 * @param const VkImageMemoryBarrier &
			 */
			[[nodiscard]]
			const VkBufferMemoryBarrier &
			get () const noexcept
			{
				return m_barrier;
			}

		private:

			VkBufferMemoryBarrier m_barrier{};
	};
}
