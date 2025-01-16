/*
 * src/Vulkan/DeviceRequirements.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Types.hpp"

namespace Emeraude::Vulkan
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
			 * @param deviceJobHint A hint for the device main job.
			 */
			explicit DeviceRequirements (DeviceJobHint deviceJobHint) noexcept;

			/**
			 * @brief Returns the main job of the device to help at queue creation decision.
			 * @return DeviceJobHint
			 */
			[[nodiscard]]
			DeviceJobHint
			jobHint () const noexcept
			{
				return m_deviceJobHint;
			}

			/**
			 * @brief Gives access to device features to configure it.
			 * @return VkPhysicalDeviceFeatures &
			 */
			[[nodiscard]]
			VkPhysicalDeviceFeatures &
			features () noexcept
			{
				return m_deviceFeatures;
			}

			/**
			 * @brief Returns the device feature for the create info.
			 * @return const VkPhysicalDeviceFeatures &
			 */
			[[nodiscard]]
			const VkPhysicalDeviceFeatures &
			features () const noexcept
			{
				return m_deviceFeatures;
			}

			/**
			 * @brief Declares graphics queues requirements.
			 * @param queues A reference to a vector of priorities for pure graphics queues.
			 * @param transferQueues A reference to a vector of priorities for transfer graphics queues.
			 * @return void
			 */
			void requireGraphicsQueues (const std::vector< float > & queues, const std::vector< float > & transferQueues = {}) noexcept;

			/**
			 * @brief Declares a specific queue for graphics presentation.
			 * @param queues A reference to a vector of priorities for presentation queues.
			 * @param surface A reference to a surface to query the validity.
			 * @param separate Try to use a different queue family than the graphics one if possible.
			 * @return void
			 */
			void requirePresentationQueues (const std::vector< float > & queues, VkSurfaceKHR surface, bool separate) noexcept;

			/**
			 * @brief Declares compute queues requirements.
			 * @param queues A reference to a vector of priorities for pure compute queues.
			 * @param transferQueues A reference to a vector of priorities for transfer compute queues.
			 * @return void
			 */
			void requireComputeQueues (const std::vector< float > & queues, const std::vector< float > & transferQueues = {}) noexcept;

			/**
			 * @brief Declares transfer queues requirements.
			 * @param queues A reference to a vector of priorities for pure transfer queues.
			 * @return void
			 */
			void requireTransferQueues (const std::vector< float > & queues) noexcept;

			/**
			 * @brief Returns whether the device configuration requires graphics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsGraphics () const noexcept
			{
				return !m_graphicsQueues.empty();
			}

			/**
			 * @brief Returns whether the device configuration requires compute.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsCompute () const noexcept
			{
				return !m_computeQueues.empty();
			}

			/**
			 * @brief Returns whether the device configuration requires graphics presentation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsPresentation () const noexcept
			{
				return !m_presentationQueues.empty();
			}

			/**
			 * @brief Returns whether to try a separate queue for presentation than graphics queue.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			tryPresentationSeparateFromGraphics () const noexcept
			{
				return m_presentationSeparated;
			}

			/**
			 * @brief Returns whether the device configuration requires separate transfer queues.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			needsTransfer () const noexcept
			{
				return !m_transferQueues.empty();
			}

			/**
			 * @brief Returns the number of required queue count for the device configuration.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t getRequiredQueueCount () const noexcept;

			/**
			 * @brief In case of graphics presentation request, this returns the surface used for graphics to check validity.
			 * @return VkSurfaceKHR
			 */
			[[nodiscard]]
			VkSurfaceKHR
			surface () const noexcept
			{
				return m_surface;
			}

			/**
			 * @brief Returns the graphics queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			graphicsQueuePriorities () const noexcept
			{
				return m_graphicsQueues;
			}

			/**
			 * @brief Returns the graphics transfer queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			graphicsTransferQueuePriorities () const noexcept
			{
				return m_graphicsTransferQueues;
			}

			/**
			 * @brief Returns the presentation queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			presentationQueuePriorities () const noexcept
			{
				return m_presentationQueues;
			}

			/**
			 * @brief Returns the compute queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			computeQueuePriorities () const noexcept
			{
				return m_computeQueues;
			}

			/**
			 * @brief Returns the compute transfer queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			computeTransferQueuePriorities () const noexcept
			{
				return m_presentationQueues;
			}

			/**
			 * @brief Returns the transfer queue priorities.
			 * @return const std::vector< float > &
			 */
			[[nodiscard]]
			const std::vector< float > &
			transferQueuePriorities () const noexcept
			{
				return m_transferQueues;
			}

			/**
			 * @brief Returns whether any queue has been declared.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasQueueDeclared () const noexcept
			{
				if ( !m_graphicsQueues.empty() )
				{
					return true;
				}

				if ( !m_computeQueues.empty() )
				{
					return true;
				}

				if ( !m_transferQueues.empty() )
				{
					return true;
				}

				return false;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const DeviceRequirements & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const DeviceRequirements & obj) noexcept;

		private:

			DeviceJobHint m_deviceJobHint;
			VkPhysicalDeviceFeatures m_deviceFeatures{};
			std::vector< float > m_graphicsQueues;
			std::vector< float > m_graphicsTransferQueues;
			std::vector< float > m_presentationQueues;
			std::vector< float > m_computeQueues;
			std::vector< float > m_computeTransferQueues;
			std::vector< float > m_transferQueues;
			VkSurfaceKHR m_surface{VK_NULL_HANDLE};
			bool m_presentationSeparated{false};
	};
}
