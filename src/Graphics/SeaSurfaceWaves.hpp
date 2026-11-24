/*
 * src/Graphics/SeaSurfaceWaves.hpp
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

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class ImageView;
	class Sampler;
	class UniformBufferObject;
}

namespace EmEn::Graphics
{
	/**
	 * @brief What a pass binds to know where the MOVING surface of a sea stands over a world XZ: what its mesh is drawn
	 * with.
	 * @note Answered by a sea whose surface moves (Scenes::SeaLevelInterface::surfaceWaves(): the FFT ocean); a sea
	 * at rest answers none and its surface is its level. Read by the underwater view to decide, per pixel, on which
	 * side of the DRAWN surface a ray starts (Effects::Atmosphere::WaterVolume, engine item underwater-view).
	 * @note The three resources are the ones the ocean's own stages bind, and the GLSL that reads them is the ocean's
	 * own (Saphir::Generator::getOceanSurfaceStandingSource(), `ocSurfaceRiseAt()`): one source for where the surface
	 * stands (owner decision 2026-11-24). Nothing of the mesh is restated here.
	 * @warning Valid for the frame it was asked in: the views and the buffer belong to the sea's geometry, and the
	 * uniforms' offset is that frame's.
	 */
	struct SeaSurfaceWaves final
	{
		/** @brief The displacement cascades (a 2D array, GENERAL layout): (Dx, h, Dz) in metres per layer, tiling. */
		const Vulkan::ImageView * displacement{nullptr};
		/** @brief The sampler the mesh reads them with (repeat, linear, no mipmap). */
		const Vulkan::Sampler * sampler{nullptr};
		/** @brief The shelter map (2D, one channel, SHADER_READ_ONLY_OPTIMAL): 1 the open sea, 0 a sheltered water. */
		const Vulkan::ImageView * shelter{nullptr};
		/** @brief The sampler the mesh reads it with (clamp to edge, linear). */
		const Vulkan::Sampler * shelterSampler{nullptr};
		/** @brief The buffer holding the surface's uniform block of this frame (Geometry::OceanSurface::Uniforms). */
		const Vulkan::UniformBufferObject * uniforms{nullptr};
		/** @brief The offset of this frame's block in that buffer, in bytes. */
		uint64_t uniformsOffset{0};
		/** @brief The size of the block, in bytes. */
		uint64_t uniformsRange{0};
		/** @brief The bound of the surface's reach above and below the level, in metres. */
		float reach{0.0F};
	};
}
