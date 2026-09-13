/*
 * src/Graphics/Effects/Shared/RTAlphaTestGLSL.hpp
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

/**
 * @file
 * @brief The ONE alpha-test candidate rule every ray query of the engine applies.
 *
 * ⚠️ A cutout (glTF alphaMode MASK, foliage, fences, sprites) is a TLAS instance flagged
 * FORCE_NO_OPAQUE: the hardware returns its triangles as CANDIDATES and the shader decides.
 * A ray launched with gl_RayFlagsOpaqueEXT overrides that flag and accepts every triangle
 * whole — a leaf becomes a solid quad, a fence a wall. RTGI did exactly that on BOTH its rays
 * (the bounce and the shadow ray) until Aug 2026, and RTR on its shadow ray: the ivy of Sponza
 * blocked the sky for the GI and cast a solid shadow at every bounce hit while the raster drew
 * it as leaves.
 *
 * The rule is a MACRO holding a string literal so it concatenates into the effects' own GLSL
 * literals at compile time — the shaders are `constexpr` literals, and a `constexpr const
 * char *` cannot be spliced into one. It relies on names EVERY RT effect shader already
 * declares: `meshSSBO`, `materialSSBO`, `textures2D`, `MeshAccessor getMeshAccessor(instance,
 * geomIdx, primitive)`, `vec2 getHitUV()`, the material flag constants `IsAlphaTest`,
 * `HasOpacityTexture`, `HasAlbedoTexture`, and the sub-geometry lookups of
 * EMEN_RT_SUBGEOMETRY_GLSL (`rtHitFirstIndex`, `rtHitMaterialIndex`) — which a shader carrying
 * its own accessor must splice BEFORE that accessor.
 *
 * An effect that declares none of those names itself (an occlusion-only effect) splices
 * EMEN_RT_SCENE_DATA_GLSL(bindlessSet) first — it brings exactly the declarations the rule needs.
 *
 * Usage, for ANY ray of ANY RT effect:
 * @code
 *   rayQueryInitializeEXT(q, topLevelAS, gl_RayFlagsNoneEXT | <other flags>, 0xFF, o, tMin, d, tMax);
 *   while (rayQueryProceedEXT(q)) { EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(q) }
 * @endcode
 * ⚠️ NEVER gl_RayFlagsOpaqueEXT: it silences the candidates this loop exists to judge.
 * gl_RayFlagsTerminateOnFirstHitEXT composes with it — a shadow ray ends at the first CONFIRMED
 * candidate, exactly as before, only it now confirms the right ones.
 */

/**
 * @brief GLSL: the sub-geometry table of the RT set (set 0, binding 4) and the two lookups every
 * hit needs — the first index of the hit geometry, and its material.
 * @note ⚠️ THE FIRST INDEX IS NOT OPTIONAL. A BLAS carrying several sub-geometries is built with
 * one VkAccelerationStructureGeometryKHR per sub-geometry, each with its own `primitiveOffset`
 * into ONE SHARED index buffer, so `rayQueryGetIntersectionPrimitiveIndexEXT` counts from the
 * start of the HIT geometry while the buffer is shared. Indexing the buffer with the raw
 * primitive index reads another sub-geometry's triangle: right material, wrong vertices, wrong
 * UVs, wrong normals. Measured on the two-layer palm of `light-and-shadow-debug` (leaves =
 * sub-geometry 0, 596 faces; bark = sub-geometry 1): every reflected trunk hit read one of the
 * first 80 LEAF triangles, stretching the bark over leaf UVs and leaving hard dark bands where
 * the borrowed normals faced away from the lights (fixed 2026-09-13).
 * @note Every consumer splices this BEFORE its own getMeshAccessor() — GLSL wants the
 * declaration first, and the accessor cannot fetch a triangle without the offset.
 */
#define EMEN_RT_SUBGEOMETRY_GLSL R"GLSL(
/* Sub-geometry table (set 0, binding 4): one uvec2 per geometry of every instance's BLAS, in
 * BLAS order — .x = first index in the shared index buffer, .y = material index. The instance's
 * own rows start at meshEntries[i*3+2].x and there are meshEntries[i*3+1].w of them. */
layout(set = 0, binding = 4) readonly buffer SubGeometryData { uvec2 subGeometries[]; } subGeoSSBO;

/* Row of a hit in the sub-geometry table. A BLAS may carry more geometries than the instance has
 * rows (it never should, but a stale metadata frame must not read another instance's rows): the
 * clamp keeps the lookup inside this instance. */
uint rtSubGeometryRow (uint instanceIndex, uint geomIdx)
{
	uint subGeoCount = meshSSBO.meshEntries[instanceIndex * 3u + 1u].w;
	uint effectiveIdx = geomIdx < subGeoCount ? geomIdx : 0u;

	return meshSSBO.meshEntries[instanceIndex * 3u + 2u].x + effectiveIdx;
}

/* First index of the hit sub-geometry in the SHARED index buffer: what turns the geometry-relative
 * primitive index a ray query returns into an offset the index buffer can be read at. */
uint rtHitFirstIndex (uint instanceIndex, uint geomIdx)
{
	return subGeoSSBO.subGeometries[rtSubGeometryRow(instanceIndex, geomIdx)].x;
}

/* Material of the hit sub-geometry. */
uint rtHitMaterialIndex (uint instanceIndex, uint geomIdx)
{
	return subGeoSSBO.subGeometries[rtSubGeometryRow(instanceIndex, geomIdx)].y;
}
)GLSL"

/**
 * @brief GLSL: the scene data the alpha-test rule reads, for an effect that does not declare it itself.
 * @note RTGI and RTR carry their own (richer) copies of these declarations — they read normals,
 * emission, roughness at hits. An occlusion-only effect (RTAO, ContactShadows) needs exactly this
 * much: the mesh/material SSBOs of the Renderer's RT set (set 0, bindings 1-3, next to the TLAS
 * at binding 0), the bindless 2D textures, and the UV fetch. `bindlessSet` is the descriptor set
 * index the effect binds the BindlessTextureManager's set at.
 * @warning Requires the extensions GL_EXT_buffer_reference2, GL_EXT_buffer_reference_uvec2,
 * GL_EXT_scalar_block_layout and GL_EXT_nonuniform_qualifier in the host shader.
 */
#define EMEN_RT_SCENE_DATA_GLSL(bindlessSet) R"GLSL(
/* Buffer reference types for vertex/index data access via device addresses. */
layout(buffer_reference, scalar) readonly buffer VertexBuffer { float v[]; };
layout(buffer_reference, scalar) readonly buffer IndexBuffer { uint i[]; };

/* RT scene data (set 0, the Renderer's set — the TLAS sits at binding 0). */
layout(set = 0, binding = 1) readonly buffer MeshMetaData { uvec4 meshEntries[]; } meshSSBO;
layout(set = 0, binding = 2) readonly buffer MaterialData { vec4 materials[]; } materialSSBO;
)GLSL" EMEN_RT_SUBGEOMETRY_GLSL R"GLSL(

/* Bindless 2D textures (BindlessTextureManager::Texture2DBinding). */
layout(set = )GLSL" #bindlessSet R"GLSL(, binding = 1) uniform sampler2D textures2D[];

/* Material flag bits (must match GPURTMaterialData). */
const uint HasAlbedoTexture = 1u << 0;
const uint HasOpacityTexture = 1u << 7;
const uint IsAlphaTest = 1u << 8;

vec2 readVertexVec2 (VertexBuffer vb, uint vertexIndex, uint strideFloats, uint attrOffsetFloats)
{
	uint base = vertexIndex * strideFloats + attrOffsetFloats;
	return vec2(vb.v[base], vb.v[base + 1u]);
}

struct MeshAccessor
{
	VertexBuffer vb;
	IndexBuffer ib;
	uint strideFloats;
	uint uvOffsetFloats;
	uint idx0, idx1, idx2;
};

/* GPUMeshMetaData layout: 3 uvec4 per instance (see RTR.cpp for details).
 * The primitive index is relative to the HIT sub-geometry, the index buffer is shared by all of
 * them: rtHitFirstIndex() is what rebases one onto the other. */
MeshAccessor getMeshAccessor (uint instanceIndex, uint geomIdx, uint primitiveIndex)
{
	MeshAccessor m;
	uvec4 meta0 = meshSSBO.meshEntries[instanceIndex * 3u];
	uvec4 meta1 = meshSSBO.meshEntries[instanceIndex * 3u + 1u];
	m.vb = VertexBuffer(uvec2(meta0.x, meta0.y));
	m.ib = IndexBuffer(uvec2(meta0.z, meta0.w));
	m.strideFloats = meta1.x / 4u;
	m.uvOffsetFloats = meta1.y / 4u;
	uint base = rtHitFirstIndex(instanceIndex, geomIdx) + primitiveIndex * 3u;
	m.idx0 = m.ib.i[base];
	m.idx1 = m.ib.i[base + 1u];
	m.idx2 = m.ib.i[base + 2u];
	return m;
}

vec2 getHitUV (MeshAccessor m, vec2 bary)
{
	vec2 uv0 = readVertexVec2(m.vb, m.idx0, m.strideFloats, m.uvOffsetFloats);
	vec2 uv1 = readVertexVec2(m.vb, m.idx1, m.strideFloats, m.uvOffsetFloats);
	vec2 uv2 = readVertexVec2(m.vb, m.idx2, m.strideFloats, m.uvOffsetFloats);
	return uv0 * (1.0 - bary.x - bary.y) + uv1 * bary.x + uv2 * bary.y;
}
)GLSL"

/** @brief GLSL: the function deciding whether a candidate triangle is solid at the hit texel. */
#define EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(
/* THE alpha-test rule: does this candidate triangle exist at the hit texel ?
 * Non-alpha-tested materials are solid (the BLAS default). An alpha-tested one is sampled at
 * the candidate's UV — opacity texture first, albedo alpha next, scalar albedo alpha last —
 * and exists only above the material's own cutoff (glTF alphaCutoff, raster parity). */
bool rtCandidateIsSolid (uint instanceIndex, uint geomIdx, uint primitiveIndex, vec2 bary)
{
	uint matBase = rtHitMaterialIndex(instanceIndex, geomIdx) * 7u;
	uint flags = floatBitsToUint(materialSSBO.materials[matBase + 4u].w);

	if ((flags & IsAlphaTest) == 0u)
	{
		return true;
	}

	MeshAccessor mesh = getMeshAccessor(instanceIndex, geomIdx, primitiveIndex);
	vec2 uv = getHitUV(mesh, bary);
	float alpha = materialSSBO.materials[matBase].a;

	if ((flags & HasOpacityTexture) != 0u)
	{
		int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 6u].y);

		if (texIndex >= 0)
		{
			alpha = texture(textures2D[nonuniformEXT(texIndex)], uv).r;
		}
	}
	else if ((flags & HasAlbedoTexture) != 0u)
	{
		int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].x);

		if (texIndex >= 0)
		{
			alpha = texture(textures2D[nonuniformEXT(texIndex)], uv).a;
		}
	}

	return alpha >= materialSSBO.materials[matBase + 6u].z;
}
)GLSL"

/** @brief GLSL: the body of a `while (rayQueryProceedEXT(q))` loop — confirms the solid candidates. */
#define EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(query) \
	"if (rayQueryGetIntersectionTypeEXT(" #query ", false) != gl_RayQueryCandidateIntersectionTriangleEXT) { continue; }\n" \
	"if (rtCandidateIsSolid(rayQueryGetIntersectionInstanceCustomIndexEXT(" #query ", false), rayQueryGetIntersectionGeometryIndexEXT(" #query ", false), rayQueryGetIntersectionPrimitiveIndexEXT(" #query ", false), rayQueryGetIntersectionBarycentricsEXT(" #query ", false))) { rayQueryConfirmIntersectionEXT(" #query "); }\n"
