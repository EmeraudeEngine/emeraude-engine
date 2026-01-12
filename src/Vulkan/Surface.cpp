/*
 * src/Vulkan/Surface.cpp
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

#include "Surface.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	Surface::~Surface ()
	{
		if ( !m_instance.usable() )
		{
			return;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroySurfaceKHR(m_instance.handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;

			this->setDestroyed();
		}
	}

	bool
	Surface::update (const std::shared_ptr< PhysicalDevice > & physicalDevice) noexcept
	{
		if ( m_handle == VK_NULL_HANDLE )
		{
			Tracer::error(ClassId, "The surface handle is null !");

			return false;
		}

		if ( physicalDevice == nullptr )
		{
			Tracer::error(ClassId, "The physical device is null !");

			return false;
		}

		const auto queueFamilyIndex = physicalDevice->getFamilyQueueIndex(VK_QUEUE_GRAPHICS_BIT);

		if ( !queueFamilyIndex.has_value() )
		{
			TraceError{ClassId} << "The physical device '" << physicalDevice->propertiesVK10().deviceName << "' doesn't support graphics !";

			return false;
		}

		m_supported = physicalDevice->getSurfaceSupport(m_handle, queueFamilyIndex.value());

		if ( !m_supported )
		{
			TraceError{ClassId} << "The physical device '" << physicalDevice->propertiesVK10().deviceName << "' doesn't support graphics presentation !";

			return false;
		}

		m_capabilities = physicalDevice->getSurfaceCapabilities(m_handle);
		m_formats = physicalDevice->getSurfaceFormats(m_handle);
		m_presentModes = physicalDevice->getSurfacePresentModes(m_handle);
		m_presentRectangles = physicalDevice->getPresentRectangles(m_handle);

		return true;
	}

	bool
	Surface::update (const std::shared_ptr< Device > & device) noexcept
	{
		if ( m_handle == VK_NULL_HANDLE )
		{
			Tracer::error(ClassId, "The surface handle is null !");

			return false;
		}

		const auto physicalDevice = device->physicalDevice();

		m_capabilities = physicalDevice->getSurfaceCapabilities(m_handle);
		m_formats = physicalDevice->getSurfaceFormats(m_handle);
		m_presentModes = physicalDevice->getSurfacePresentModes(m_handle);
		m_presentRectangles = physicalDevice->getPresentRectangles(m_handle);

		return true;
	}

	std::string
	Surface::getCapabilitiesString (const VkSurfaceCapabilitiesKHR & capabilities) noexcept
	{
		std::stringstream output;

		output <<
			"Minimum image count : " << capabilities.minImageCount << "\n"
			"Maximum image count : " << capabilities.maxImageCount << "\n"
			"Current extent : [" << capabilities.currentExtent.width << ", " << capabilities.currentExtent.height << "]\n"
			"Minimum image extent : [" << capabilities.minImageExtent.width << ", " << capabilities.minImageExtent.height << "]\n"
			"Maximum image extent : [" << capabilities.maxImageExtent.width << ", " << capabilities.maxImageExtent.height << "]\n"
			"Maximum image array layers : " << capabilities.maxImageArrayLayers << "\n"
			"Supported transforms : " << capabilities.supportedTransforms << "\n"
			"Current transform : " << capabilities.currentTransform << "\n"
			"Supported composite alpha : " << capabilities.supportedCompositeAlpha << "\n"
			"Supported usage flags : " << capabilities.supportedUsageFlags << "\n";

		return output.str();
	}
}
