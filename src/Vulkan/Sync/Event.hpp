/*
 * src/Vulkan/Sync/Event.hpp
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

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractDeviceDependentObject.hpp"

namespace EmEn::Vulkan::Sync
{
	enum class EventStatus : uint8_t
	{
		Set,
		Reset,
		Error
	};

	static constexpr auto SetString{"Set"};
	static constexpr auto ResetString{"Reset"};
	//static constexpr auto ErrorString{"Error"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param value The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (EventStatus value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (EventStatus value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief The synchronization event class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Event final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanEvent"};

			/**
			 * @brief Constructs an event.
			 * @param device A reference to a smart pointer of the device.
			 * @param createFlags The createInfo flags. Default none.
			 */
			explicit
			Event (const std::shared_ptr< Device > & device, VkEventCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{device}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
			}

			/**
			 * @brief Constructs an event with a createInfo.
			 * @param device A reference to a smart pointer of the device.
			 * @param createInfo A reference to the createInfo.
			 */
			Event (const std::shared_ptr< Device > & device, const VkEventCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Event (const Event & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Event (Event && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Event & operator= (const Event & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Event & operator= (Event && copy) noexcept = default;

			/**
			 * @brief Destructs the event.
			 */
			~Event () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the event vulkan handle.
			 * @return VkEvent
			 */
			[[nodiscard]]
			VkEvent
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the event createInfo.
			 * @return const VkEventCreateInfo &
			 */
			[[nodiscard]]
			const VkEventCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the event status.
			 * @return Status
			 */
			[[nodiscard]]
			EventStatus getStatus () const noexcept;

			/**
			 * @brief Sets the event status.
			 * @return bool
			 */
			[[nodiscard]]
			bool set () const noexcept;

			/**
			 * @brief Reset the event status.
			 * @return bool
			 */
			[[nodiscard]]
			bool reset () const noexcept;

		private:

			VkEvent m_handle{VK_NULL_HANDLE};
			VkEventCreateInfo m_createInfo{};
	};
}
