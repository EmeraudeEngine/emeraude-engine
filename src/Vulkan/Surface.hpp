/*
 * src/Vulkan/Surface.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"
#include "Instance.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class PhysicalDevice;
	class Device;
};

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines the surface where the final image will be drawn and presented to the screen.
	 * @extends EmEn::Vulkan::AbstractObject This is a Vulkan API object.
	 */
	class Surface final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSurface"};

			/**
			 * @brief Constructs a surface.
			 * @param instance A reference to a vulkan instance.
			 * @param surface A vulkan surface handle.
			 */
			Surface (const Instance & instance, const VkSurfaceKHR & surface) noexcept
				: m_instance{instance},
				m_handle{surface}
			{
				/* NOTE: This is a resource given "as is" from the OS. */
				this->setCreated();
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Surface (const Surface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Surface (Surface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Surface & operator= (const Surface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Surface & operator= (Surface && copy) noexcept = delete;

			/**
			 * @brief Destructs the surface.
			 */
			~Surface () override;

			/**
			 * @brief Updates surface information with a physical device.
			 * @param physicalDevice A reference to the physical device smart pointer.
			 * @return bool
			 */
			bool update (const std::shared_ptr< PhysicalDevice > & physicalDevice) noexcept;

			/**
			 * @brief Updates surface information with a physical device.
			 * @param device A reference to the device smart pointer.
			 * @return bool
			 */
			bool update (const std::shared_ptr< Device > & device) noexcept;

			/**
			 * @brief Returns the surface handle.
			 * @return VkSurfaceKHR
			 */
			[[nodiscard]]
			VkSurfaceKHR
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the surface capabilities.
			 * @return const VkSurfaceCapabilitiesKHR &
			 */
			[[nodiscard]]
			const VkSurfaceCapabilitiesKHR &
			capabilities () const noexcept
			{
				return m_capabilities;
			}

			/**
			 * @brief Returns the surface formats.
			 * @return const std::vector< VkSurfaceFormatKHR > &
			 */
			[[nodiscard]]
			const std::vector< VkSurfaceFormatKHR > &
			formats () const noexcept
			{
				return m_formats;
			}

			/**
			 * @brief Returns the surface present modes.
			 * @return const std::vector< VkPresentModeKHR > &
			 */
			[[nodiscard]]
			const std::vector< VkPresentModeKHR > &
			presentModes () const noexcept
			{
				return m_presentModes;
			}

			/**
			 * @brief Returns the surface present rectangles.
			 * @return const std::vector< VkRect2D > &
			 */
			[[nodiscard]]
			const std::vector< VkRect2D > &
			presentRectangles () const noexcept
			{
				return m_presentRectangles;
			}

			/**
			 * @brief Returns whether this surface supports the presentation.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			presentationSupported () const noexcept
			{
				return m_supported;
			}

			/**
			 * @brief Returns a string of surface capabilities.
			 * @param capabilities A reference to a surface capabilities structure.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string getCapabilitiesString (const VkSurfaceCapabilitiesKHR & capabilities) noexcept;

		private:

			const Instance & m_instance;
			VkSurfaceKHR m_handle{VK_NULL_HANDLE};
			VkSurfaceCapabilitiesKHR m_capabilities{};
			std::vector< VkSurfaceFormatKHR > m_formats;
			std::vector< VkPresentModeKHR > m_presentModes;
			std::vector< VkRect2D > m_presentRectangles;
			bool m_supported{false};
	};
}
