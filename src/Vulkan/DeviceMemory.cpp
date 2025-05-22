/*
 * src/Vulkan/DeviceMemory.cpp
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

#include "DeviceMemory.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	DeviceMemory::DeviceMemory (const std::shared_ptr< Device > & device, const VkMemoryRequirements & memoryRequirement, VkMemoryPropertyFlags memoryPropertyFlags) noexcept
		: AbstractDeviceDependentObject(device), m_memoryRequirement(memoryRequirement), m_memoryPropertyFlags(memoryPropertyFlags)
	{

	}

	DeviceMemory::~DeviceMemory ()
	{
		this->destroyFromHardware();
	}

	bool
	DeviceMemory::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this device memory !");

			return false;
		}

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = m_memoryRequirement.size;
		allocateInfo.memoryTypeIndex = this->device()->findMemoryType(m_memoryRequirement.memoryTypeBits, m_memoryPropertyFlags);

		const auto result = vkAllocateMemory(this->device()->handle(), &allocateInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} <<
				"Unable to allocate " << m_memoryRequirement.size << " bytes (alignment:" << m_memoryRequirement.alignment << " bytes) "
				"in the device memory " << m_handle << " : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	DeviceMemory::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the device memory " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a device memory");

			vkFreeMemory(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	void *
	DeviceMemory::mapMemory (VkDeviceSize offset, VkDeviceSize size) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->hasDevice() )
			{
				Tracer::error(ClassId, "There is no device !");

				return nullptr;
			}

			if ( m_handle == VK_NULL_HANDLE )
			{
				Tracer::error(ClassId, "The device memory is not allocated !");

				return nullptr;
			}
		}

		void * pointer = nullptr;

		const auto result = vkMapMemory(this->device()->handle(), m_handle, offset, size, 0, &pointer);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to map the device memory : " << vkResultToCString(result) << " !";

			return nullptr;
		}

		return pointer;
	}

	void
	DeviceMemory::unmapMemory () const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->hasDevice() )
			{
				Tracer::error(ClassId, "There is no device !");

				return;
			}

			if ( m_handle == VK_NULL_HANDLE )
			{
				Tracer::error(ClassId, "The device memory is not allocated !");

				return;
			}
		}

		vkUnmapMemory(this->device()->handle(), m_handle);
	}
}
