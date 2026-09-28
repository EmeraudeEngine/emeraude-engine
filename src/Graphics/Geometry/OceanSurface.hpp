/*
 * src/Graphics/Geometry/OceanSurface.hpp
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

/**
 * @brief The contract between an ocean surface geometry (C++, Geometry::OceanSurfaceResource) and the shader generator
 * (GLSL, Saphir::Generator::declareOceanSurface() and the ocean branch of AbstractVertexStage).
 * @details An ocean is a heightfield-STYLE surface (EnableHeightfieldSurface + EnableOceanSurface): the CDLOD of the
 * terrain (F. Strugar, JGT 2009) on an INFINITE plane — a world-aligned quadtree selected by distance around the camera,
 * one shared flat patch pushed per node (the heightfield node + camera push constants), the odd lattice coordinates
 * geomorphed onto the even ones toward the next level — displaced by the FFT wave cascades (Graphics::OceanWaves)
 * instead of lifted from a height clipmap. The fragment stage rebuilds the choppy surface's normal per pixel from the
 * slopes, through the heightfield per-pixel frame.
 * @note Its descriptor set layout is the heightfield's plus one binding (Saphir::Generator::getOceanSurfaceDescriptorSetLayout()):
 * binding 0 is the displacement, binding 1 the slopes, binding 2 the Uniforms below, binding 3 the whitecap foam.
 * @note Everything both sides must agree on lives HERE.
 */
namespace EmEn::Graphics::Geometry::OceanSurface
{
	/** @brief Binding of the displacement cascades (sampler2DArray): (λ Dx, h, λ Dz, λ ∂Dx/∂z). */
	static constexpr uint32_t DisplacementBinding{0};
	/** @brief Binding of the slope cascades (sampler2DArray): (∂h/∂x, ∂h/∂z, λ ∂Dx/∂x, λ ∂Dz/∂z). */
	static constexpr uint32_t SlopesBinding{1};
	/** @brief Binding of the per-frame uniform block. */
	static constexpr uint32_t UniformsBinding{2};
	/** @brief Binding of the whitecap foam cascades (sampler2DArray): coverage 0-1 in the red channel. */
	static constexpr uint32_t FoamBinding{3};
	/** @brief Maximum number of levels of detail (quadtree depth) the uniform block describes. */
	static constexpr uint32_t MaxLevelsOfDetail{16};

	/**
	 * @brief The per-frame uniform block (std140, vec4 members only).
	 * @note Mirrored by Saphir::Generator::declareOceanSurface(); change both together.
	 */
	struct Uniforms
	{
		/** @brief x = finest cell (m), y = unused, z = level count, w = patch quads per side. */
		std::array< float, 4 > grid{};
		/** @brief x = sea level (m), yzw = unused. */
		std::array< float, 4 > level{};
		/** @brief xyz = tile side of each cascade (m), w = cascade count. */
		std::array< float, 4 > cascades{};
		/** @brief x = U per metre, y = V per metre, z = U at x = 0, w = V at z = 0. */
		std::array< float, 4 > textureCoordinates{};
		/** @brief Per level of detail: x = morph start (m), y = 1 / (morph end − morph start), z = the distance it is drawn to (m), w = unused. */
		std::array< std::array< float, 4 >, MaxLevelsOfDetail > levelsOfDetail{};
	};

	/** @brief GLSL name of the displacement sampler. */
	static constexpr auto DisplacementSamplerName{"ocDisplacement"};
	/** @brief GLSL name of the slope sampler. */
	static constexpr auto SlopesSamplerName{"ocSlopes"};
	/** @brief GLSL name of the whitecap foam sampler. */
	static constexpr auto FoamSamplerName{"ocFoam"};
	/** @brief GLSL name of the fragment-stage function returning the whitecap coverage at a lattice XZ: `float ocWhitecapAt(vec2)`. */
	static constexpr auto WhitecapFunction{"ocWhitecapAt"};
	/** @brief GLSL block type name of the uniforms. */
	static constexpr auto UniformBlockName{"OceanSurface"};
	/** @brief GLSL instance name of the uniforms. */
	static constexpr auto UniformBlockInstance{"ocSurface"};
	/** @brief GLSL name of the vertex stage's UNDISPLACED world XZ (the lattice point the cascades are indexed by). */
	static constexpr auto LatticePositionVariable{"ocLatticeXZ"};
}
