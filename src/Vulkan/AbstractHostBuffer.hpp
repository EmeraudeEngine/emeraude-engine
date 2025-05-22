/*
 * src/Vulkan/AbstractHostBuffer.hpp
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
#include <memory>
#include <mutex>
#include <vector>

/* Local inclusions for inheritances. */
#include "Buffer.hpp"
#include "DeviceMemory.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class MemoryRegion;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines an abstract class for all host-side buffers in Vulkan API.
	 * @extends EmEn::Vulkan::Buffer This is the base for Vulkan buffer logics.
	 */
	class AbstractHostBuffer : public Buffer
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractHostBuffer (const AbstractHostBuffer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractHostBuffer (AbstractHostBuffer && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			AbstractHostBuffer & operator= (const AbstractHostBuffer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			AbstractHostBuffer & operator= (AbstractHostBuffer && copy) noexcept = delete;

			/**
			 * @brief Destructs an abstract host buffer.
			 */
			~AbstractHostBuffer () override = default;

			/**
			 * @brief Writes data into the host (CPU side) video memory.
			 * @param memoryRegion A reference to a memory region to perform the copy from source to destination.
			 * @return bool
			 */
			bool writeData (const MemoryRegion & memoryRegion) const noexcept;

			/**
			 * @brief Writes data into the host (CPU side) video memory.
			 * @param memoryRegions A reference to a list of memory region to perform the copy from source to destination.
			 * @return bool
			 */
			bool writeData (const std::vector< MemoryRegion > & memoryRegions) noexcept;

			/**
			 * @brief Maps the video memory to be able to write in it.
			 * @param offset The beginning of the map.
			 * @param size The size of the mapping.
			 * @return void *
			 */
			template< typename pointer_t >
			[[nodiscard]]
			pointer_t *
			mapMemory (VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const noexcept
			{
				return static_cast< pointer_t * >(this->deviceMemory()->mapMemory(offset, size));
			}

			/**
			 * @brief Unmaps the video memory.
			 * @return void
			 */
			void
			unmapMemory () const noexcept
			{
				this->deviceMemory()->unmapMemory();
			}

		protected:

			/**
			 * @brief Constructs an abstract host buffer.
			 * @param device A reference to a smart pointer to the device where the buffer will be created.
			 * @param createFlags The createInfo flags.
			 * @param deviceSize The size in bytes.
			 * @param bufferUsageFlags The buffer usage flags.
			 */
			AbstractHostBuffer (const std::shared_ptr< Device > & device, VkBufferCreateFlags createFlags, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags) noexcept
				: Buffer{device, createFlags, deviceSize, bufferUsageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}
			{

			}

		private:

			mutable std::mutex m_memoryAccess;
	};
}
