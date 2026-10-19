/*
 * src/Vulkan/PortabilitySubset.hpp
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

/* Third-party inclusions. */
#include "volk.h"

namespace EmEn::Vulkan::PortabilitySubset
{
	/**
	 * @brief The VK_KHR_portability_subset extension name.
	 * @note The VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME macro lives behind
	 * VK_ENABLE_BETA_EXTENSIONS, which this project does not define — Instance.hpp already spells
	 * the name out for the same reason.
	 */
	constexpr auto ExtensionName{"VK_KHR_portability_subset"};

	/** @brief VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR. */
	constexpr VkStructureType FeaturesType{static_cast< VkStructureType >(1000163000)};

	/**
	 * @brief Mirror of VkPhysicalDevicePortabilitySubsetFeaturesKHR.
	 *
	 * @warning The member order is fixed by the published extension and must NEVER be reordered:
	 * this is read and written by the driver through a pNext chain, so a mismatch silently shifts
	 * every field.
	 *
	 * @note **Why this matters beyond the struct itself.** When the extension is enabled, the spec
	 * requires the application to chain this structure into `VkDeviceCreateInfo::pNext` to enable the
	 * portability features it needs. Enabling the extension WITHOUT chaining it leaves every feature
	 * below **disabled** — the device supports them, the application simply never asked. That is not
	 * a theoretical concern: it silently cost the engine hardware depth comparison
	 * (`mutableComparisonSamplers`), which made every shadow-map descriptor write fail on MoltenVK
	 * and left the shadow term reading an undefined descriptor.
	 */
	struct Features
	{
		VkStructureType sType{FeaturesType};
		void * pNext{nullptr};
		VkBool32 constantAlphaColorBlendFactors{VK_FALSE};
		VkBool32 events{VK_FALSE};
		VkBool32 imageViewFormatReinterpretation{VK_FALSE};
		VkBool32 imageViewFormatSwizzle{VK_FALSE};
		VkBool32 imageView2DOn3DImage{VK_FALSE};
		VkBool32 multisampleArrayImage{VK_FALSE};
		VkBool32 mutableComparisonSamplers{VK_FALSE};
		VkBool32 pointPolygons{VK_FALSE};
		VkBool32 samplerMipLodBias{VK_FALSE};
		VkBool32 separateStencilMaskRef{VK_FALSE};
		VkBool32 shaderSampleRateInterpolationFunctions{VK_FALSE};
		VkBool32 tessellationIsolines{VK_FALSE};
		VkBool32 tessellationPointMode{VK_FALSE};
		VkBool32 triangleFans{VK_FALSE};
		VkBool32 vertexAttributeAccessBeyondStride{VK_FALSE};
	};
}
