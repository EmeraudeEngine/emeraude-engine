/*
 * src/Vulkan/AccelerationStructure.cpp
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

#include "AccelerationStructure.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	AccelerationStructure::AccelerationStructure (AccelerationStructure && other) noexcept
		: AbstractDeviceDependentObject{other.device()},
		m_handle{other.m_handle},
		m_type{other.m_type},
		m_size{other.m_size},
		m_deviceAddress{other.m_deviceAddress},
		m_fpDestroyAccelerationStructure{other.m_fpDestroyAccelerationStructure},
		m_backingBuffer{std::move(other.m_backingBuffer)}
	{
		other.m_handle = VK_NULL_HANDLE;
		other.m_deviceAddress = 0;
		other.m_fpDestroyAccelerationStructure = nullptr;

		if ( other.isCreated() )
		{
			this->setCreated();

			other.setDestroyed();
		}
	}

	AccelerationStructure &
	AccelerationStructure::operator= (AccelerationStructure && other) noexcept
	{
		if ( this != &other )
		{
			this->destroyFromHardware();

			this->setDeviceForMove(other.device());
			m_handle = other.m_handle;
			m_type = other.m_type;
			m_size = other.m_size;
			m_deviceAddress = other.m_deviceAddress;
			m_fpDestroyAccelerationStructure = other.m_fpDestroyAccelerationStructure;
			m_backingBuffer = std::move(other.m_backingBuffer);

			other.m_handle = VK_NULL_HANDLE;
			other.m_deviceAddress = 0;
			other.m_fpDestroyAccelerationStructure = nullptr;

			if ( other.isCreated() )
			{
				this->setCreated();

				other.setDestroyed();
			}
		}

		return *this;
	}

	bool
	AccelerationStructure::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this acceleration structure !");

			return false;
		}

		/* 1. Create the backing buffer for the acceleration structure. */
		if ( !m_backingBuffer.createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the backing buffer (" << m_size << " bytes) !";

			return false;
		}

		/* 2. Load extension function pointers. */
		const auto deviceHandle = this->device()->handle();

		auto fpCreateAccelerationStructure = reinterpret_cast< PFN_vkCreateAccelerationStructureKHR >(vkGetDeviceProcAddr(deviceHandle, "vkCreateAccelerationStructureKHR"));
		auto fpGetAccelerationStructureDeviceAddress = reinterpret_cast< PFN_vkGetAccelerationStructureDeviceAddressKHR >(vkGetDeviceProcAddr(deviceHandle, "vkGetAccelerationStructureDeviceAddressKHR"));
		m_fpDestroyAccelerationStructure = reinterpret_cast< PFN_vkDestroyAccelerationStructureKHR >(vkGetDeviceProcAddr(deviceHandle, "vkDestroyAccelerationStructureKHR"));

		if ( fpCreateAccelerationStructure == nullptr || fpGetAccelerationStructureDeviceAddress == nullptr || m_fpDestroyAccelerationStructure == nullptr )
		{
			Tracer::error(ClassId, "Unable to load acceleration structure extension functions !");

			this->destroyFromHardware();

			return false;
		}

		/* 3. Create the acceleration structure on the backing buffer. */
		VkAccelerationStructureCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.createFlags = 0;
		createInfo.buffer = m_backingBuffer.handle();
		createInfo.offset = 0;
		createInfo.size = m_size;
		createInfo.type = m_type;
		createInfo.deviceAddress = 0;

		if ( const auto result = fpCreateAccelerationStructure(deviceHandle, &createInfo, VK_NULL_HANDLE, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create an acceleration structure : " << vkResultToCString(result) << " !";

			this->destroyFromHardware();

			return false;
		}

		/* 4. Query the device address for use in TLAS instance references and shader bindings. */
		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.pNext = nullptr;
		addressInfo.accelerationStructure = m_handle;

		m_deviceAddress = fpGetAccelerationStructureDeviceAddress(deviceHandle, &addressInfo);

		this->setCreated();

		return true;
	}

	bool
	AccelerationStructure::destroyFromHardware () noexcept
	{
		/* First, destroy the acceleration structure. */
		if ( m_handle != VK_NULL_HANDLE && m_fpDestroyAccelerationStructure != nullptr )
		{
			m_fpDestroyAccelerationStructure(this->device()->handle(), m_handle, VK_NULL_HANDLE);

			m_handle = VK_NULL_HANDLE;
			m_deviceAddress = 0;
		}

		/* Then, destroy the backing buffer. */
		m_backingBuffer.destroyFromHardware();

		this->setDestroyed();

		return true;
	}
}
