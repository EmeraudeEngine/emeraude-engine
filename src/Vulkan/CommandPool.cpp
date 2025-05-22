/*
 * src/Vulkan/CommandPool.cpp
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

#include "CommandPool.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	CommandPool::CommandPool (const std::shared_ptr< Device > & device, uint32_t queueFamilyIndex, bool transientCB, bool enableCBReset, bool enableProtectCB) noexcept
		: AbstractDeviceDependentObject(device)
	{
		m_createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = 0;
		m_createInfo.queueFamilyIndex = queueFamilyIndex;

		if ( transientCB )
		{
			m_createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		}

		if ( enableCBReset )
		{
			m_createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		}

		if ( enableProtectCB )
		{
			m_createInfo.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
		}
	}

	CommandPool::CommandPool (const std::shared_ptr< Device > & device, const VkCommandPoolCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(device), m_createInfo(createInfo)
	{

	}

	CommandPool::~CommandPool ()
	{
		this->destroyFromHardware();
	}

	bool
	CommandPool::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this command pool !");

			return false;
		}

		const auto result = vkCreateCommandPool(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create command pool : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	CommandPool::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to destroy this command pool !");

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle("Destroying a command pool");

			vkDestroyCommandPool(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	VkCommandBuffer
	CommandPool::allocateCommandBuffer (bool primaryLevel) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_allocationMutex};

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_handle;
		allocateInfo.level = primaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBufferHandle = VK_NULL_HANDLE;

		const auto result = vkAllocateCommandBuffers(
			this->device()->handle(),
			&allocateInfo,
			&commandBufferHandle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to allocate a command buffer : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		return commandBufferHandle;
	}

	void
	CommandPool::freeCommandBuffer (VkCommandBuffer commandBufferHandle) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_allocationMutex};

		/* FIXME: This causes a VK_ERROR_DEVICE_LOST */
		this->device()->waitIdle("Freeing command buffer from a command pool");

		vkFreeCommandBuffers(
			this->device()->handle(),
			m_handle,
			1, &commandBufferHandle
		);
	}
}
