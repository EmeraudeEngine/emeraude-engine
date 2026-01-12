/*
 * src/Vulkan/CommandPool.cpp
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

#include "CommandPool.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	CommandPool::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this command pool !");

			return false;
		}

		if ( const auto result = vkCreateCommandPool(this->device()->handle(), &m_createInfo, nullptr, &m_handle); result != VK_SUCCESS )
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
			vkDestroyCommandPool(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	VkCommandBuffer
	CommandPool::allocateCommandBuffer (bool primaryLevel) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::fatal(ClassId, "The command pool is not created! Unable to allocate command buffer.");

				return VK_NULL_HANDLE;
			}
		}

		/* [VULKAN-CPU-SYNC] vkAllocateCommandBuffers() */
		const std::lock_guard< std::mutex > lock{m_commandPoolAccess};

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_handle;
		allocateInfo.level = primaryLevel ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBufferHandle = VK_NULL_HANDLE;

		if ( const auto result = vkAllocateCommandBuffers(this->device()->handle(), &allocateInfo, &commandBufferHandle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to allocate a command buffer : " << vkResultToCString(result) << " !";

			return VK_NULL_HANDLE;
		}

		return commandBufferHandle;
	}

	void
	CommandPool::freeCommandBuffer (VkCommandBuffer commandBufferHandle) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::fatal(ClassId, "The command pool is not created! Unable to free command buffer.");

				return;
			}
		}

		/* [VULKAN-CPU-SYNC] vkFreeCommandBuffers() */
		const std::lock_guard< std::mutex > lock{m_commandPoolAccess};

		if constexpr ( IsDebug )
		{
			if ( commandBufferHandle == VK_NULL_HANDLE )
			{
				Tracer::fatal(ClassId, "Trying to free a command buffer with a null pointer handle.");

				return;
			}
		}

		vkFreeCommandBuffers(this->device()->handle(), m_handle, 1, &commandBufferHandle);
	}

	bool
	CommandPool::resetCommandBuffers (bool releaseMemory) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !this->isCreated() )
			{
				Tracer::fatal(ClassId, "The command pool is not created! Unable to reset this command pool.");

				return false;
			}
		}

		/* [VULKAN-CPU-SYNC] vkResetCommandPool() */
		const std::lock_guard< std::mutex > lock{m_commandPoolAccess};

		if ( const auto result = vkResetCommandPool(this->device()->handle(), m_handle, releaseMemory ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to reset the command pool : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
