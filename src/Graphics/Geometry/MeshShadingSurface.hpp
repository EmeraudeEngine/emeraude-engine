/*
 * src/Graphics/Geometry/MeshShadingSurface.hpp
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

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief The contract of a MESH-SHADING surface (Geometry flag EnableMeshShadingSurface): a flat rectangle
	 * cut into square tiles, re-tessellated every frame by a task + mesh program and displaced by the
	 * material's height map.
	 * @note Everything the draw needs to push, and every constant the GLSL is generated with. The shaders are
	 * written by Saphir::Generator::MeshShadingSurfaceHelper; the geometry keeps its ordinary flat vertex
	 * buffer, which is what a device without VK_EXT_mesh_shader draws (with the material's POM), what the
	 * ray tracing traces and what the physics stands on.
	 * @note One TASK workgroup per tile (dispatch = tileCountX × tileCountZ); it picks the tile's subdivision
	 * from the camera distance and launches (subdivision / MeshletQuads)² MESH workgroups, one meshlet of at
	 * most MeshletQuads × MeshletQuads quads each. Owner decisions 2026-09-22, engine item
	 * mesh-shader-displaced-surface.
	 */
	struct MeshShadingSurface
	{
		/** @brief Quads per side of one meshlet: 8 × 8 quads = 81 vertices, 128 triangles (NVIDIA's advice: ≤ 128). */
		static constexpr uint32_t MeshletQuads{8};
		/** @brief Vertices per side of one meshlet. */
		static constexpr uint32_t MeshletVertices{MeshletQuads + 1};
		/** @brief Threads of the task and mesh workgroups. */
		static constexpr uint32_t WorkgroupSize{32};
		/** @brief The finest subdivision of a tile, in quads per side: 16 × 16 meshlets. */
		static constexpr uint32_t MaxTileSubdivision{128};

		/** @brief World X of the surface's first tile corner. */
		float originX{0.0F};
		/** @brief World Z of the surface's first tile corner. */
		float originZ{0.0F};
		/** @brief Side of one tile, in metres. */
		float tileSize{1.0F};
		/** @brief Texture repeats per metre: the flat grid's UV is (world − origin) × uvPerMeter. */
		float uvPerMeter{1.0F};
		/** @brief Tiles along X. */
		uint32_t tileCountX{0};
		/** @brief Tiles along Z. */
		uint32_t tileCountZ{0};
	};
}
