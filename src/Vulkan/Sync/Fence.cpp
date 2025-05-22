/*
 * src/Vulkan/Sync/Fence.cpp
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

#include "Fence.hpp"

/* Local inclusions. */
#include "Vulkan/Device.hpp"
#include "Vulkan/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan::Sync
{
	const char *
	to_cstring (FenceStatus value) noexcept
	{
		switch ( value )
		{
			case FenceStatus::Ready :
				return ReadyString;

			case FenceStatus::Busy :
				return BusyString;

			case FenceStatus::Error :
				return ErrorString;

			default:
				return "Unknown";
		}
	}

	Fence::Fence (const std::shared_ptr< Device > & device, VkFenceCreateFlags createFlags) noexcept
		: AbstractDeviceDependentObject(device)
	{
		m_createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = createFlags; /* NOTE: Avoid the first use blocks everything ... */
	}

	Fence::Fence (const std::shared_ptr< Device > & device, const VkFenceCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(device), m_createInfo(createInfo)
	{

	}

	Fence::~Fence ()
	{
		this->destroyFromHardware();
	}

	bool
	Fence::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this fence !");

			return false;
		}

		const auto result = vkCreateFence(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a fence : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Fence::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the fence " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if (  m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a fence");

			vkDestroyFence(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	bool
	Fence::reset () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to reset this fence !");

			return false;
		}

		const auto result = vkResetFences(this->device()->handle(), 1, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the fence : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Fence::signal () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to signal this fence !");

			return false;
		}

		const auto result = vkResetFences(this->device()->handle(), 1, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the fence : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	FenceStatus
	Fence::getStatus () const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to get status from this fence !");

			return FenceStatus::Error;
		}

		const auto result = vkGetFenceStatus(this->device()->handle(), m_handle);

		switch ( result )
		{
			case VK_SUCCESS :
				return FenceStatus::Ready;

			case VK_NOT_READY :
				return FenceStatus::Busy;

			case VK_ERROR_DEVICE_LOST :
			default :
				return FenceStatus::Error;
		}
	}

	bool
	Fence::wait (uint64_t timeout) const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to use this fence !");

			return false;
		}

		const auto result = vkWaitForFences(
			this->device()->handle(),
			1, &m_handle,
			VK_FALSE,
			timeout
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait the fence : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Fence::waitAndReset (uint64_t timeout) const noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to use this fence !");

			return false;
		}

		auto result = vkWaitForFences(
			this->device()->handle(),
			1, &m_handle,
			VK_FALSE,
			timeout
		);

		switch ( result )
		{
			case VK_SUCCESS :
				break;

			case VK_TIMEOUT :
				TraceError{ClassId} << "The fence exceeded the waiting timeout (" << timeout << " ns) !";
				break;

			default:
				TraceError{ClassId} << "Unable to wait the fence : " << vkResultToCString(result) << " !";

				return false;
		}

		result = vkResetFences(this->device()->handle(), 1, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the fence : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Fence::waitForAll (const std::shared_ptr< Device > & device, const std::vector< VkFence > & fences, uint64_t timeout) noexcept
	{
		const auto result = vkWaitForFences(
			device->handle(),
			static_cast< uint32_t >(fences.size()), fences.data(),
			VK_TRUE,
			timeout
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait fences : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	VkFence
	Fence::waitForAnyOf (const std::shared_ptr< Device > & device, const std::vector< VkFence > & fences, uint64_t timeout) noexcept
	{
		const auto result = vkWaitForFences(
			device->handle(),
			static_cast< uint32_t >(fences.size()), fences.data(),
			VK_FALSE,
			timeout
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait fences : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		for ( auto * fence : fences )
		{
			if ( vkGetFenceStatus(device->handle(), fence) == VK_SUCCESS )
			{
				return fence;
			}
		}

		return VK_NULL_HANDLE;
	}
}
