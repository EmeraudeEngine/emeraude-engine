/*
 * src/Scenes/GPUMeshMetaData.hpp
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
#include <cstdint>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Scenes
{
	/** @brief Maximum number of sub-geometries (= materials) per BLAS, packed inline in
	 * GPUMeshMetaData. Lets the RT trace shader look up the right material per hit via
	 * `materialIndices[rayQueryGetIntersectionGeometryIndexEXT(...)]` without needing a
	 * separate per-sub-geometry SSBO. Raise this if a renderable needs more than 4
	 * distinct materials. */
	constexpr uint32_t MaxSubGeometriesPerMesh{4};

	/**
	 * @brief GPU-side per-instance mesh metadata for ray tracing shaders (std430 layout).
	 * @note Indexed by TLAS instanceCustomIndex. Provides the RT shader with device
	 *       addresses for vertex/index data, byte offsets for attribute fetch, and one
	 *       material index per sub-geometry of the BLAS. The shader looks up the right
	 *       material via materialIndices[geometryIndex] where geometryIndex comes from
	 *       rayQueryGetIntersectionGeometryIndexEXT.
	 *       Total size: 48 bytes (3 uvec4) for std430 alignment.
	 */
	struct GPUMeshMetaData
	{
		/** @brief Device address of the vertex buffer (VBO). */
		VkDeviceAddress vertexBufferAddress{0};
		/** @brief Device address of the index buffer (IBO). 0 if non-indexed. */
		VkDeviceAddress indexBufferAddress{0};
		/** @brief Vertex stride in bytes (floats per vertex * sizeof(float)). */
		uint32_t vertexStride{0};
		/** @brief Byte offset to primary UV coordinates within a vertex. */
		uint32_t primaryUVByteOffset{0};
		/** @brief Byte offset to normal (or tangent space start) within a vertex. */
		uint32_t normalByteOffset{0};
		/** @brief Number of valid entries in materialIndices (rest are unused). */
		uint32_t subGeometryCount{0};
		/** @brief One material index per sub-geometry. Indexed in the shader by the value
		 * returned by rayQueryGetIntersectionGeometryIndexEXT (0..subGeometryCount-1). */
		uint32_t materialIndices[MaxSubGeometriesPerMesh]{};
	};

	static_assert(sizeof(GPUMeshMetaData) == 48, "GPUMeshMetaData must be 48 bytes for std430.");
	static_assert(sizeof(GPUMeshMetaData) % 16 == 0, "GPUMeshMetaData must be 16-byte aligned for std430.");
}
