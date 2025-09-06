/*
 * src/Vulkan/CommandPool.hpp
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

/* Local inclusions. */
#include "AbstractDeviceDependentObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The CommandPool class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This Vulkan object needs a device.
	 */
	class CommandPool final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanCommandPool"};

			/**
			 * @brief Constructs a command pool.
			 * @param device A reference to a smart pointer of a device.
			 * @param queueFamilyIndex Set which family queue will be used by the command pool.
			 * @param transientCB Tells command buffer will be short-lived.
			 * @param enableCBReset Enables the command buffer to be reset into initial state.
			 * @param enableProtectCB Enables the protected memory (Request protectedMemory feature and Vulkan 1.1).
			 */
			CommandPool (const std::shared_ptr< Device > & device, uint32_t queueFamilyIndex, bool transientCB, bool enableCBReset, bool enableProtectCB) noexcept
				: AbstractDeviceDependentObject{device}
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

			/**
			 * @brief Constructs a command pool with createInfo.
			 * @param device A reference to a smart pointer of a device.
			 * @param createInfo A reference to a createInfo.
			 */
			CommandPool (const std::shared_ptr< Device > & device, const VkCommandPoolCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			CommandPool (const CommandPool & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			CommandPool (CommandPool && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			CommandPool & operator= (const CommandPool & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			CommandPool & operator= (CommandPool && copy) noexcept = delete;

			/**
			 * @brief Destructs the command pool.
			 */
			~CommandPool () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the command pool vulkan handle.
			 * @return VkCommandPool
			 */
			[[nodiscard]]
			VkCommandPool
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the command pool createInfo.
			 * @return const VkCommandPoolCreateInfo &
			 */
			[[nodiscard]]
			const VkCommandPoolCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the queue family index used at creation.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			queueFamilyIndex () const noexcept
			{
				return m_createInfo.queueFamilyIndex;
			}

			/**
			 * @brief Allocates one command buffer from this pool.
			 * @param primaryLevel
			 * @return bool
			 */
			[[nodiscard]]
			VkCommandBuffer allocateCommandBuffer (bool primaryLevel) const noexcept;

			/**
			 * @brief Frees one command buffer.
			 * @param commandBufferHandle A command buffer handle.
			 * @return void
			 */
			void freeCommandBuffer (VkCommandBuffer commandBufferHandle) const noexcept;

		private:

			VkCommandPool m_handle{VK_NULL_HANDLE};
			VkCommandPoolCreateInfo m_createInfo{};
			mutable std::mutex m_allocationsAccess;
	};
}
