/*
 * src/Vulkan/DeviceRequirements.hpp
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
#include <sstream>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Window.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief This class describes the requirements to create a Vulkan logical device.
	 */
	class DeviceRequirements final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanDeviceRequirements"};

			/**
			 * @brief Constructs a device requirements.
			 * @param enableGraphics The device will be used for graphics.
			 * @param window A pointer to the window. This will enable the presentation request.
			 * @param enableCompute The device will be used for compute.
			 */
			explicit
			DeviceRequirements (bool enableGraphics, Window * window, bool enableCompute) noexcept
				: m_surface{enableGraphics && window != nullptr ? window->surface()->handle() : VK_NULL_HANDLE},
				m_enableGraphics{enableGraphics},
				m_enableCompute{enableCompute}
			{
				/* NOTE: Device features from Vulkan 1.3 API. */
				m_featuresVK13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
				m_featuresVK13.pNext = nullptr;
				/* NOTE: Device features from Vulkan 1.2 API. */
				m_featuresVK12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
				m_featuresVK12.pNext = &m_featuresVK13;
				/* NOTE: Device features from Vulkan 1.1 API. */
				m_featuresVK11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
				m_featuresVK11.pNext = &m_featuresVK12;
				/* NOTE: Device features from Vulkan 1.0 API. */
				m_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				m_features.pNext = &m_featuresVK11;
			}

			/**
			 * @brief Returns the physical device features.
			 * @return const VkPhysicalDeviceFeatures2 &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceFeatures2 &
			features () const noexcept
			{
				return m_features;
			}

			/**
			 * @brief Gives access to configure Vulkan 1.0 API device features.
			 * @return VkPhysicalDeviceFeatures &
			 */
			[[nodiscard]]
			VkPhysicalDeviceFeatures &
			featuresVK10 () noexcept
			{
				return m_features.features;
			}

			/**
			 * @brief Returns the Vulkan 1.0 API device features for the createInfo.
			 * @return const VkPhysicalDeviceFeatures &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceFeatures &
			featuresVK10 () const noexcept
			{
				return m_features.features;
			}

			/**
			 * @brief Gives access to configure Vulkan 1.1 API device features.
			 * @return VkPhysicalDeviceVulkan11Features &
			 */
			[[nodiscard]]
			VkPhysicalDeviceVulkan11Features &
			featuresVK11 () noexcept
			{
				return m_featuresVK11;
			}

			/**
			 * @brief Returns the Vulkan 1.1 API device features for the createInfo.
			 * @return const VkPhysicalDeviceVulkan11Features &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceVulkan11Features &
			featuresVK11 () const noexcept
			{
				return m_featuresVK11;
			}

			/**
			 * @brief Gives access to configure Vulkan 1.2 API device features.
			 * @return VkPhysicalDeviceVulkan11Features &
			 */
			[[nodiscard]]
			VkPhysicalDeviceVulkan12Features &
			featuresVK12 () noexcept
			{
				return m_featuresVK12;
			}

			/**
			 * @brief Returns the Vulkan 1.2 API device features for the createInfo.
			 * @return const VkPhysicalDeviceVulkan11Features &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceVulkan12Features &
			featuresVK12 () const noexcept
			{
				return m_featuresVK12;
			}

			/**
			 * @brief Gives access to configure Vulkan 1.3 API device features.
			 * @return VkPhysicalDeviceVulkan13Features &
			 */
			[[nodiscard]]
			VkPhysicalDeviceVulkan13Features &
			featuresVK13 () noexcept
			{
				return m_featuresVK13;
			}

			/**
			 * @brief Returns the Vulkan 1.3 API device features for the createInfo.
			 * @return const VkPhysicalDeviceVulkan13Features &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceVulkan13Features &
			featuresVK13 () const noexcept
			{
				return m_featuresVK13;
			}

			/**
			 * @brief Returns whether the device configuration requires graphics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsGraphics () const noexcept
			{
				return m_enableGraphics;
			}

			/**
			 * @brief Returns whether the device configuration requires compute.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsCompute () const noexcept
			{
				return m_enableCompute;
			}

			/**
			 * @brief Returns whether the device configuration requires graphics presentation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsPresentation () const noexcept
			{
				return m_surface != VK_NULL_HANDLE;
			}

			/**
			 * @brief In the case of graphics presentation request, this returns the surface used for graphics to check validity.
			 * @return VkSurfaceKHR
			 */
			[[nodiscard]]
			VkSurfaceKHR
			surface () const noexcept
			{
				return m_surface;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const DeviceRequirements & obj)
			{
				out <<
					"Device requirements" "\n"
					" - Request graphics: " << ( obj.needsGraphics() ? "yes" : "no" ) << "\n"
					" - Request presentation: " << ( obj.needsPresentation() ? "yes" : "no" ) << "\n"
					" - Request compute: " << ( obj.needsCompute() ? "yes" : "no" ) << "\n";

				return out;
			}

			VkPhysicalDeviceFeatures2 m_features{};
			VkPhysicalDeviceVulkan11Features m_featuresVK11{};
			VkPhysicalDeviceVulkan12Features m_featuresVK12{};
			VkPhysicalDeviceVulkan13Features m_featuresVK13{};
			VkSurfaceKHR m_surface{VK_NULL_HANDLE};
			bool m_enableGraphics{false};
			bool m_enableCompute{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const DeviceRequirements & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
