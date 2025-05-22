/*
 * src/Vulkan/Queue.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

namespace EmEn::Vulkan
{
	namespace Sync
	{
		class Fence;
		class Semaphore;
	}

	class Device;
	class CommandBuffer;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a device working queue.
	 * @extends EmEn::Vulkan::AbstractObject A queue is directly created by the device, so a simple object is perfect.
	 */
	class Queue final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanQueue"};

			/**
			 * @brief Constructs a device queue.
			 * @param queue The handle of the queue.
			 * @param familyQueueIndex Set which family queue is used to create this queue.
			 */
			Queue (VkQueue queue, uint32_t familyQueueIndex) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Queue (const Queue & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Queue (Queue && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Queue & operator= (const Queue & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Queue & operator= (Queue && copy) noexcept = default;

			/**
			 * @brief Destructs the device queue.
			 */
			~Queue () override;

			/**
			 * @brief Returns the vulkan queue handle.
			 * @return VkQueue
			 */
			[[nodiscard]]
			VkQueue
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the queue family index of the physical device is used by this queue.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			familyQueueIndex () const noexcept
			{
				return m_familyQueueIndex;
			}

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits multiple semaphores, signals multiple semaphores.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphores A reference to a vector of semaphore handles.
			 * @param waitStageFlags A mask for wait stage.
			 * @param signalSemaphores A reference to a vector of semaphore handles.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits one semaphore, signals one semaphore.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphore A semaphore handle.
			 * @param waitStageFlags A mask for wait stage.
			 * @param signalSemaphore A semaphore handle.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, VkSemaphore signalSemaphore, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits one semaphore, signals multiple semaphores.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphore A semaphore handle.
			 * @param waitStageFlags A mask for wait stage.
			 * @param signalSemaphores A reference to a vector of semaphore handles.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits multiple semaphores, signals one semaphore.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphores A reference to a vector of semaphore handles.
			 * @param waitStageFlags A mask for wait stage.
			 * @param signalSemaphore A semaphore handles.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, VkSemaphore signalSemaphore, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits one semaphore.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphore A semaphore handle.
			 * @param waitStageFlags A mask for wait stage.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore waitSemaphore, VkPipelineStageFlags waitStageFlags, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Waits multiple semaphores.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param waitSemaphores A reference to a vector of semaphore handles.
			 * @param waitStageFlags A mask for wait stage.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & waitSemaphores, VkPipelineStageFlags waitStageFlags, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Signals one semaphore.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param signalSemaphore A semaphore handle.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, VkSemaphore signalSemaphore, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note Signals multiple semaphores.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param signalSemaphores A reference to a vector of semaphore handles.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, const std::vector< VkSemaphore > & signalSemaphores, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue.
			 * @note No semaphore usage.
			 * @param buffer A reference to a command buffer smart pointer.
			 * @param fence A fence handle. Default None.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & buffer, VkFence fence = VK_NULL_HANDLE) const noexcept;

			/**
			 * @brief Submits a present info.
			 * @FIXME Bad design.
			 * @param presentInfo A pointer to a presentInfo structure.
			 * @param swapChainRecreationNeeded A bool to switch on bad presentation.
			 * @return bool
			 */
			bool present (const VkPresentInfoKHR * presentInfo, bool & swapChainRecreationNeeded) const noexcept;

			/**
			 * @brief Waits for the queue to complete work.
			 * @return void
			 */
			[[deprecated("Don't use this method of synchronization.")]]
			[[nodiscard]]
			bool waitIdle () const noexcept;

		private:

			/**
			 * @brief Submit work on GPU.
			 * @param submitInfo A reference to a submit info structure.
			 * @param fence A fence handle.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const VkSubmitInfo & submitInfo, VkFence fence) const noexcept;

			static std::mutex s_mutex;

			VkQueue m_handle;
			uint32_t m_familyQueueIndex;
	};
}
