/*
 * src/Vulkan/DeviceRequirements.cpp
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

#include "DeviceRequirements.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>
#include <sstream>

/* Local usages. */
#include "Window.hpp"

namespace EmEn::Vulkan
{
	DeviceRequirements::DeviceRequirements (bool enableGraphics, Window * window, bool enableCompute) noexcept
		: m_enableGraphics{enableGraphics},
		m_enableCompute{enableCompute}
	{
		if ( enableGraphics && window != nullptr )
		{
			if ( window->surface() == nullptr )
			{
				TraceError{ClassId} << "The window surface pointer is null!";
			}
			else
			{
				m_surface = window->surface()->handle();
			}
		}

		/* NOTE: Every structure carries its type; which ones enter the chain given to vkCreateDevice() depends on the
		 * device's effective API version and its enabled extensions: linkFeatureChain(). */
		m_portabilitySubsetFeatures.sType = PortabilitySubset::FeaturesType;
		m_meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		m_faultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
		m_rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		m_timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		m_featuresVK13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		m_featuresVK12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		m_featuresVK11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		m_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		this->linkFeatureChain(VK_API_VERSION_1_1, {});
	}

	void
	DeviceRequirements::linkFeatureChain (uint32_t apiVersion, const std::vector< const char * > & extensions) noexcept
	{
		const auto enabled = [&extensions] (const char * name) {
			return std::ranges::any_of(extensions, [name] (const char * extension) {
				return std::strcmp(extension, name) == 0;
			});
		};

		m_features.pNext = nullptr;

		void ** tail = &m_features.pNext;

		const auto append = [&tail] (auto & structure) {
			structure.pNext = nullptr;
			*tail = &structure;
			tail = &structure.pNext;
		};

		/* NOTE: VkPhysicalDeviceVulkan11Features and VkPhysicalDeviceVulkan12Features exist from 1.2. Below, the only
		 * feature the engine enables is the timeline, through VK_KHR_timeline_semaphore's structure (same member). */
		if ( apiVersion >= VK_API_VERSION_1_2 )
		{
			append(m_featuresVK11);
			append(m_featuresVK12);
		}
		else if ( enabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) )
		{
			m_timelineSemaphoreFeatures.timelineSemaphore = m_featuresVK12.timelineSemaphore;

			append(m_timelineSemaphoreFeatures);
		}

		if ( apiVersion >= VK_API_VERSION_1_3 )
		{
			append(m_featuresVK13);
		}

		if ( enabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) )
		{
			append(m_accelerationStructureFeatures);
		}

		if ( enabled(VK_KHR_RAY_QUERY_EXTENSION_NAME) )
		{
			append(m_rayQueryFeatures);
		}

		if ( enabled(VK_EXT_DEVICE_FAULT_EXTENSION_NAME) )
		{
			append(m_faultFeatures);
		}

		if ( enabled(VK_EXT_MESH_SHADER_EXTENSION_NAME) )
		{
			append(m_meshShaderFeatures);
		}

		/* NOTE: Chained whenever the extension is enabled: the specification makes every portability feature disabled
		 * by default, so omitting the structure silently loses capabilities the device has. */
		if ( enabled(PortabilitySubset::ExtensionName) )
		{
			append(m_portabilitySubsetFeatures);
		}
	}

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

	std::string
	to_string (const DeviceRequirements & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
