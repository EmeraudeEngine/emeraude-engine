/*
 * src/Saphir/Generator/OceanSurfaceHelper.hpp
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

/* STL inclusions. */
#include <cstdint>

/* STL inclusions. */
#include <memory>

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSetLayout;
	class LayoutManager;
}

namespace EmEn::Saphir
{
	class AbstractShader;
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief Declares an ocean surface's resources and functions in a shader (Graphics::Geometry::OceanSurface): the
	 * displacement, slope and foam cascades (and, in a vertex stage, the previous displacement with
	 * `ocPreviousDisplacementAt()`), the per-frame uniform block, `ocDisplacementAt()` and `ocNormalAt()`, and —
	 * in a fragment stage — `hfPixelNormalAt()`, the normal the heightfield per-pixel frame rebuilds, and
	 * `ocWhitecapAt()`, the whitecap coverage.
	 * @note The set layout is getOceanSurfaceDescriptorSetLayout().
	 * @param shader The shader.
	 * @param setIndex The PerModel set index of the program.
	 * @param fragmentStage True in a fragment stage (adds the per-pixel normal).
	 * @return bool
	 */
	bool declareOceanSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept;

	/**
	 * @brief Returns the descriptor set layout of an ocean surface (Graphics::Geometry::OceanSurface): the heightfield's
	 * three bindings (displacement, slopes, uniforms), the whitecap foam at binding 3 and the previous frame's
	 * displacement at binding 4 (the velocity).
	 * @param layoutManager The layout manager.
	 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
	 */
	[[nodiscard]]
	std::shared_ptr< Vulkan::DescriptorSetLayout > getOceanSurfaceDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;
}
