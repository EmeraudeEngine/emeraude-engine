/*
 * src/Vulkan/Sync/Fence.hpp
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
#include <limits>
#include <vector>

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan::Sync
{
	/**
	 * @brief The fence status.
	 */
	enum class FenceStatus : uint8_t
	{
		Ready,
		Busy,
		Error
	};

	static constexpr auto ReadyString{"Ready"};
	static constexpr auto BusyString{"Busy"};
	//static constexpr auto ErrorString{"Error"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (FenceStatus value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (FenceStatus value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief The synchronization fence class. Fences are designed for GPU-to-CPU synchronization.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Fence final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanFence"};

			/**
			 * @brief Constructs a fence.
			 * @param device A reference to a smart pointer of the device.
			 * @param createFlags The createInfo flags. Default VK_FENCE_CREATE_SIGNALED_BIT.
			 */
			explicit Fence (const std::shared_ptr< Device > & device, VkFenceCreateFlags createFlags = VK_FENCE_CREATE_SIGNALED_BIT) noexcept;

			/**
			 * @brief Constructs an fence with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 */
			Fence (const std::shared_ptr< Device > & device, const VkFenceCreateInfo & createInfo) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Fence (const Fence & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Fence (Fence && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Fence & operator= (const Fence & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Fence & operator= (Fence && copy) noexcept = default;

			/**
			 * @brief Destructs the fence.
			 */
			~Fence () override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the fence vulkan handle.
			 * @return VkFence
			 */
			[[nodiscard]]
			VkFence
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the fence createInfo.
			 * @return const VkFenceCreateInfo &
			 */
			[[nodiscard]]
			const VkFenceCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Sets the fence as unsignaled.
			 * @return bool
			 */
			[[nodiscard]]
			bool reset () const noexcept;

			/**
			 * @brief Sets the fence as signaled.
			 * @return bool
			 */
			[[nodiscard]]
			bool signal () const noexcept;

			/**
			 * @brief Returns the fence status.
			 * @return Status
			 */
			[[nodiscard]]
			FenceStatus getStatus () const noexcept;

			/**
			 * @brief Waits for fence to be signaled.
			 * @param timeout A duration in nanoseconds for timeout. Default close to infinity (584 years).
			 * @return bool
			 */
			[[nodiscard]]
			bool wait (uint64_t timeout = std::numeric_limits< uint64_t >::max()) const noexcept;

			/**
			 * @brief Waits for a fence to be signaled and reset it.
			 * @param timeout A duration in nanoseconds for timeout. Default close to infinity (584 years).
			 * @return bool
			 */
			[[nodiscard]]
			bool waitAndReset (uint64_t timeout = std::numeric_limits< uint64_t >::max()) const noexcept;

			/**
			 * @brief Waits for all fences to be signaled.
			 * @warning All the fences must be created by the device passed in parameters.
			 * @param device A reference to the device smart pointer.
			 * @param fences A reference to a vector of fences.
			 * @param timeout A duration in nanoseconds for timeout. Default close to infinity (584 years).
			 * @return bool
			 */
			static bool waitForAll (const std::shared_ptr< Device > & device, const std::vector< VkFence > & fences, uint64_t timeout = std::numeric_limits< uint64_t >::max()) noexcept;

			/**
			 * @brief Waits for any of fences to be signaled and returns it.
			 * @param device A reference to the device smart pointer.
			 * @param fences A reference to a vector of fences.
			 * @param timeout A duration in nanoseconds for timeout. Default close to infinity (584 years).
			 * @return bool
			 */
			static VkFence waitForAnyOf (const std::shared_ptr< Device > & device, const std::vector< VkFence > & fences, uint64_t timeout = std::numeric_limits< uint64_t >::max()) noexcept;

		private:

			VkFence m_handle{VK_NULL_HANDLE};
			VkFenceCreateInfo m_createInfo{};
	};
}
