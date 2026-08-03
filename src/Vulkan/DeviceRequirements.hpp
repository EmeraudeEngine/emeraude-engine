/*
 * src/Vulkan/DeviceRequirements.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions. */
#include "PortabilitySubset.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Window;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief This class describes the requirements to create a Vulkan logical device.
	 */
	class EMEN_API DeviceRequirements final
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
			DeviceRequirements (bool enableGraphics, Window * window, bool enableCompute) noexcept;

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

			/**
			 * @brief Gives access to configure acceleration structure features (KHR extension).
			 * @return VkPhysicalDeviceAccelerationStructureFeaturesKHR &
			 */
			[[nodiscard]]
			VkPhysicalDeviceAccelerationStructureFeaturesKHR &
			accelerationStructureFeatures () noexcept
			{
				return m_accelerationStructureFeatures;
			}

			/**
			 * @brief Returns the acceleration structure features (KHR extension).
			 * @return const VkPhysicalDeviceAccelerationStructureFeaturesKHR &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceAccelerationStructureFeaturesKHR &
			accelerationStructureFeatures () const noexcept
			{
				return m_accelerationStructureFeatures;
			}

			/**
			 * @brief Gives access to configure ray query features (KHR extension).
			 * @return VkPhysicalDeviceRayQueryFeaturesKHR &
			 */
			[[nodiscard]]
			VkPhysicalDeviceRayQueryFeaturesKHR &
			rayQueryFeatures () noexcept
			{
				return m_rayQueryFeatures;
			}

			/**
			 * @brief Returns the ray query features (KHR extension).
			 * @return const VkPhysicalDeviceRayQueryFeaturesKHR &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceRayQueryFeaturesKHR &
			rayQueryFeatures () const noexcept
			{
				return m_rayQueryFeatures;
			}

			/**
			 * @brief Gives access to configure device fault features (EXT extension).
			 * @return VkPhysicalDeviceFaultFeaturesEXT &
			 */
			[[nodiscard]]
			VkPhysicalDeviceFaultFeaturesEXT &
			faultFeatures () noexcept
			{
				return m_faultFeatures;
			}

			/**
			 * @brief Gives access to configure the VK_KHR_portability_subset device features.
			 * @note Only meaningful when the selected device advertises the extension. Fill it from
			 * PhysicalDevice::portabilitySubsetFeatures(): requesting a feature the device does not
			 * support fails device creation, and requesting NOTHING leaves them all disabled even
			 * though the device supports them — which is exactly how the engine silently lost hardware
			 * depth comparison on MoltenVK.
			 * @return PortabilitySubset::Features &
			 */
			[[nodiscard]]
			PortabilitySubset::Features &
			portabilitySubsetFeatures () noexcept
			{
				return m_portabilitySubsetFeatures;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const DeviceRequirements & obj);

			VkPhysicalDeviceFeatures2 m_features{};
			VkPhysicalDeviceVulkan11Features m_featuresVK11{};
			VkPhysicalDeviceVulkan12Features m_featuresVK12{};
			VkPhysicalDeviceVulkan13Features m_featuresVK13{};
			VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};
			VkPhysicalDeviceRayQueryFeaturesKHR m_rayQueryFeatures{};
			VkPhysicalDeviceFaultFeaturesEXT m_faultFeatures{};
			PortabilitySubset::Features m_portabilitySubsetFeatures{};
			VkSurfaceKHR m_surface{VK_NULL_HANDLE};
			bool m_enableGraphics{false};
			bool m_enableCompute{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	EMEN_API std::string to_string (const DeviceRequirements & obj) noexcept;
}
