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
#include <memory>
#include <mutex>
#include <span>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"

namespace EmEn::Vulkan
{
	class Device;
	class CommandBuffer;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Structure for synchronization parameters for a queue submission.
	 */
	struct SynchInfo
	{
		/**
		 * @brief Adds a wait info structure for semaphores.
		 * @param semaphores A list of semaphore handles.
		 * @param stages A list of flag mask for stages to wait.
		 * @return SynchInfo &
		 */
		SynchInfo &
		waits (std::span< const VkSemaphore > semaphores, std::span< const VkPipelineStageFlags > stages)
		{
			waitSemaphores = semaphores;
			waitStages = stages;

			return *this;
		}

		/**
		 * @brief Adds a signal info structure for semaphores.
		 * @param semaphores A list of semaphore handles.
		 * @return SynchInfo &
		 */
		SynchInfo &
		signals (std::span< const VkSemaphore > semaphores)
		{
			signalSemaphores = semaphores;

			return *this;
		}

		/**
		 * @brief Adds a fence to signal.
		 * @param fenceHandle A fence handle.
		 * @return SynchInfo &
		 */
		SynchInfo &
		withFence (VkFence fenceHandle)
		{
			fence = fenceHandle;

			return *this;
		}

		std::span< const VkSemaphore > waitSemaphores;
		std::span< const VkPipelineStageFlags > waitStages;
		std::span< const VkSemaphore > signalSemaphores;
		VkFence fence{VK_NULL_HANDLE};
	};

	/**
	 * @brief Defines a device working queue.
	 * @extends EmEn::Vulkan::AbstractObject A queue is directly created by the device, so a simple object is perfect.
	 */
	class Queue final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanQueue"};

			static inline std::atomic_int s_queueInstanceCounter{0};

			/**
			 * @brief Constructs a device queue.
			 * @param queue The handle of the queue.
			 * @param familyQueueIndex Set which family queue is used to create this queue.
			 */
			Queue (VkQueue queue, uint32_t familyQueueIndex) noexcept
				: m_handle{queue},
				m_familyQueueIndex{familyQueueIndex}
			{
				const int count = ++s_queueInstanceCounter;

				TraceDebug{ClassId} << "New Queue created! Handle: " << static_cast< void * >(m_handle) << ". Instance count: " << count;

				this->setCreated();
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Queue (const Queue & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Queue (Queue && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Queue & operator= (const Queue & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Queue & operator= (Queue && copy) noexcept = delete;

			/**
			 * @brief Destructs the device queue.
			 */
			~Queue () override
			{
				this->setDestroyed();
			}

			/**
			 * @brief Returns this queue uses the queue family index of the physical device.
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
			 * @param commandBuffer A reference to a command buffer smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & commandBuffer) const noexcept;

			/**
			 * @brief Submits a command buffer to the queue with synchronization info.
			 * @param commandBuffer A reference to a command buffer smart pointer.
			 * @param synchInfo A reference to a syncInfo.
			 * @return bool
			 */
			[[nodiscard]]
			bool submit (const std::shared_ptr< CommandBuffer > & commandBuffer, const SynchInfo & synchInfo) const noexcept;

			/**
			 * @brief Submits a present info structure.
			 * @param presentInfo A pointer to a presentInfo structure.
			 * @param swapChainRecreationNeeded A bool to switch on bad presentation.
			 * @return bool
			 */
			bool present (const VkPresentInfoKHR * presentInfo, bool & swapChainRecreationNeeded) const noexcept;

			/**
			 * @brief Waits for the queue to complete work.
			 * @note Don't use this method of synchronization.
			 * @return void
			 */
			[[nodiscard]]
			bool waitIdle () const noexcept;

		private:

			VkQueue m_handle;
			uint32_t m_familyQueueIndex;
			mutable std::mutex m_queueAccess;
	};
}
