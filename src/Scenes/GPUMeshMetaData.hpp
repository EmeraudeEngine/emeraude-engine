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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <array>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Scenes
{
	/**
	 * @brief GPU-side per-sub-geometry row for ray tracing shaders (std430 layout).
	 * @note One row per VkAccelerationStructureGeometryKHR of the instance's BLAS, in BLAS
	 *	   order, so `rayQueryGetIntersectionGeometryIndexEXT` indexes it directly once the
	 *	   instance's GPUMeshMetaData::subGeometryTableOffset is added.
	 *
	 *	   ⚠️ firstIndex is NOT decoration: a multi-sub-geometry BLAS is partitioned with a
	 *	   `primitiveOffset` per geometry (Geometry::Interface::buildAccelerationStructure,
	 *	   RenderableInstance::Abstract::createRTSkinnedGeometryResources), so the primitive
	 *	   index a ray query returns is relative to ITS geometry while the index buffer is
	 *	   SHARED by all of them. A hit shader that indexes the shared buffer with the raw
	 *	   primitive index reads another sub-geometry's triangle — wrong vertices, wrong UVs,
	 *	   wrong normals. Measured on the two-layer palm (leaves = sub-geometry 0 with 596
	 *	   faces, bark = sub-geometry 1): every reflected trunk hit read one of the first 80
	 *	   LEAF triangles, which stretched the bark texture over leaf UVs and left hard dark
	 *	   bands where the borrowed normals faced away from the lights (fixed 2026-09-13).
	 *	   Total size: 8 bytes (uvec2) for std430 alignment.
	 */
	struct EMEN_API GPUSubGeometryData
	{
		/** @brief First index of the sub-geometry inside the SHARED index buffer, i.e. the
		 * BLAS geometry's primitiveOffset expressed in indices. */
		uint32_t firstIndex{0};
		/** @brief Index of the sub-geometry's material in the RT material SSBO. */
		uint32_t materialIndex{0};
	};

	static_assert(sizeof(GPUSubGeometryData) == 8, "GPUSubGeometryData must be 8 bytes for std430.");

	/**
	 * @brief GPU-side per-instance mesh metadata for ray tracing shaders (std430 layout).
	 * @note Indexed by TLAS instanceCustomIndex. Provides the RT shader with device
	 *	   addresses for vertex/index data, byte offsets for attribute fetch, and the range
	 *	   of the sub-geometry table (GPUSubGeometryData) describing the BLAS.
	 *
	 *	   The table is an INDIRECTION on purpose (owner decision, 2026-09-13): the material
	 *	   indices used to be packed inline in a fixed `uint32_t[4]`, which silently gave the
	 *	   wrong material to every sub-geometry past the fourth — Humans/OldMan carries seven.
	 *	   A per-instance offset into a shared table has no such ceiling and keeps the entry
	 *	   size constant.
	 *	   Total size: 48 bytes (3 uvec4) for std430 alignment.
	 */
	struct EMEN_API GPUMeshMetaData
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
		/** @brief Number of rows this instance owns in the sub-geometry table. Always >= 1. */
		uint32_t subGeometryCount{0};
		/** @brief Index of this instance's FIRST row in the sub-geometry table. The row of a
		 * hit is `subGeometryTableOffset + rayQueryGetIntersectionGeometryIndexEXT(...)`. */
		uint32_t subGeometryTableOffset{0};
		/** @brief Padding to the 16-byte std430 alignment. Free for a future per-instance field. */
		std::array< uint32_t, 3 > reserved{};
	};

	static_assert(sizeof(GPUMeshMetaData) == 48, "GPUMeshMetaData must be 48 bytes for std430.");
	static_assert(sizeof(GPUMeshMetaData) % 16 == 0, "GPUMeshMetaData must be 16-byte aligned for std430.");
}
