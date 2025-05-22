/*
 * src/Vulkan/AbstractDeviceBuffer.hpp
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

/* Local inclusions for inheritances. */
#include "Buffer.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class TransferManager;
	class MemoryRegion;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines an abstract class for all device-side buffers in Vulkan API.
	 * @extends EmEn::Vulkan::Buffer This is the base for Vulkan buffer logics.
	 */
	class AbstractDeviceBuffer : public Buffer
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDeviceBuffer (const AbstractDeviceBuffer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDeviceBuffer (AbstractDeviceBuffer && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDeviceBuffer & operator= (const AbstractDeviceBuffer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			AbstractDeviceBuffer & operator= (AbstractDeviceBuffer && copy) noexcept = delete;

			/**
			 * @brief Destructs the device buffer.
			 */
			~AbstractDeviceBuffer () override = default;

			/**
			 * @brief Writes data into the device (GPU side) video memory.
			 * @param transferManager A reference to a transfer manager.
			 * @param memoryRegion A reference to the memory region.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept;

		protected:

			/**
			 * @brief Constructs a device buffer.
			 * @param device A reference to the device smart pointer.
			 * @param createFlags The createInfo flags.
			 * @param deviceSize The size in bytes.
			 * @param bufferUsageFlags The buffer usage flags.
			 */
			AbstractDeviceBuffer (const std::shared_ptr< Device > & device, VkBufferCreateFlags createFlags, VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags) noexcept
				: Buffer{device, createFlags, deviceSize, bufferUsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}
			{

			}
	};
}
