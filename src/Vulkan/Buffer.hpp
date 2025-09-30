/*
 * src/Vulkan/Buffer.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "DeviceMemory.hpp"
#include "MemoryRegion.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class TransferManager;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines the base class of all buffers in Vulkan API.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject To allocate memory on a device.
	 */
	class Buffer : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanBuffer"};

			/**
			 * @brief Constructs a buffer.
			 * @param device A reference to a device smart pointer.
			 * @param createFlags The createInfo flags.
			 * @param size The size in bytes.
			 * @param usageFlags The buffer usage flags.
			 * @param memoryPropertyFlag The type of memory flags.
			 */
			Buffer (const std::shared_ptr< Device > & device, VkBufferCreateFlags createFlags, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlag) noexcept
				: AbstractDeviceDependentObject{device},
				m_memoryPropertyFlag{memoryPropertyFlag}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.size = size;
				m_createInfo.usage = usageFlags;
				/* TODO: If one day we had to share a buffer between a dedicated compute and graphics family, we need to describe it here. */
				m_createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				m_createInfo.queueFamilyIndexCount = 0;
				m_createInfo.pQueueFamilyIndices = nullptr;
			}

			/**
			 * @brief Constructs a buffer with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 * @param memoryPropertyFlag The type of memory flags.
			 */
			Buffer (const std::shared_ptr< Device > & device, const VkBufferCreateInfo & createInfo, VkMemoryPropertyFlags memoryPropertyFlag) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo},
				m_memoryPropertyFlag{memoryPropertyFlag}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Buffer (const Buffer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param other A reference to the copied instance.
			 */
			Buffer (Buffer && other) noexcept;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Buffer & operator= (const Buffer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param other A reference to the copied instance.
			 */
			Buffer & operator= (Buffer && other) noexcept;

			/**
			 * @brief Destructs the buffer.
			 */
			~Buffer () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept final;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept final;

			/**
			 * @brief Recreates a new buffer on the device.
			 * @param size The new size of the buffer in bytes.
			 * @return bool
			 */
			bool
			recreateOnHardware (VkDeviceSize size) noexcept
			{
				m_createInfo.size = size;

				this->destroyFromHardware();

				return this->createOnHardware();
			}

			/**
			 * @brief Returns whether the buffer can be read or written by the CPU directly.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isHostVisible () const noexcept
			{
				return (m_memoryPropertyFlag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
			}

			/**
			 * @brief Returns the buffer vulkan handle.
			 * @return VkBuffer
			 */
			[[nodiscard]]
			VkBuffer
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the buffer createInfo.
			 * @return const VkBufferCreateInfo &
			 */
			[[nodiscard]]
			const VkBufferCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the buffer creation flags.
			 * @return VkBufferCreateFlags
			 */
			[[nodiscard]]
			VkBufferCreateFlags
			createFlags () const noexcept
			{
				return m_createInfo.flags;
			}

			/**
			 * @brief Returns the buffer size in bytes.
			 * @warning This information comes from the createInfo, not the device memory.
			 * @return VkDeviceSize
			 */
			[[nodiscard]]
			VkDeviceSize
			bytes () const noexcept
			{
				return m_createInfo.size;
			}

			/**
			 * @brief Returns the buffer usage flags.
			 * @return VkBufferUsageFlags
			 */
			[[nodiscard]]
			VkBufferUsageFlags
			usageFlags () const noexcept
			{
				return m_createInfo.usage;
			}

			/**
			 * @brief Returns the buffer usage memory property flags.
			 * @return VkMemoryPropertyFlags
			 */
			[[nodiscard]]
			VkMemoryPropertyFlags
			memoryPropertyFlags () const noexcept
			{
				return m_memoryPropertyFlag;
			}

			/**
			 * @brief Writes data into the device (GPU side) video memory.
			 * @param transferManager A reference to a transfer manager.
			 * @param memoryRegion A reference to the memory region.
			 * @return bool
			 */
			[[nodiscard]]
			bool transferData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept;

			/**
			 * @brief Writes data into the device (GPU side) video memory.
			 * @param transferManager  A reference to a transfer manager.
			 * @param data A reference to a vector.
			 * @return bool
			 */
			template< typename data_t >
			[[nodiscard]]
			bool
			transferData (TransferManager & transferManager, const std::vector< data_t > & data) noexcept
			{
				const auto bytes = data.size() * sizeof(data_t);

				if ( !this->transferData(transferManager, MemoryRegion{data.data(), bytes}) )
				{
					TraceError{ClassId} << "Unable to transfer " << bytes << " bytes into the buffer !";

					return false;
				}

				return true;
			}

			/**
			 * @brief Writes data into the host (CPU side) video memory.
			 * @warning Only available for host buffers.
			 * @param memoryRegion A reference to a memory region to perform the copy from source to destination.
			 * @return bool
			 */
			bool writeData (const MemoryRegion & memoryRegion) const noexcept;

			/**
			 * @brief Writes data into the host (CPU side) video memory.
			 * @warning Only available for host buffers.
			 * @param memoryRegions A reference to a list of memory region to perform the copy from source to destination.
			 * @return bool
			 */
			bool writeData (const std::vector< MemoryRegion > & memoryRegions) noexcept;

			/**
			 * @brief Writes data into the host (CPU side) video memory.
			 * @warning Only available for host buffers.
			 * @param data A reference to a vector.
			 * @return bool
			 */
			template< typename data_t >
			[[nodiscard]]
			bool
			writeData (const std::vector< data_t > & data) noexcept
			{
				const auto bytes = data.size() * sizeof(data_t);

				if ( !this->writeData(MemoryRegion{data.data(), bytes}) )
				{
					TraceError{ClassId} << "Unable to write " << bytes << " bytes into the buffer !";

					return false;
				}

				return true;
			}

			/**
			 * @brief Maps the video memory to be able to write in it.
			 * @warning Only available for host buffers.
			 * @param offset The beginning of the map.
			 * @param size The size of the mapping.
			 * @return void *
			 */
			template< typename pointer_t >
			[[nodiscard]]
			pointer_t *
			mapMemory (VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) const noexcept
			{
				if ( !this->isHostVisible() )
				{
					Tracer::error(ClassId, "This buffer is not host visible! You can't map it.");

					return nullptr;
				}

				return static_cast< pointer_t * >(m_deviceMemory->mapMemory(offset, size));
			}

			/**
			 * @brief Unmaps the video memory.
			 * @warning Only available for host buffers.
			 * @return void
			 */
			void
			unmapMemory () const noexcept
			{
				if ( this->isHostVisible() )
				{
					m_deviceMemory->unmapMemory();
				}
			}

			/**
			 * @brief Returns the descriptor buffer info.
			 * @param offset Where to start in the buffer.
			 * @param range The data length after offset.
			 * @return VkDescriptorBufferInfo
			 */
			[[nodiscard]]
			VkDescriptorBufferInfo
			getDescriptorInfo (uint32_t /*offset*/, uint32_t range) const noexcept
			{
				/* FIXME: Setting the offset breaks some scenes! */

				VkDescriptorBufferInfo descriptorInfo{};
				descriptorInfo.buffer = m_handle;
				descriptorInfo.offset = 0;
				descriptorInfo.range = range;

				return descriptorInfo;
			}

			/**
			 * @brief Constructs a host buffer visible by the CPU.
			 * @param device A reference to a device smart pointer.
			 * @param createFlags The createInfo flags.
			 * @param size The size in bytes.
			 * @param usageFlags The buffer usage flags.
			 * @return Buffer
			 */
			[[nodiscard]]
			static
			Buffer
			createHostBuffer (const std::shared_ptr< Device > & device, VkBufferCreateFlags createFlags, VkDeviceSize size, VkBufferUsageFlags usageFlags) noexcept
			{
				return {device, createFlags, size, usageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
			}

			/**
			 * @brief Constructs a device buffer visible only by the GPU.
			 * @param device A reference to a device smart pointer.
			 * @param createFlags The createInfo flags.
			 * @param size The size in bytes.
			 * @param usageFlags The buffer usage flags.
			 * @return Buffer
			 */
			[[nodiscard]]
			static
			Buffer
			createDeviceBuffer (const std::shared_ptr< Device > & device, VkBufferCreateFlags createFlags, VkDeviceSize size, VkBufferUsageFlags usageFlags) noexcept
			{
				return {device, createFlags, size, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
			}

		private:

			VkBuffer m_handle{VK_NULL_HANDLE};
			VkBufferCreateInfo m_createInfo{};
			VkMemoryPropertyFlags m_memoryPropertyFlag;
			std::unique_ptr< DeviceMemory > m_deviceMemory;
			mutable std::mutex m_hostMemoryAccess;
	};
}
