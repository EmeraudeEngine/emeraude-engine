/*
 * src/Saphir/Generator/SkinningLayoutHelper.hpp
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

/* Local inclusions for usages. */
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief Returns the cached descriptor set layout for the skeletal skinning SSBO.
	 * @param layoutManager A reference to the layout manager.
	 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
	 */
	[[nodiscard]]
	inline
	std::shared_ptr< Vulkan::DescriptorSetLayout >
	getSkinningDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept
	{
		static constexpr auto UUID{"SkinningSSBO"};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier("SkeletalAnimation", "SkinningMatrices", "DescriptorSetLayout");

			/* Binding 0: Skinning matrices SSBO (host-visible, updated per frame). */
			descriptorSetLayout->declareStorageBuffer(0, VK_SHADER_STAGE_VERTEX_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}
}
