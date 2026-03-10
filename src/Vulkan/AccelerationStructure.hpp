/*
 * src/Vulkan/AccelerationStructure.hpp
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
#include <memory>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "Buffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Wraps a Vulkan acceleration structure (VK_KHR_acceleration_structure).
	 * @details This class manages the lifecycle of a VkAccelerationStructureKHR and its
	 * backing buffer. It supports both Bottom-Level (BLAS) and Top-Level (TLAS) acceleration
	 * structures. Building geometry into the structure is handled externally.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject To allocate on a device.
	 */
	class AccelerationStructure final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanAccelerationStructure"};

			/**
			 * @brief Constructs an acceleration structure.
			 * @param device A reference to a device smart pointer.
			 * @param type The acceleration structure type (BLAS or TLAS).
			 * @param size The required size in bytes for the backing buffer.
			 */
			AccelerationStructure (const std::shared_ptr< Device > & device, VkAccelerationStructureTypeKHR type, VkDeviceSize size) noexcept
				: AbstractDeviceDependentObject{device},
				m_type{type},
				m_size{size},
				m_backingBuffer{device, 0, size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AccelerationStructure (const AccelerationStructure & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param other A reference to the moved instance.
			 */
			AccelerationStructure (AccelerationStructure && other) noexcept;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			AccelerationStructure & operator= (const AccelerationStructure & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param other A reference to the moved instance.
			 */
			AccelerationStructure & operator= (AccelerationStructure && other) noexcept;

			/**
			 * @brief Destructs the acceleration structure.
			 */
			~AccelerationStructure () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept final;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept final;

			/**
			 * @brief Returns the acceleration structure handle.
			 * @return VkAccelerationStructureKHR
			 */
			[[nodiscard]]
			VkAccelerationStructureKHR
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the acceleration structure type.
			 * @return VkAccelerationStructureTypeKHR
			 */
			[[nodiscard]]
			VkAccelerationStructureTypeKHR
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the backing buffer size in bytes.
			 * @return VkDeviceSize
			 */
			[[nodiscard]]
			VkDeviceSize
			size () const noexcept
			{
				return m_size;
			}

			/**
			 * @brief Returns the device address of the acceleration structure.
			 * @note The acceleration structure must be created before calling this.
			 * @return VkDeviceAddress
			 */
			[[nodiscard]]
			VkDeviceAddress
			deviceAddress () const noexcept
			{
				return m_deviceAddress;
			}

			/**
			 * @brief Returns a reference to the backing buffer.
			 * @return const Buffer &
			 */
			[[nodiscard]]
			const Buffer &
			backingBuffer () const noexcept
			{
				return m_backingBuffer;
			}

		private:

			VkAccelerationStructureKHR m_handle{VK_NULL_HANDLE};
			VkAccelerationStructureTypeKHR m_type{VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR};
			VkDeviceSize m_size{0};
			VkDeviceAddress m_deviceAddress{0};
			PFN_vkDestroyAccelerationStructureKHR m_fpDestroyAccelerationStructure{nullptr};
			Buffer m_backingBuffer;
	};
}
