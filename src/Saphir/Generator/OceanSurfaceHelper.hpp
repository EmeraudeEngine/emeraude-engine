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
#include <string>

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
	 * @brief Where a shader finds what tells where an ocean surface stands (getOceanSurfaceStandingSource()).
	 */
	struct OceanSurfaceStandingBindings final
	{
		/** @brief The descriptor set. */
		uint32_t set{0};
		/** @brief The binding of the displacement cascades (sampler2DArray). */
		uint32_t displacement{0};
		/** @brief The binding of the shelter map (sampler2D). */
		uint32_t shelter{0};
		/** @brief The binding of the surface's per-frame uniform block (Graphics::Geometry::OceanSurface::Uniforms). */
		uint32_t uniforms{0};
	};

	/**
	 * @brief Returns the GLSL of where an ocean surface stands, for a shader written by hand (an effect): the
	 * displacement cascades, the shelter map and the uniform block at the given bindings, `ocShelterAt()`,
	 * `ocCascadeWeight()`, `ocDisplacementAt()` and `ocSurfaceRiseAt()` — how high the DRAWN surface stands above the
	 * sea level over a world XZ around the eye.
	 * @note The SAME declarations declareOceanSurface() gives the ocean's own stages (owner decision 2026-11-24: one
	 * source for where the surface stands): what draws the sea and what stands a pixel against it cannot disagree.
	 * @param bindings Where the shader binds the three resources.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string getOceanSurfaceStandingSource (const OceanSurfaceStandingBindings & bindings) noexcept;

	/**
	 * @brief Declares an ocean surface's resources and functions in a shader (Graphics::Geometry::OceanSurface): the
	 * displacement, slope and foam cascades (and, in a vertex stage, the previous displacement with
	 * `ocPreviousDisplacementAt()`), the shelter map with `ocShelterAt()`, the per-frame uniform block,
	 * `ocDisplacementAt()` and `ocNormalAt()`, and — in a fragment stage — `hfPixelNormalAt()`, the normal the
	 * heightfield per-pixel frame rebuilds, and `ocWhitecapAt()`, the whitecap coverage.
	 * @note The set layout is getOceanSurfaceDescriptorSetLayout().
	 * @param shader The shader.
	 * @param setIndex The PerModel set index of the program.
	 * @param fragmentStage True in a fragment stage (adds the per-pixel normal).
	 * @return bool
	 */
	bool declareOceanSurface (AbstractShader & shader, uint32_t setIndex, bool fragmentStage) noexcept;

	/**
	 * @brief Returns the descriptor set layout of an ocean surface (Graphics::Geometry::OceanSurface): the heightfield's
	 * three bindings (displacement, slopes, uniforms), the whitecap foam at binding 3, the previous frame's
	 * displacement at binding 4 (the velocity) and the shelter map at binding 5.
	 * @param layoutManager The layout manager.
	 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
	 */
	[[nodiscard]]
	std::shared_ptr< Vulkan::DescriptorSetLayout > getOceanSurfaceDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;
}
