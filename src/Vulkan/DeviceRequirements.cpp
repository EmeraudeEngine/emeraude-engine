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

		/* NOTE: Portability subset features (KHR extension) — MoltenVK and other portability
		 * implementations. It sits at the TAIL of the chain and stays all-zero unless the caller fills
		 * it (see Instance.cpp, graphics device features configuration).
		 *
		 * This structure being chained is NOT optional when the extension is enabled: the spec makes
		 * every portability feature disabled by default, so an application that enables the extension
		 * and omits the structure silently loses capabilities the device actually has. */
		m_portabilitySubsetFeatures.sType = PortabilitySubset::FeaturesType;
		m_portabilitySubsetFeatures.pNext = nullptr;
		/* NOTE: Device fault features (EXT extension) — GPU device-lost diagnostics. */
		m_faultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
		m_faultFeatures.pNext = &m_portabilitySubsetFeatures;
		/* NOTE: Ray query features (KHR extension). */
		m_rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		m_rayQueryFeatures.pNext = &m_faultFeatures;
		/* NOTE: Acceleration structure features (KHR extension). */
		m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		m_accelerationStructureFeatures.pNext = &m_rayQueryFeatures;
		/* NOTE: Device features from Vulkan 1.3 API. */
		m_featuresVK13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		m_featuresVK13.pNext = &m_accelerationStructureFeatures;
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
