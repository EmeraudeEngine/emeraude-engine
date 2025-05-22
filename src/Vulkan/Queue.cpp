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
#include "Sync/Fence.hpp"
#include "Sync/Semaphore.hpp"
#include "CommandBuffer.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;

	std::mutex Queue::s_mutex{};

	bool
	Queue::submit (const VkSubmitInfo & submitInfo, VkFence fence) const noexcept
	{
		const std::lock_guard< std::mutex > lock{s_mutex};

		const auto result = vkQueueSubmit(
			m_handle,
			1, &submitInfo,
			fence
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to submit work into the queue : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence) const noexcept
	{
		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		if ( !waitSemaphores.empty() )
		{
			submitInfo.waitSemaphoreCount = static_cast< uint32_t >(waitSemaphores.size());
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.pWaitDstStageMask = &waitStageFlags;
		}
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		if ( !signalSemaphores.empty() )
		{
			submitInfo.signalSemaphoreCount = static_cast< uint32_t >(signalSemaphores.size());
			submitInfo.pSignalSemaphores = signalSemaphores.data();
		}

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, VkSemaphore signalSemaphore, VkFence fence) const noexcept
	{
		if ( waitSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to wait is a null pointer ! (1)");

			return false;
		}

		if ( signalSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to signal is a null pointer ! (1)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence) const noexcept
	{
		if ( waitSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to wait is a null pointer ! (2)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		if ( !signalSemaphores.empty() )
		{
			submitInfo.signalSemaphoreCount = static_cast< uint32_t >(signalSemaphores.size());
			submitInfo.pSignalSemaphores = signalSemaphores.data();
		}

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, VkSemaphore signalSemaphore, VkFence fence) const noexcept
	{
		if ( signalSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to signal is a null pointer ! (3)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		if ( !waitSemaphores.empty() )
		{
			submitInfo.waitSemaphoreCount = static_cast< uint32_t >(waitSemaphores.size());
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.pWaitDstStageMask = &waitStageFlags;
		}
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, VkFence fence) const noexcept
	{
		if ( waitSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to wait is a null pointer ! (4)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitStageFlags;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		//submitInfo.signalSemaphoreCount = 0;
		//submitInfo.pSignalSemaphores = nullptr;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, VkFence fence) const noexcept
	{
		if ( waitSemaphores.empty() )
		{
			Tracer::warning(ClassId, "No semaphore to wait ! (5)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		if ( !waitSemaphores.empty() )
		{
			submitInfo.waitSemaphoreCount = static_cast< uint32_t >(waitSemaphores.size());
			submitInfo.pWaitSemaphores = waitSemaphores.data();
			submitInfo.pWaitDstStageMask = &waitStageFlags;
		}
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		//submitInfo.signalSemaphoreCount = 0;
		//submitInfo.pSignalSemaphores = nullptr;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore signalSemaphore, VkFence fence) const noexcept
	{
		if ( signalSemaphore == VK_NULL_HANDLE )
		{
			Tracer::warning(ClassId, "The semaphore to signal is a null pointer ! (6)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		//submitInfo.waitSemaphoreCount = 0;
		//submitInfo.pWaitSemaphores = nullptr;
		//submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence) const noexcept
	{
		if ( signalSemaphores.empty() )
		{
			Tracer::warning(ClassId, "No semaphore to wait ! (7)");

			return false;
		}

		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		//submitInfo.waitSemaphoreCount = 0;
		//submitInfo.pWaitSemaphores = nullptr;
		//submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = static_cast< uint32_t >(signalSemaphores.size());
		submitInfo.pSignalSemaphores = signalSemaphores.data();

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::submit (const std::shared_ptr< CommandBuffer > & buffer, VkFence fence) const noexcept
	{
		VkCommandBuffer commandBufferHandle = buffer->handle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.pNext = nullptr;
		//submitInfo.waitSemaphoreCount = 0;
		//submitInfo.pWaitSemaphores = nullptr;
		//submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		//submitInfo.signalSemaphoreCount = 0;
		//submitInfo.pSignalSemaphores = nullptr;

		return this->submit(submitInfo, fence);
	}

	bool
	Queue::present (const VkPresentInfoKHR * presentInfo, bool & swapChainRecreationNeeded) const noexcept
	{
		//const std::lock_guard< std::mutex > lock{s_mutex};

		switch ( vkQueuePresentKHR(m_handle, presentInfo) )
		{
			case VK_SUCCESS :
				return true;

			case VK_ERROR_OUT_OF_DATE_KHR :
				Tracer::info(ClassId, "VK_ERROR_OUT_OF_DATE_KHR @ presentation !");

				swapChainRecreationNeeded = true;

				return false;

			case VK_SUBOPTIMAL_KHR :
				Tracer::info(ClassId, "VK_SUBOPTIMAL_KHR @ presentation !");

				swapChainRecreationNeeded = true;

				return false;

			default :
				Tracer::error(ClassId, "Unable to present an image !");

				return false;
		}
	}

	bool
	Queue::waitIdle () const noexcept
	{
		const auto result = vkQueueWaitIdle(m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to wait the queue to complete : " << vkResultToCString(result) << " !";

			return false;
		}

		return true;
	}
}
