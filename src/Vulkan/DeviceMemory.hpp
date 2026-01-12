/*
 * src/Vulkan/DeviceMemory.hpp
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
#include <cstddef>
#include <memory>

/* Local inclusions. */
#include "AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The device memory wrapper class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class DeviceMemory final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDeviceMemory"};

			/**
			 * @brief Constructs a device memory.
			 * @param device A reference to a smart pointer of the device.
			 * @param requirement A reference to a memory requirement.
			 * @param propertyFlags The type of memory.
			 */
			DeviceMemory (const std::shared_ptr< Device > & device, const VkMemoryRequirements2 & requirement, VkMemoryPropertyFlags propertyFlags) noexcept
				: AbstractDeviceDependentObject{device},
				m_requirement{requirement},
				m_propertyFlags{propertyFlags}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DeviceMemory (const DeviceMemory & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DeviceMemory (DeviceMemory && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			DeviceMemory & operator= (const DeviceMemory & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			DeviceMemory & operator= (DeviceMemory && copy) noexcept = default;

			/**
			 * @brief Destructs the device memory.
			 */
			~DeviceMemory () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Maps the video memory to be able to write in it.
			 * @param offset The beginning of the map.
			 * @param size The size of the mapping.
			 * @return void *
			 */
			[[nodiscard]]
			void * mapMemory (VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const noexcept;

			/**
			 * @brief Unmaps the video memory.
			 * @return void
			 */
			void unmapMemory () const noexcept;

			/**
			 * @brief Returns the device memory vulkan handle.
			 * @return VkDeviceMemory
			 */
			[[nodiscard]]
			VkDeviceMemory
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the memory requirement.
			 * @return const VkMemoryRequirements2 &
			 */
			[[nodiscard]]
			const VkMemoryRequirements2 &
			memoryRequirement () const noexcept
			{
				return m_requirement;
			}

			/**
			 * @brief Returns the memory property flags.
			 * @return VkMemoryPropertyFlags
			 */
			[[nodiscard]]
			VkMemoryPropertyFlags
			memoryPropertyFlags () const noexcept
			{
				return m_propertyFlags;
			}

			/**
			 * @brief Returns the size in bytes of the allocated memory.
			 * @note More convenient way than DeviceMemory::memoryRequirement().size.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			bytes () const noexcept
			{
				return m_requirement.memoryRequirements.size;
			}

		private:

			VkDeviceMemory m_handle{VK_NULL_HANDLE};
			VkMemoryRequirements2 m_requirement;
			VkMemoryPropertyFlags m_propertyFlags;
	};
}
