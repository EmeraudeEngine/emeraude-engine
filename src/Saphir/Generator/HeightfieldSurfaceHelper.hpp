/*
 * src/Saphir/Generator/HeightfieldSurfaceHelper.hpp
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
#include <cstdint>
#include <memory>

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class DescriptorSetLayout;
		class LayoutManager;
	}

	namespace Saphir
	{
		class AbstractShader;
	}
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief Returns the cached descriptor set layout of a heightfield surface (Graphics::Geometry::HeightfieldSurface).
	 * @note Binding 0 = height clipmap, 1 = normal clipmap (both sampler2DArray), 2 = the per-frame
	 * uniform block; all visible to the vertex AND fragment stages (the vertex stage lights its frame
	 * from the normals too). A heightfield program binds it at its PerModel set index.
	 * @param layoutManager A reference to the layout manager.
	 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
	 */
	[[nodiscard]]
	EMEN_API
	std::shared_ptr< Vulkan::DescriptorSetLayout > getHeightfieldSurfaceDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

	/**
	 * @brief Declares, in a shader, the heightfield surface: its two samplers, its uniform block and
	 * the GLSL functions reading them — `hfHeight(xz, level)` and `hfNormalAt(xz, level)`, plus, in a
	 * fragment stage, `hfPixelNormalAt(xz)` (the normal of the finest level that covers the pixel and
	 * is not finer than its footprint, blended between two levels).
	 * @param shader A reference to the shader.
	 * @param setIndex The descriptor set index the surface is bound at (the program's PerModel set).
	 * @param fragmentStage Whether this is the fragment stage (adds the per-pixel lookup).
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API
	bool declareHeightfieldSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept;
}
