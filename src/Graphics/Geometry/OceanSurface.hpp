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
 * binding 0 is the displacement, binding 1 the slopes, binding 2 the Uniforms below, binding 3 the whitecap foam, binding
 * 4 the previous frame's displacement (the velocity of the displaced surface), binding 5 the shelter map.
 * @note Everything both sides must agree on lives HERE.
 * @note THE MESH: a patch cell of the finest level, between the lattice points (x, z) and (x + 1, z + 1) in cells of
 * Uniforms::grid.x metres, is cut along its (x, z)-(x + 1, z + 1) diagonal into the triangles (x, z), (x, z + 1),
 * (x + 1, z + 1) and (x, z), (x + 1, z + 1), (x + 1, z) — OceanSurfaceResource's patch indices. Each lattice point is
 * moved by the displacement read AT that point and the surface is flat in between: what `ocSurfaceRiseAt()` walks
 * (Saphir::Generator, the ocean surface helper) to tell how high the DRAWN surface stands over a world XZ. Change the
 * two together.
 * @note THE SHELTER MAP (owner decisions 2026-11-24, engine item ocean-sheltered-water): one channel over a rectangle
 * of the world, 1 where the water is the open sea, 0 where land shelters it. Every read of the cascades — the
 * displacement, the normals, the whitecaps — weighs each cascade by `mix(shelteredShares[cascade], 1, map)`: a
 * sheltered water keeps the share of its smallest waves alone. Outside the rectangle the map's edge holds: the sea.
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
	/** @brief Binding of the PREVIOUS frame's displacement cascades (sampler2DArray, same layout as the displacement). */
	static constexpr uint32_t PreviousDisplacementBinding{4};
	/** @brief Binding of the shelter map (sampler2D, one channel): 1 the open sea, 0 a water sheltered by land. */
	static constexpr uint32_t ShelterBinding{5};
	/** @brief Maximum number of levels of detail (quadtree depth) the uniform block describes. */
	static constexpr uint32_t MaxLevelsOfDetail{16};
	/** @brief The most texels a side of the shelter map may hold (what base VertexFactory::buildShelterMap() takes). */
	static constexpr uint32_t MaxShelterMapSide{4096};

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
		/** @brief The shelter map's rectangle: x = the world X of its first corner (m), y = its world Z, z = 1 / its
		 * side on X (1/m), w = 1 / its side on Z. All zero without a map: every read lands on one texel of a map that
		 * is the open sea everywhere. */
		std::array< float, 4 > shelter{};
		/** @brief xyz = the share of each cascade's waves a sheltered water keeps (0 to 1), w = unused. */
		std::array< float, 4 > shelteredShares{};
		/** @brief Per level of detail: x = morph start (m), y = 1 / (morph end − morph start), z = the distance it is drawn to (m), w = unused. */
		std::array< std::array< float, 4 >, MaxLevelsOfDetail > levelsOfDetail{};
	};

	/** @brief GLSL name of the displacement sampler. */
	static constexpr auto DisplacementSamplerName{"ocDisplacement"};
	/** @brief GLSL name of the slope sampler. */
	static constexpr auto SlopesSamplerName{"ocSlopes"};
	/** @brief GLSL name of the previous displacement sampler. */
	static constexpr auto PreviousDisplacementSamplerName{"ocPreviousDisplacement"};
	/** @brief GLSL name of the vertex stage's previous displaced world position (the velocity's previous position). */
	static constexpr auto PreviousPositionVariable{"ocPreviousPosition"};
	/** @brief GLSL name of the whitecap foam sampler. */
	static constexpr auto FoamSamplerName{"ocFoam"};
	/** @brief GLSL name of the shelter map sampler. */
	static constexpr auto ShelterSamplerName{"ocShelter"};
	/** @brief GLSL name of the function returning how high the DRAWN surface stands above the sea level over a world
	 * XZ, around the eye (the finest level's mesh): `float ocSurfaceRiseAt(vec2)`. */
	static constexpr auto SurfaceRiseFunction{"ocSurfaceRiseAt"};
	/** @brief GLSL name of the fragment-stage function returning the whitecap coverage at a lattice XZ: `float ocWhitecapAt(vec2)`. */
	static constexpr auto WhitecapFunction{"ocWhitecapAt"};
	/** @brief GLSL block type name of the uniforms. */
	static constexpr auto UniformBlockName{"OceanSurface"};
	/** @brief GLSL instance name of the uniforms. */
	static constexpr auto UniformBlockInstance{"ocSurface"};
	/** @brief GLSL name of the vertex stage's UNDISPLACED world XZ (the lattice point the cascades are indexed by). */
	static constexpr auto LatticePositionVariable{"ocLatticeXZ"};
}
