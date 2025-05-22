/*
 * src/Vulkan/Sync/Semaphore.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan::Sync
{
	/**
	 * @brief The synchronization semaphore class. Semaphores are designed for GPU tasks synchronization.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Semaphore final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSemaphore"};

			/**
			 * @brief Constructs semaphore.
			 * @param device A reference to a smart pointer of the device.
			 * @param createFlags The createInfo flags. Default none.
			 */
			explicit Semaphore (const std::shared_ptr< Device > & device, VkSemaphoreCreateFlags createFlags = 0) noexcept;

			/**
			 * @brief Constructs a semaphore with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 */
			Semaphore (const std::shared_ptr< Device > & device, const VkSemaphoreCreateInfo & createInfo) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Semaphore (const Semaphore & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Semaphore (Semaphore && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Semaphore & operator= (const Semaphore & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Semaphore & operator= (Semaphore && copy) noexcept = default;

			/**
			 * @brief Destructs the semaphore.
			 */
			~Semaphore () override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the semaphore vulkan handle.
			 * @return VkSemaphore
			 */
			[[nodiscard]]
			VkSemaphore
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the semaphore createInfo.
			 * @return const VkSemaphoreCreateInfo &
			 */
			[[nodiscard]]
			const VkSemaphoreCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

		private:

			VkSemaphore m_handle{VK_NULL_HANDLE};
			VkSemaphoreCreateInfo m_createInfo{};
	};
}
