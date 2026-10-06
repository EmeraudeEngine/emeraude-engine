/*
 * src/Vulkan/Queue.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Queue.hpp"

/* Local inclusions. */
#include "StaticVector.hpp"
#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Vulkan
{
	using namespace Base;

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
		const std::scoped_lock lock{*m_device};

		if ( const auto result = vkQueueSubmit(m_handle, 1, &submitInfo, VK_NULL_HANDLE); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to submit work into the queue : " << vkResultToCString(result) << " !";

			if ( result == VK_ERROR_DEVICE_LOST )
			{
				m_device->dumpDeviceLostDiagnostics("Queue::submit");
			}

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

		const auto tracked = synchInfo.timelineValue != nullptr;

		if ( tracked )
		{
			*synchInfo.timelineValue = 0;

			if ( m_timeline == VK_NULL_HANDLE )
			{
				TraceError{ClassId} << "The queue '" << this->identifier() << "' has no timeline semaphore: a tracked submission is refused !";

				return false;
			}
		}

		/* NOTE: A tracked submission signals the caller's semaphores plus the queue timeline. The values of
		 * the binary semaphores are ignored by Vulkan but the arrays must have the same length. */
		Base::StaticVector< VkSemaphore, MaxTrackedSignalSemaphores > signalSemaphores;
		Base::StaticVector< uint64_t, MaxTrackedSignalSemaphores > signalValues;

		if ( tracked )
		{
			if ( synchInfo.signalSemaphores.size() >= MaxTrackedSignalSemaphores )
			{
				TraceError{ClassId} << "A tracked submission signals at most " << (MaxTrackedSignalSemaphores - 1) << " semaphores of its own !";

				return false;
			}

			for ( const auto semaphore : synchInfo.signalSemaphores )
			{
				signalSemaphores.push_back(semaphore);
				signalValues.push_back(0);
			}
		}

		VkCommandBuffer commandBufferHandle = commandBuffer.handle();

		VkTimelineSemaphoreSubmitInfo timelineInfo{};
		timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

		VkSubmitInfo submitInfo
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
		const std::scoped_lock lock{*m_device};

		/* NOTE: The value is taken under the lock that serializes every submission, so the timeline is
		 * signalled in increasing order on this queue. */
		const auto value = m_lastTimelineValue + 1;

		if ( tracked )
		{
			signalSemaphores.push_back(m_timeline);
			signalValues.push_back(value);

			timelineInfo.signalSemaphoreValueCount = static_cast< uint32_t >(signalValues.size());
			timelineInfo.pSignalSemaphoreValues = signalValues.data();

			submitInfo.pNext = &timelineInfo;
			submitInfo.signalSemaphoreCount = static_cast< uint32_t >(signalSemaphores.size());
			submitInfo.pSignalSemaphores = signalSemaphores.data();
		}

		if ( const auto result = vkQueueSubmit(m_handle, 1, &submitInfo, synchInfo.fence); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to submit work into the queue : " << vkResultToCString(result) << " !";

			if ( result == VK_ERROR_DEVICE_LOST )
			{
				m_device->dumpDeviceLostDiagnostics("Queue::submit");
			}

			return false;
		}

		if ( tracked )
		{
			m_lastTimelineValue = value;
			*synchInfo.timelineValue = value;
		}

		return true;
	}

	bool
	Queue::submit (const SynchInfo & synchInfo) const noexcept
	{
		if ( synchInfo.waitSemaphores.size() != synchInfo.waitStages.size() )
		{
			Tracer::error(ClassId, "Wait semaphore count must equal wait stage count!");

			return false;
		}

		const VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast< uint32_t >(synchInfo.waitSemaphores.size()),
			.pWaitSemaphores = synchInfo.waitSemaphores.data(),
			.pWaitDstStageMask = synchInfo.waitStages.data(),
			.commandBufferCount = 0,
			.pCommandBuffers = nullptr,
			.signalSemaphoreCount = static_cast< uint32_t >(synchInfo.signalSemaphores.size()),
			.pSignalSemaphores = synchInfo.signalSemaphores.data(),
		};

		/* [VULKAN-CPU-SYNC] vkQueueSubmit() */
		const std::scoped_lock lock{*m_device};

		if ( const auto result = vkQueueSubmit(m_handle, 1, &submitInfo, synchInfo.fence); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to submit synchronization into the queue : " << vkResultToCString(result) << " !";

			if ( result == VK_ERROR_DEVICE_LOST )
			{
				m_device->dumpDeviceLostDiagnostics("Queue::submit");
			}

			return false;
		}

		return true;
	}

	bool
	Queue::present (const VkPresentInfoKHR * presentInfo, std::atomic<SwapChainStatus> & swapChainStatus) const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkQueuePresentKHR() */
		const std::scoped_lock lock{*m_device};

		switch ( const auto result = vkQueuePresentKHR(m_handle, presentInfo) )
		{
			case VK_SUCCESS :
				return true;

			case VK_SUBOPTIMAL_KHR :
				Tracer::debug(ClassId, "vkQueuePresentKHR() detected the swap-chain is 'sub-optimal'! [SWAP-CHAIN-RECREATION-PLANNED]");

				swapChainStatus = SwapChainStatus::Degraded;

				return true;

			case VK_ERROR_OUT_OF_DATE_KHR :
				Tracer::debug(ClassId, "vkQueuePresentKHR() detected the swap-chain is 'out of date' by the system! [SWAP-CHAIN-RECREATION-PLANNED]");

				swapChainStatus = SwapChainStatus::Degraded;

				return false;

			case VK_ERROR_DEVICE_LOST :
			case VK_ERROR_SURFACE_LOST_KHR :
			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT :
			default :
				TraceError{ClassId} << "Unable to present an image : " << vkResultToCString(result) << " !";

				if ( const auto * hint = vkResultDiagnosticHint(result); hint != nullptr )
				{
					TraceWarning{ClassId} << hint;
				}

				if ( result == VK_ERROR_DEVICE_LOST )
				{
					m_device->dumpDeviceLostDiagnostics("Queue::present");
				}

				swapChainStatus = SwapChainStatus::Failure;

				return false;
		}
	}

	bool
	Queue::waitIdle () const noexcept
	{
		/* [VULKAN-CPU-SYNC] vkQueueWaitIdle() */
		const std::scoped_lock lock{*m_device};

		if ( const auto result = vkQueueWaitIdle(m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait the queue to complete : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
	bool
	Queue::createTimeline () noexcept
	{
		if ( m_timeline != VK_NULL_HANDLE )
		{
			return true;
		}

		VkSemaphoreTypeCreateInfo typeCreateInfo{};
		typeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		typeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeCreateInfo.initialValue = 0;

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		createInfo.pNext = &typeCreateInfo;

		if ( const auto result = vkCreateSemaphore(m_device->handle(), &createInfo, nullptr, &m_timeline); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create the timeline semaphore of the queue '" << this->identifier() << "' : " << vkResultToCString(result) << " !";

			m_timeline = VK_NULL_HANDLE;

			return false;
		}

		return true;
	}

	void
	Queue::destroyTimeline () noexcept
	{
		if ( m_timeline == VK_NULL_HANDLE || m_device == nullptr )
		{
			return;
		}

		vkDestroySemaphore(m_device->handle(), m_timeline, nullptr);

		m_timeline = VK_NULL_HANDLE;
	}

	bool
	Queue::isReached (uint64_t value) const noexcept
	{
		if ( value == 0 )
		{
			return true;
		}

		if ( m_timeline == VK_NULL_HANDLE )
		{
			return false;
		}

		uint64_t completedValue = 0;

		if ( const auto result = vkGetSemaphoreCounterValue(m_device->handle(), m_timeline, &completedValue); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to read the timeline of the queue '" << this->identifier() << "' : " << vkResultToCString(result) << " !";

			return false;
		}

		return completedValue >= value;
	}

	bool
	Queue::waitUntilReached (uint64_t value, uint64_t timeoutNanoseconds) const noexcept
	{
		if ( value == 0 )
		{
			return true;
		}

		if ( m_timeline == VK_NULL_HANDLE )
		{
			return false;
		}

		VkSemaphoreWaitInfo waitInfo{};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores = &m_timeline;
		waitInfo.pValues = &value;

		/* [VULKAN-CPU-SYNC] vkWaitSemaphores() */
		if ( const auto result = vkWaitSemaphores(m_device->handle(), &waitInfo, timeoutNanoseconds); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Waiting the timeline value " << value << " of the queue '" << this->identifier() << "' failed : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
