/*
 * src/Vulkan/Buffer.cpp
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

#include "Buffer.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "TransferManager.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	Buffer::Buffer (Buffer && other) noexcept
		: AbstractDeviceDependentObject{other.device()},
		m_handle{other.m_handle},
		m_createInfo{other.m_createInfo},
		m_deviceMemory{std::move(other.m_deviceMemory)},
		m_memoryAllocation{other.m_memoryAllocation},
		m_hostVisible{other.m_hostVisible}
	{
		other.m_handle = VK_NULL_HANDLE;

		if ( other.isCreated() )
		{
			this->setCreated();

			other.setDestroyed();
		}
	}

	Buffer &
	Buffer::operator= (Buffer && other) noexcept
	{
		if ( this != &other )
		{
			this->destroyFromHardware();

			this->setDeviceForMove(other.device());
			m_handle = other.m_handle;
			m_createInfo = other.m_createInfo;
			m_deviceMemory = std::move(other.m_deviceMemory);
			m_memoryAllocation = other.m_memoryAllocation;
			m_hostVisible = other.m_hostVisible;

			other.m_handle = VK_NULL_HANDLE;

			if ( other.isCreated() )
			{
				this->setCreated();

				other.setDestroyed();
			}
		}

		return *this;
	}

	bool
	Buffer::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this buffer !");

			return false;
		}

		const auto result =
			this->device()->useMemoryAllocator() ?
			this->createWithVMA() :
			this->createManually();

		if ( !result )
		{
			this->destroyFromHardware();

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Buffer::destroyFromHardware () noexcept
	{
		const auto result =
			this->device()->useMemoryAllocator() ?
			this->destroyWithVMA() :
			this->destroyManually();

		if ( !result )
		{
			return false;
		}

		this->setDestroyed();

		return true;
	}

	bool
	Buffer::createManually () noexcept
	{
		/* 1. Create the buffer. */
		if ( const auto result = vkCreateBuffer(this->device()->handle(), &m_createInfo, VK_NULL_HANDLE, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a buffer : " << vkResultToCString(result) << " !";

			return false;
		}

		/* 2. Allocate memory for the new buffer. */
		VkBufferMemoryRequirementsInfo2 info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
		info.pNext = VK_NULL_HANDLE;
		info.buffer = m_handle;

		VkMemoryRequirements2 memoryRequirement{};
		memoryRequirement.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirement.pNext = VK_NULL_HANDLE;

		vkGetBufferMemoryRequirements2(this->device()->handle(), &info, &memoryRequirement);

		VkMemoryPropertyFlags memoryPropertyFlags = m_hostVisible ?
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		m_deviceMemory = std::make_unique< DeviceMemory >(this->device(), memoryRequirement, memoryPropertyFlags);
		m_deviceMemory->setIdentifier(ClassId, this->identifier(), "DeviceMemory");

		if ( !m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a device memory for the buffer " << m_handle << " !";

			return false;
		}

		/* 3. Bind the buffer to the device memory. */
		if ( const auto result = vkBindBufferMemory(this->device()->handle(), m_handle, m_deviceMemory->handle(), 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the buffer " << m_handle << " to the device memory " << m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Buffer::destroyManually () noexcept
	{
		/* First, release memory. */
		if ( m_deviceMemory != nullptr )
		{
			m_deviceMemory.reset();
		}

		/* Then, destroy the buffer. */
		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyBuffer(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		return true;
	}

	bool
	Buffer::createWithVMA () noexcept
	{
		VmaAllocationCreateInfo allocInfo{};
		if ( m_hostVisible )
		{
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		//allocInfo.requiredFlags = 0;
		//allocInfo.preferredFlags = 0;
		//allocInfo.memoryTypeBits = 0;
		//allocInfo.pool = VK_NULL_HANDLE; /* Default pool. */
		//allocInfo.pUserData = nullptr;
		//allocInfo.priority = 0.5F;

		/* Bind the buffer to the device memory */
		if ( const auto result = vmaCreateBuffer(this->device()->memoryAllocatorHandle(), &m_createInfo, &allocInfo, &m_handle, &m_memoryAllocation, nullptr); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a buffer with VMA : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Buffer::destroyWithVMA () noexcept
	{
		if ( m_handle != VK_NULL_HANDLE )
		{
			vmaDestroyBuffer(this->device()->memoryAllocatorHandle(), m_handle, m_memoryAllocation);

			m_handle = VK_NULL_HANDLE;
		}

		return true;
	}

	void *
	Buffer::mapMemory (VkDeviceSize offset, VkDeviceSize size) const noexcept
	{
		if ( !this->isHostVisible() )
		{
			Tracer::error(ClassId, "This buffer is not host visible! You can't map it.");

			return nullptr;
		}

		if ( m_memoryAllocation != VK_NULL_HANDLE )
		{
			void * pointer = nullptr;

			if ( const auto result = vmaMapMemory(this->device()->memoryAllocatorHandle(), m_memoryAllocation, &pointer); result != VK_SUCCESS )
			{
				TraceError{ClassId} << "Unable to map (VMA) the buffer from offset " << offset << " for " << size << " bytes.";

				return nullptr;
			}

			return pointer;
		}

		return m_deviceMemory->mapMemory(offset, size);
	}

	void
	Buffer::unmapMemory (VkDeviceSize offset, VkDeviceSize size) const noexcept
	{
		if ( !this->isHostVisible() )
		{
			return;
		}

		if ( m_memoryAllocation != VK_NULL_HANDLE )
		{
			const auto allocator = this->device()->memoryAllocatorHandle();

			vmaFlushAllocation(allocator, m_memoryAllocation, offset, size);

			vmaUnmapMemory(allocator, m_memoryAllocation);
		}
		else
		{
			m_deviceMemory->unmapMemory();
		}
	}

	bool
	Buffer::transferData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		return transferManager.uploadBuffer(*this, memoryRegion.bytes(), [&memoryRegion] (const Buffer & stagingBuffer) {
			return stagingBuffer.writeData(memoryRegion);
		});
	}

	bool
	Buffer::writeData (const MemoryRegion & memoryRegion) const noexcept
	{
		if ( !this->isHostVisible() )
		{
			Tracer::error(ClassId, "This buffer is not host visible! You can't write data directly to it.");

			return false;
		}

		/* [VULKAN-CPU-SYNC] CHECK */
		const std::lock_guard< std::mutex > lock{m_hostMemoryAccess};

		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		if ( m_memoryAllocation != VK_NULL_HANDLE )
		{
			const auto allocator = this->device()->memoryAllocatorHandle();

			void * pointer = nullptr;

			if ( const auto result = vmaMapMemory(allocator, m_memoryAllocation, &pointer); result != VK_SUCCESS )
			{
				TraceError{ClassId} << "Unable to map (VMA) the buffer from offset " << memoryRegion.offset() << " for " << memoryRegion.bytes() << " bytes.";

				return false;
			}

			std::memcpy(static_cast< std::byte * >(pointer) + memoryRegion.offset(), memoryRegion.source(), memoryRegion.bytes());

			vmaFlushAllocation(allocator, m_memoryAllocation, memoryRegion.offset(), memoryRegion.bytes());

			vmaUnmapMemory(allocator, m_memoryAllocation);
		}
		else
		{
			auto * pointer = m_deviceMemory->mapMemory(memoryRegion.offset(), memoryRegion.bytes());

			if ( pointer == nullptr )
			{
				TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << memoryRegion.bytes() << " bytes.";

				return false;
			}

			std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

			m_deviceMemory->unmapMemory();
		}

		return true;
	}

	bool
	Buffer::writeData (const std::vector< MemoryRegion > & memoryRegions) noexcept
	{
		if ( !this->isHostVisible() )
		{
			Tracer::error(ClassId, "This buffer is not host visible! You can't write data directly to it.");

			return false;
		}

		// [VULKAN-CPU-SYNC] CHECK
		const std::lock_guard< std::mutex > lock{m_hostMemoryAccess};

		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		if ( memoryRegions.empty() )
		{
			Tracer::error(ClassId, "No memory region to write !");

			return false;
		}

		const auto allocator = this->device()->memoryAllocatorHandle();

		// TODO: Check for performance improvement on mapping the buffer once with larger boundaries.
		return std::ranges::all_of(memoryRegions, [&] (const auto & memoryRegion) {
			if ( m_memoryAllocation != VK_NULL_HANDLE )
			{
				void * pointer = nullptr;

				if ( const auto result = vmaMapMemory(allocator, m_memoryAllocation, &pointer); result != VK_SUCCESS )
				{
					TraceError{ClassId} << "Unable to map (VMA) the buffer from offset " << memoryRegion.offset() << " for " << this->bytes() << " bytes.";

					return false;
				}

				std::memcpy(static_cast< std::byte * >(pointer) + memoryRegion.offset(), memoryRegion.source(), memoryRegion.bytes());

				vmaFlushAllocation(allocator, m_memoryAllocation, memoryRegion.offset(), memoryRegion.bytes());

				vmaUnmapMemory(allocator, m_memoryAllocation);
			}
			else
			{
				auto * pointer = m_deviceMemory->mapMemory(memoryRegion.offset(), this->bytes());

				if ( pointer == nullptr )
				{
					TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << this->bytes() << " bytes.";

					return false;
				}

				std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

				m_deviceMemory->unmapMemory();
			}

			return true;
		});
	}
}
