/*
 * src/Vulkan/Queue.cpp
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

#include "Queue.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "CommandBuffer.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	Queue::submit (const CommandBuffer & commandBuffer) const noexcept
	{
		VkCommandBuffer commandBufferHandle = commandBuffer.handle();

		const VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = VK_NULL_HANDLE,
			.pWaitDstStageMask = VK_NULL_HANDLE,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBufferHandle,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = VK_NULL_HANDLE,
		};

		/* [VULKAN-CPU-SYNC] vkQueueSubmit() */
		const std::lock_guard< Device > lock{*m_device};

		if ( const auto result = vkQueueSubmit(m_handle, 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to submit work into the queue : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

    bool
	Queue::submit (const CommandBuffer & commandBuffer, const SynchInfo & synchInfo) const noexcept
    {
        if ( synchInfo.waitSemaphores.size() != synchInfo.waitStages.size() )
        {
        	Tracer::error(ClassId, "Wait semaphore count must equal wait stage count!");

            return false;
        }

        VkCommandBuffer commandBufferHandle = commandBuffer.handle();

        const VkSubmitInfo submitInfo
		{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast< uint32_t >(synchInfo.waitSemaphores.size()),
            .pWaitSemaphores = synchInfo.waitSemaphores.data(),
            .pWaitDstStageMask = synchInfo.waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBufferHandle,
            .signalSemaphoreCount = static_cast< uint32_t >(synchInfo.signalSemaphores.size()),
            .pSignalSemaphores = synchInfo.signalSemaphores.data(),
        };

		/* [VULKAN-CPU-SYNC] vkQueueSubmit() */
		const std::lock_guard< Device > lock{*m_device};

		if ( const auto result = vkQueueSubmit(m_handle, 1, &submitInfo, synchInfo.fence); result != VK_SUCCESS )
        {
        	TraceError{ClassId} << "Unable to submit work into the queue : " << vkResultToCString(result) << " !";

            return false;
        }

        return true;
    }

	bool
	Queue::present (const VkPresentInfoKHR * presentInfo, std::atomic<Status> & swapChainStatus) const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkQueuePresentKHR() */
		const std::lock_guard< Device > lock{*m_device};

		switch ( const auto result = vkQueuePresentKHR(m_handle, presentInfo) )
		{
			case VK_SUCCESS :
				return true;

			case VK_SUBOPTIMAL_KHR :
				Tracer::debug(ClassId, "vkQueuePresentKHR() detected the swap-chain is 'sub-optimal'! [SWAP-CHAIN-RECREATION-PLANNED]");

				swapChainStatus = Status::Degraded;

				return true;

			case VK_ERROR_OUT_OF_DATE_KHR :
				Tracer::debug(ClassId, "vkQueuePresentKHR() detected the swap-chain is 'out of date' by the system! [SWAP-CHAIN-RECREATION-PLANNED]");

				swapChainStatus = Status::Degraded;

				return false;

			case VK_ERROR_DEVICE_LOST :
			case VK_ERROR_SURFACE_LOST_KHR :
			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT :
			default :
				TraceError{ClassId} << "Unable to present an image : " << vkResultToCString(result) << " !";

				swapChainStatus = Status::Failure;

				return false;
		}
	}

	bool
	Queue::waitIdle () const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkQueueWaitIdle() */
		const std::lock_guard< Device > lock{*m_device};

		if ( const auto result = vkQueueWaitIdle(m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait the queue to complete : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
