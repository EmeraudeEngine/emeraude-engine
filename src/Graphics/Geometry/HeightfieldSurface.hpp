/*
 * src/Graphics/Geometry/HeightfieldSurface.hpp
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
#include <array>
#include <cstdint>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/**
 * @brief The contract between a heightfield geometry (C++) and the shader generator (GLSL).
 * @details A heightfield surface is drawn as ONE shared flat patch, instanced per quadtree node by
 * push constants, and displaced in the vertex stage by a CLIPMAP of heights: `ClipLevelCount` square
 * levels of `ClipTexelCount²` texels, level `l` having a texel of `cellSize · 2^l`, all centred on the
 * camera and updated toroidally (a world lattice point `i` lives in texel `i mod ClipTexelCount`, so a
 * REPEAT sampler addresses it without any per-level origin). A second array holds the normal of each
 * level, baked on the GPU from that level's heights; the fragment stage reads it per pixel.
 * References: F. Strugar, "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps"
 * (JGT, 2009, https://github.com/fstrugar/CDLOD) for the patch, the node selection and the geomorph;
 * F. Losasso & H. Hoppe, "Geometry Clipmaps" (SIGGRAPH 2004) for the toroidal level update.
 * @note Everything both sides must agree on lives HERE: bindings, formats, the uniform block layout
 * and the names the generated GLSL uses.
 */
namespace EmEn::Graphics::Geometry::HeightfieldSurface
{
	/** @brief Maximum number of clip levels the uniform block describes. */
	static constexpr uint32_t MaxClipLevels{8};

	/** @brief Maximum number of levels of detail (quadtree depth) the uniform block describes. */
	static constexpr uint32_t MaxLevelsOfDetail{16};

	/** @brief Binding of the height clipmap (sampler2DArray, one layer per level) — vertex AND fragment stages. */
	static constexpr uint32_t HeightsBinding{0};

	/** @brief Binding of the normal clipmap (sampler2DArray, one layer per level) — fragment stage. */
	static constexpr uint32_t NormalsBinding{1};

	/** @brief Binding of the per-frame uniform block (Uniforms) — vertex AND fragment stages. */
	static constexpr uint32_t UniformsBinding{2};

	/**
	 * @brief Format of the height clipmap: 16-bit fixed point over the surface's height range.
	 * @note Owner decision (2026-09-22). The range is known at load (the source's bounding box), so a
	 * texel is `min + unorm · range`: 6 cm of quantum over 4000 m.
	 */
	static constexpr VkFormat HeightFormat{VK_FORMAT_R16_UNORM};

	/**
	 * @brief Format of the normal clipmap: the X and Z components of the unit normal (Y is positive on
	 * a heightfield and rebuilt from them).
	 * @note Half floats keep their precision where a terrain needs it — near a flat normal, where X
	 * and Z are small. Written by a compute pass, so it needs `shaderStorageImageExtendedFormats`.
	 */
	static constexpr VkFormat NormalFormat{VK_FORMAT_R16G16_SFLOAT};

	/**
	 * @brief The per-frame uniform block, std140 (vec4 members only, no padding question).
	 * @note Mirrored by Saphir::Generator::declareHeightfieldSurface(); change both together.
	 */
	struct Uniforms
	{
		/** @brief x = cell size (m), y = texels per level side, z = level count, w = patch quads per side. */
		std::array< float, 4 > grid{};
		/** @brief x = height range (m), y = minimum height (m), zw = unused. */
		std::array< float, 4 > height{};
		/** @brief x = U per metre, y = V per metre, z = U at x = 0, w = V at z = 0. */
		std::array< float, 4 > textureCoordinates{};
		/** @brief Per clip level: xy = centre (world XZ), z = half extent the level is VALID over (m), w = texel size (m). */
		std::array< std::array< float, 4 >, MaxClipLevels > levels{};
		/** @brief Per level of detail: x = morph start (m), y = 1 / (morph end - morph start), z = the distance it is drawn to (m), w = unused. */
		std::array< std::array< float, 4 >, MaxLevelsOfDetail > levelsOfDetail{};
	};

	static_assert(sizeof(Uniforms) == (3 + MaxClipLevels + MaxLevelsOfDetail) * 16, "HeightfieldSurface::Uniforms must stay a packed array of vec4 (std140).");

	/** @brief GLSL name of the height clipmap sampler. */
	static constexpr auto HeightsSamplerName{"hfHeights"};

	/** @brief GLSL name of the normal clipmap sampler. */
	static constexpr auto NormalsSamplerName{"hfNormals"};

	/** @brief GLSL name of the uniform block type. */
	static constexpr auto UniformBlockName{"HeightfieldSurface"};

	/** @brief GLSL instance name of the uniform block. */
	static constexpr auto UniformBlockInstance{"hfSurface"};

	/** @brief GLSL name of the world XZ the vertex stage hands the fragment stage (smooth vec2). */
	static constexpr auto PixelPositionVarying{"hfPixelXZ"};

	/** @brief GLSL name of the flat object-to-world rotation of a surface vector (mat3). */
	static constexpr auto PixelToWorldVarying{"hfPixelToWorld"};

	/** @brief GLSL name of the flat normal matrix (object-to-view) of a surface vector (mat3). */
	static constexpr auto PixelToViewVarying{"hfPixelToView"};
}
