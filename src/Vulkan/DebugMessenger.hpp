/*
 * src/Vulkan/DebugMessenger.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Instance;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Holds the debug messenger extension from the Vulkan API.
	 * @extends EmEn::Vulkan::AbstractObject Do not need a device to works.
	 */
	class DebugMessenger final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDebugMessenger"};

			/**
			 * @brief Constructs the Vulkan debugger.
			 * @param instance A reference to a vulkan instance.
			 */
			explicit DebugMessenger (const Instance & instance) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			DebugMessenger (const DebugMessenger & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			DebugMessenger (DebugMessenger && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			DebugMessenger & operator= (const DebugMessenger & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			DebugMessenger & operator= (DebugMessenger && copy) noexcept = delete;

			/**
			 * @brief Destructs the Vulkan debugger.
			 */
			~DebugMessenger () override;

			/**
			 * @brief Returns a createInfo for debugging.
			 * @return VkDebugUtilsMessengerEXT
			 */
			[[nodiscard]]
			VkDebugUtilsMessengerEXT
			handle () noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns a createInfo for debugging.
			 * @return const VkDebugUtilsMessengerCreateInfoEXT &
			 */
			[[nodiscard]]
			const VkDebugUtilsMessengerCreateInfoEXT &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief getCreateInfo
			 * @return VkDebugUtilsMessengerCreateInfoEXT
			 */
			[[nodiscard]]
			static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo () noexcept;

		private:

			/**
			 * @brief Debug callback signature method for Vulkan messenger.
			 * @param messageSeverity
			 * @param messageType
			 * @param pCallbackData
			 * @param pUserData
			 * @return VkBool32
			 */
			static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData) noexcept;

			const Instance & m_instance;
			VkDebugUtilsMessengerEXT m_handle{VK_NULL_HANDLE};
			VkDebugUtilsMessengerCreateInfoEXT m_createInfo{};
	};
}
