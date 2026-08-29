/*
 * src/Graphics/Effects/Framebuffer/RTAlphaTestGLSL.hpp
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
 * declares: `meshSSBO`, `materialSSBO`, `textures2D`, `MeshAccessor getMeshAccessor()`,
 * `vec2 getHitUV()`, and the material flag constants `IsAlphaTest`, `HasOpacityTexture`,
 * `HasAlbedoTexture`.
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

/** @brief GLSL: the function deciding whether a candidate triangle is solid at the hit texel. */
#define EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(
/* Material index of a hit, clamped to the renderable's material slots (a BLAS may carry more
 * sub-geometries than the renderable has materials — animated sprite frame groups). */
uint rtHitMaterialIndex (uint instanceIndex, uint geomIdx)
{
	uint subGeoCount = meshSSBO.meshEntries[instanceIndex * 3u + 1u].w;
	uint effectiveIdx = (geomIdx < subGeoCount) ? geomIdx : 0u;
	return meshSSBO.meshEntries[instanceIndex * 3u + 2u][effectiveIdx];
}

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

	MeshAccessor mesh = getMeshAccessor(instanceIndex, primitiveIndex);
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
