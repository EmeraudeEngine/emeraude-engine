/*
 * src/Vulkan/Buffer.cpp
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

#include "Buffer.hpp"

/* Local inclusions. */
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	Buffer::Buffer (Buffer && other) noexcept
		: AbstractDeviceDependentObject{other.device()},
		m_handle{other.m_handle},
		m_createInfo{other.m_createInfo},
		m_memoryPropertyFlag{other.m_memoryPropertyFlag},
		m_deviceMemory{std::move(other.m_deviceMemory)}
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
			m_memoryPropertyFlag = other.m_memoryPropertyFlag;
			m_deviceMemory = std::move(other.m_deviceMemory);

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

		if ( const auto result = vkCreateBuffer(this->device()->handle(), &m_createInfo, VK_NULL_HANDLE, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a buffer : " << vkResultToCString(result) << " !";

			return false;
		}

		/* Allocate memory for the new buffer. */
		VkBufferMemoryRequirementsInfo2 info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
		info.pNext = VK_NULL_HANDLE;
		info.buffer = m_handle;

		VkMemoryRequirements2 memoryRequirement{};
		memoryRequirement.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirement.pNext = VK_NULL_HANDLE;

		vkGetBufferMemoryRequirements2(this->device()->handle(), &info, &memoryRequirement);

		m_deviceMemory = std::make_unique< DeviceMemory >(this->device(), memoryRequirement, m_memoryPropertyFlag);
		m_deviceMemory->setIdentifier(ClassId, this->identifier(), "DeviceMemory");

		if ( !m_deviceMemory->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a device memory for the buffer " << m_handle << " !";

			this->destroyFromHardware();

			return false;
		}

		/* Bind the buffer to the device memory */
		if ( const auto result = vkBindBufferMemory(this->device()->handle(), m_handle, m_deviceMemory->handle(), 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to bind the buffer " << m_handle << " to the device memory " << m_deviceMemory->handle() <<
				" : " << vkResultToCString(result) << " !";

			this->destroyFromHardware();

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Buffer::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to destroy this buffer !");

			return false;
		}

		if ( m_deviceMemory != nullptr )
		{
			m_deviceMemory->destroyFromHardware();
			m_deviceMemory.reset();
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyBuffer(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	bool
	Buffer::transferData (TransferManager & transferManager, const MemoryRegion & memoryRegion) noexcept
	{
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "The buffer is not created ! Use one of the Buffer::create() methods first.");

			return false;
		}

		return transferManager.transferBuffer(*this, memoryRegion.bytes(), [&memoryRegion] (const Buffer & stagingBuffer) {
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

		/* Lock the buffer for access. */
		auto * pointer = m_deviceMemory->mapMemory(memoryRegion.offset(), memoryRegion.bytes());

		if ( pointer == nullptr )
		{
			TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << memoryRegion.bytes() << " bytes.";

			return false;
		}

		/* Raw data copy... */
		std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

		/* Unlock the buffer. */
		m_deviceMemory->unmapMemory();

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

		/* [VULKAN-CPU-SYNC] CHECK */
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

		/* TODO: Check for performance improvement on mapping the buffer once with larger boundaries. */

		/* Raw data copy... */
		return std::ranges::all_of(memoryRegions, [&] (const auto & memoryRegion) {
			/* Lock the buffer for access. */
			auto * pointer = m_deviceMemory->mapMemory(memoryRegion.offset(), this->bytes());

			if ( pointer == nullptr )
			{
				TraceError{ClassId} << "Unable to map the buffer from offset " << memoryRegion.offset() << " for " << this->bytes() << " bytes.";

				return false;
			}

			/* Raw data copy... */
			std::memcpy(pointer, memoryRegion.source(), memoryRegion.bytes());

			/* Unlock the buffer. */
			m_deviceMemory->unmapMemory();

			return true;
		});
	}
}
