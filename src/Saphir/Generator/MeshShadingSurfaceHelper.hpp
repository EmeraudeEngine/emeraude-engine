/*
 * src/Saphir/Generator/MeshShadingSurfaceHelper.hpp
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

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics::Material
	{
		class Interface;
	}

	namespace Saphir
	{
		class MeshShader;
		class TaskShader;

		namespace Generator
		{
			class Abstract;
		}
	}
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief Returns the GLSL expression of the InstanceTransforms slot in a mesh-shading surface's stages: the raw
	 * uint bits of the surface view vec4's w (a mesh workgroup has no gl_InstanceIndex).
	 * @return const char * A static string (AbstractVertexStage::setInstanceIndexExpression()).
	 */
	[[nodiscard]]
	const char * meshSurfaceInstanceIndexExpression () noexcept;

	/** @brief The task payload variable shared by the two stages of a mesh-shading surface. */
	static constexpr auto MeshSurfacePayload{"msSurface"};

	/**
	 * @brief Writes the task and mesh stages of a MESH-SHADING surface (Graphics::Geometry::MeshShadingSurface).
	 * @note The TASK stage: one workgroup per tile (dispatch = tile counts), which picks the tile's subdivision
	 * from the camera distance — about MeshShadingSurfaceDetail metres of quad per metre of distance, a power of
	 * two up to MeshShadingSurface::MaxTileSubdivision — and launches its meshlets. Beyond the material's
	 * geometry-to-parallax handover the relief is parallax only, so the tile is one flat quad.
	 * The MESH stage: one meshlet of at most 8 × 8 quads, plus the SKIRTS of the tile edges it owns (owner
	 * decision 2026-09-22: a vertical strip down to the deepest relief, which hides the cracks between two tiles
	 * of different subdivisions; both windings). It PROVIDES the vertex attributes (position, flat frame, UV)
	 * to the shared per-vertex synthesis; the depth is the MATERIAL's (Material::Interface::
	 * generateSurfaceDisplacementCode()), the same relief as its parallax.
	 * @warning The matrices push-constant block, holding the two surface vec4, must be declared on BOTH stages
	 * first (Generator::Abstract::declareMatrixPushConstantBlock()).
	 * @param generator The generator (material set, push constants).
	 * @param material The material that owns the relief.
	 * @param taskShader The task stage.
	 * @param meshShader The mesh stage.
	 * @return bool
	 */
	[[nodiscard]]
	bool generateMeshShadingSurface (const Abstract & generator, const Graphics::Material::Interface & material, TaskShader & taskShader, MeshShader & meshShader) noexcept;
}
