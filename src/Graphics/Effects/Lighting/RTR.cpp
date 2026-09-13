/*
 * src/Graphics/Effects/Lighting/RTR.cpp
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

#include "RTR.hpp"

/* Local inclusions. */
#include "Graphics/Effects/Shared/IrradianceProbesGLSL.hpp"
#include "Graphics/Effects/Shared/RTAlphaTestGLSL.hpp"

/* STL inclusions. */
#include <algorithm>
#include <bit>
#include <cmath>

/* Local inclusions. */
#include "Graphics/IrradianceProbeVolume.hpp"
#include "Graphics/Renderer.hpp"
#include "Scenes/LightSet.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace
{
	using namespace EmEn;

	/* RTR trace pass: traces reflection rays using GL_EXT_ray_query.
	 * On hit, samples the bindless albedo texture at the interpolated UV.
	 * Falls back to scalar albedo when no texture is available.
	 *
	 * Descriptor set 0 (RT data — bound from Renderer::rtDescriptorSet()):
	 *   binding 0: accelerationStructureEXT (TLAS)
	 *   binding 1: RTMeshMetaData SSBO
	 *   binding 2: RTMaterialData SSBO
	 *   binding 3: RTLightData SSBO
	 *   binding 4: RTSubGeometryData SSBO (first index + material, per BLAS geometry)
	 *
	 * Descriptor set 1 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: environment cubemap (miss fallback)
	 *
	 * Descriptor set 2 (bindless textures — from BindlessTextureManager):
	 *   binding 1: sampler2D[] (2D texture array)
	 */
	constexpr auto RTRTraceFragmentShader = R"GLSL(
#version 460
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outReflection;

/* Buffer reference types for vertex/index data access via device addresses. */
layout(buffer_reference, scalar) readonly buffer VertexBuffer { float v[]; };
layout(buffer_reference, scalar) readonly buffer IndexBuffer { uint i[]; };

/* RT data (set 0). */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 1) readonly buffer MeshMetaData
{
	/* Each entry = 2 uvec4 (32 bytes):
	 *   uvec4[0]: vertexBufferAddress(lo,hi) + indexBufferAddress(lo,hi)
	 *   uvec4[1]: vertexStride(u32) + primaryUVByteOffset(u32)
	 *			 + normalByteOffset(u32) + materialIndex(u32) */
	uvec4 meshEntries[];
} meshSSBO;

layout(set = 0, binding = 2) readonly buffer MaterialData
{
	vec4 materials[];
} materialSSBO;

/* Light SSBO (set 0, binding 3).
 * Each light = 4 vec4 (64 bytes):
 *   vec4[0]: colorR, colorG, colorB, intensity
 *   vec4[1]: posX, posY, posZ, radius
 *   vec4[2]: dirX, dirY, dirZ, type (0=dir, 1=point, 2=spot)
 *   vec4[3]: innerCosAngle, outerCosAngle, pad, pad */
layout(set = 0, binding = 3) readonly buffer LightData
{
	vec4 lights[];
} lightSSBO;

)GLSL" EMEN_RT_SUBGEOMETRY_GLSL R"GLSL(

/* Input textures (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform samplerCube envCubemap;
layout(set = 1, binding = 3) uniform sampler2D albedoTex;
/* Hit data map, written per trace pixel: R = glossy cone width (the combine sizes its pyramid
 * lookup from it instead of a screen-uniform guess), G = hit distance hitT (the temporal
 * accumulation reprojects the reflected content through the virtual point P + V·hitT; 0 = no
 * reflection here, maxDistance on a sky miss so the environment reprojects as a far point). */
layout(set = 1, binding = 4, rg16f) uniform writeonly image2D coneImage;

/* Bindless textures (set 2). Binding 1 = 2D texture array, binding 3 = cube array
 * (reserved slots: 1 = scene irradiance E/pi, 2 = GGX-prefiltered environment). */
layout(set = 2, binding = 1) uniform sampler2D textures2D[];
layout(set = 2, binding = 3) uniform samplerCube texturesCube[];

/* Irradiance probe volume (set 3): the radiance cache, read at every hit for the indirect diffuse
 * the reflected surface receives — on screen or not. See Effects/Shared/IrradianceProbesGLSL.hpp. */
)GLSL" EMEN_IRRADIANCE_PROBES_GLSL(3) R"GLSL(

/* Per-frame parameters (set 1, binding 5). A UBO and NOT push constants: this block is 148
 * bytes, above the 128-byte Vulkan minimum guarantee for maxPushConstantsSize, so the pipeline
 * layout failed to create on any device exposing exactly 128 (part of the AMD/Intel fleet) and
 * RTR was never created there -- the scene silently lost its reflections.
 * ⚠️ The member list and its order are UNCHANGED on purpose: in std140 a vec3 has a base
 * alignment of 16 and a size of 12, so every `vec3 + float` pair packs into 16 bytes exactly
 * like the C++ `float[3] + float` it mirrors. The offsets are pinned by static_assert in
 * RTR.hpp -- a mismatch here reads garbage with no compile error. */
layout(set = 1, binding = 5, std140) uniform TraceParams
{
	mat4 invViewProj;
	vec3 invViewCol0; float viewPosX;
	vec3 invViewCol1; float viewPosY;
	vec3 invViewCol2; float viewPosZ;
	float maxDistance;
	float intensity;
	float fadeScreenEdge;
	uint lightCount;
	vec4 ambientLight;
	float coneScale;
};

/* Material flag bits (must match GPURTMaterialData). */
const uint HasAlbedoTexture   = 1u << 0;
const uint HasNormalTexture   = 1u << 1;
const uint HasRoughnessTexture = 1u << 2;
const uint HasMetalnessTexture = 1u << 3;
const uint HasEmissionTexture = 1u << 4;
const uint IsEmissive		 = 1u << 6;
const uint HasOpacityTexture  = 1u << 7;
const uint IsAlphaTest		= 1u << 8;
const uint RoughnessTexInverted = 1u << 9;
/* Texel source channel of the roughness/metalness textures, packed as 2-bit indices
 * (0:R, 1:G, 2:B, 3:A) — matches GPURTMaterialData::RoughnessChannelShift/MetalnessChannelShift. */
const uint RoughnessChannelShift = 16u;
const uint MetalnessChannelShift = 18u;
const uint ChannelMask = 3u;

const float PI = 3.14159265359;
/* Prefiltered environment mip count - 1 (IBLTexture::PrefilteredMipLevels). */
const float PrefilteredMaxLod = 5.0;


/* GGX/Smith/Schlick — the same microfacet family as the raster PBR pass, so a
 * reflected surface matches the directly rendered one. */
float distributionGGX (float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 0.0001);
}

float geometrySmith (float NdotV, float NdotL, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float gv = NdotV / (NdotV * (1.0 - k) + k);
	float gl = NdotL / (NdotL * (1.0 - k) + k);
	return gv * gl;
}

vec3 fresnelSchlick (float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/* Light type constants. */
const float LIGHT_DIRECTIONAL = 0.0;
const float LIGHT_POINT = 1.0;
const float LIGHT_SPOT = 2.0;

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(fadeScreenEdge), uv)
			  * smoothstep(vec2(0.0), vec2(fadeScreenEdge), vec2(1.0) - uv);
	return fade.x * fade.y;
}

/* Read vertex attribute (vec3) from vertex buffer at given float offset. */
vec3 readVertexVec3 (VertexBuffer vb, uint vertexIndex, uint strideFloats, uint attrOffsetFloats)
{
	uint base = vertexIndex * strideFloats + attrOffsetFloats;
	return vec3(vb.v[base], vb.v[base + 1u], vb.v[base + 2u]);
}

/* Read vertex attribute (vec2) from vertex buffer at given float offset. */
vec2 readVertexVec2 (VertexBuffer vb, uint vertexIndex, uint strideFloats, uint attrOffsetFloats)
{
	uint base = vertexIndex * strideFloats + attrOffsetFloats;
	return vec2(vb.v[base], vb.v[base + 1u]);
}

/* Shared mesh data unpacking — returns VB, IB refs and offsets via out params. */
struct MeshAccessor
{
	VertexBuffer vb;
	IndexBuffer ib;
	uint strideFloats;
	uint normalOffsetFloats;
	uint uvOffsetFloats;
	uint idx0, idx1, idx2;
};

/* GPUMeshMetaData layout (3 uvec4 = 48 bytes per instance):
 *   [0] = (vbAddrLo, vbAddrHi, ibAddrLo, ibAddrHi)
 *   [1] = (strideBytes, uvOffsetBytes, normalOffsetBytes, subGeometryCount)
 *   [2] = (subGeometryTableOffset, reserved x3) — the instance's first row in the
 *								 sub-geometry table (set 0, binding 4), which a hit
 *								 addresses with rayQueryGetIntersectionGeometryIndexEXT
 *
 * ⚠️ primitiveIndex is relative to the HIT sub-geometry while the index buffer is SHARED by all
 * of them: rtHitFirstIndex() rebases it. Without that the trunk of the two-layer palm read the
 * leaves' triangles (see EMEN_RT_SUBGEOMETRY_GLSL). */
MeshAccessor getMeshAccessor (uint instanceIndex, uint geomIdx, uint primitiveIndex)
{
	MeshAccessor m;

	uvec4 meta0 = meshSSBO.meshEntries[instanceIndex * 3u];
	uvec4 meta1 = meshSSBO.meshEntries[instanceIndex * 3u + 1u];

	m.vb = VertexBuffer(uvec2(meta0.x, meta0.y));
	m.ib = IndexBuffer(uvec2(meta0.z, meta0.w));
	m.strideFloats = meta1.x / 4u;
	m.uvOffsetFloats = meta1.y / 4u;
	m.normalOffsetFloats = meta1.z / 4u;

	uint base = rtHitFirstIndex(instanceIndex, geomIdx) + primitiveIndex * 3u;

	m.idx0 = m.ib.i[base];
	m.idx1 = m.ib.i[base + 1u];
	m.idx2 = m.ib.i[base + 2u];

	return m;
}

/* Interpolate a vec3 vertex attribute at the hit point (barycentric). */
vec3 getHitAttributeVec3 (MeshAccessor m, vec2 bary, uint offsetFloats)
{
	vec3 a0 = readVertexVec3(m.vb, m.idx0, m.strideFloats, offsetFloats);
	vec3 a1 = readVertexVec3(m.vb, m.idx1, m.strideFloats, offsetFloats);
	vec3 a2 = readVertexVec3(m.vb, m.idx2, m.strideFloats, offsetFloats);

	return a0 * (1.0 - bary.x - bary.y) + a1 * bary.x + a2 * bary.y;
}

/* Interpolate the shading normal at the hit point.
 * ⚠️ Returns vec3(0.0) — never a NaN — when the interpolation degenerates (zero vertex normals,
 * or a mesh table row not yet filled): normalize(0) is NaN, that NaN became the ORIGIN of the
 * shadow rays traced from the hit, and a NaN ray query is a DEVICE LOSS on NVIDIA. The caller
 * resolves a zero normal to a finite fallback (facing the ray). */
vec3 getHitNormal (MeshAccessor m, vec2 bary)
{
	vec3 n = getHitAttributeVec3(m, bary, m.normalOffsetFloats);
	float lengthSquared = dot(n, n);

	return lengthSquared > 1e-12 ? n * inversesqrt(lengthSquared) : vec3(0.0);
}

/* Interpolate UV at hit point. */
vec2 getHitUV (MeshAccessor m, vec2 bary)
{
	vec2 uv0 = readVertexVec2(m.vb, m.idx0, m.strideFloats, m.uvOffsetFloats);
	vec2 uv1 = readVertexVec2(m.vb, m.idx1, m.strideFloats, m.uvOffsetFloats);
	vec2 uv2 = readVertexVec2(m.vb, m.idx2, m.strideFloats, m.uvOffsetFloats);

	return uv0 * (1.0 - bary.x - bary.y) + uv1 * bary.x + uv2 * bary.y;
}

/* RAY-CONE TEXTURE LOD at the hit -- Akenine-Moller, Nilsson, Andersson, Barre-Brisebois, Toth and
 * Karras, "Texture Level of Detail Strategies for Real-Time Ray Tracing", Ray Tracing Gems,
 * chapter 20 (Apress, 2019): the ray-cone method, re-derived here, no code taken.
 * `texture()` in this pass had UNDEFINED derivatives: hitUV is not continuous across a quad (two
 * neighbouring trace pixels can hit unrelated triangles) and the lookups sit in divergent control
 * flow, so the hardware picked an arbitrary mip -- in practice the finest. The reflection of a
 * textured wall was a field of point samples on a texture minified several times over, which the
 * TAA jitter turned into a shimmer: measured 2026-09-13 on light-and-shadow-debug, the static brick
 * cube's reflection moved 80x more than the mirror floor around it, its energy on the mortar lines.
 * The triangle constant below is half the log2 of the hit triangle's UV-area over world-area ratio,
 * texture-independent; rtTextureLod() adds half the log2 of one texture's texel count. */
float rtTriangleLodBase (MeshAccessor m, mat4x3 objectToWorld)
{
	vec3 p0 = objectToWorld * vec4(readVertexVec3(m.vb, m.idx0, m.strideFloats, 0u), 1.0);
	vec3 p1 = objectToWorld * vec4(readVertexVec3(m.vb, m.idx1, m.strideFloats, 0u), 1.0);
	vec3 p2 = objectToWorld * vec4(readVertexVec3(m.vb, m.idx2, m.strideFloats, 0u), 1.0);
	vec2 t0 = readVertexVec2(m.vb, m.idx0, m.strideFloats, m.uvOffsetFloats);
	vec2 t1 = readVertexVec2(m.vb, m.idx1, m.strideFloats, m.uvOffsetFloats);
	vec2 t2 = readVertexVec2(m.vb, m.idx2, m.strideFloats, m.uvOffsetFloats);

	/* Twice the areas on both sides: only the ratio matters. */
	float worldArea = length(cross(p1 - p0, p2 - p0));
	vec2 e1 = t1 - t0;
	vec2 e2 = t2 - t0;
	float uvArea = abs(e1.x * e2.y - e1.y * e2.x);

	return 0.5 * log2(max(uvArea, 1e-12) / max(worldArea, 1e-12));
}

/* The LOD of ONE texture at the hit: the cone footprint on the surface (hitLod carries the
 * footprint in metres, the incidence and the triangle constant) expressed in that texture's texels. */
float rtTextureLod (int texIndex, float hitLod)
{
	ivec2 size = textureSize(textures2D[nonuniformEXT(texIndex)], 0);

	return hitLod + 0.5 * log2(float(size.x * size.y));
}

)GLSL" EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(
/* Shadow ray: returns 1.0 when the path from the surface toward the light is unoccluded,
 * 0.0 otherwise. TerminateOnFirstHit: the first CONFIRMED candidate ends the traversal.
 * ⚠️ NOT gl_RayFlagsOpaqueEXT: the reflection ray below judged its alpha-tested candidates
 * while this one accepted them whole — a leaf shadowed a reflected surface as a solid quad.
 * Both rays now apply the ONE shared rule (RTAlphaTestGLSL.hpp). */
float shadowRayVisibility (vec3 origin, vec3 direction, float maxT)
{
	/* ⚠️ The spec forbids a NaN operand (VUID-RuntimeSpirv-OpRayQueryInitializeKHR-06351) and
	 * NVIDIA answers one with a DEVICE LOSS — measured on Sponza under GPU-assisted validation,
	 * 2026-09-13: a degenerate hit normal made the shadow-ray origin NaN, and 22 point lights
	 * turned one bad hit into 22 illegal queries. A non-finite ray has no meaningful occluder. */
	if (any(isnan(origin)) || any(isinf(origin)) || any(isnan(direction)))
	{
		return 1.0;
	}

	rayQueryEXT shadowQuery;
	rayQueryInitializeEXT(
		shadowQuery, topLevelAS,
		gl_RayFlagsTerminateOnFirstHitEXT, 0xFF,
		origin, 0.0, direction, maxT
	);

	while (rayQueryProceedEXT(shadowQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(shadowQuery) R"GLSL(
	}

	return rayQueryGetIntersectionTypeEXT(shadowQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

/* Compute direct lighting at the reflection hit point (Lambert diffuse over all scene lights).
 * Each contribution is gated by a shadow ray: without the occlusion test, every hit point
 * received the light straight through walls — shadows simply did not exist INSIDE the
 * reflections (a reflected shadowed area looked fully lit). Same fix as RTGI. */
vec3 computeDirectLighting (vec3 hitPos, vec3 hitNormal, vec3 V, vec3 albedo, float roughnessHit, float metalnessHit, vec3 F0)
{
	/* Shadow ray origin offset along the hit normal (no bias push constant in RTR). */
	const float ShadowRayBias = 0.01;

	vec3 totalLight = vec3(0.0);

	for (uint i = 0u; i < lightCount; i++)
	{
		uint base = i * 4u;
		vec4 colorIntensity = lightSSBO.lights[base];
		vec4 posRadius = lightSSBO.lights[base + 1u];
		vec4 dirType = lightSSBO.lights[base + 2u];

		vec3 lightColor = colorIntensity.rgb * colorIntensity.a;
		float type = dirType.w;

		vec3 L;
		float attenuation = 1.0;
		/* Directional lights: any hit toward the light occludes, whatever the distance. */
		float shadowDistance = 10000.0;

		if (type < 0.5)
		{
			/* Directional light: direction is pre-computed, no attenuation. */
			L = normalize(-dirType.xyz);
		}
		else
		{
			/* Point or spot light: compute direction from position. */
			vec3 toLight = posRadius.xyz - hitPos;
			float dist = length(toLight);
			L = toLight / max(dist, 0.0001);
			shadowDistance = dist;

			/* Distance attenuation with radius falloff — the RASTER curve, verbatim:
			 * `max(1 - dot(d/r, d/r), 0)` (LightGenerator.PBR.cpp). The squared-falloff form
			 * written here, `clamp(1 - d/r, 0, 1)²`, is a DIFFERENT curve: at half the radius it
			 * returns 0.25 where the raster returns 0.75, so a reflected surface was lit by a
			 * third of the light the rendered one received at mid-range and the two images
			 * diverged with distance. A light with no radius is not attenuated at all in the
			 * raster, so it is not attenuated here either. */
			float radius = posRadius.w;

			if (radius > 0.0)
			{
				float distanceRatio = dist / radius;
				attenuation = max(1.0 - distanceRatio * distanceRatio, 0.0);
			}

			/* Spot light cone. */
			if (type > 1.5)
			{
				vec4 spotParams = lightSSBO.lights[base + 3u];
				float innerCos = spotParams.x;
				float outerCos = spotParams.y;
				float cosAngle = dot(-L, normalize(dirType.xyz));
				attenuation *= clamp((cosAngle - outerCos) / max(innerCos - outerCos, 0.0001), 0.0, 1.0);
			}
		}

		float NdotL = max(dot(hitNormal, L), 0.0);

		/* Skip the shadow ray when the light cannot contribute anyway. */
		if (NdotL * attenuation <= 0.0)
		{
			continue;
		}

		float visibility = 1.0;

		/* Only shadow-ray the lights that cast shadows in the raster passes (flag in the
		 * 4th SSBO vec4): a light without a shadow map deliberately shines through geometry
		 * on screen, and the reflection must match the rendered scene — otherwise the
		 * reflections show shadows that do not exist in the image. */
		if (lightSSBO.lights[base + 3u].z > 0.5)
		{
			vec3 shadowOrigin = hitPos + hitNormal * ShadowRayBias;
			visibility = shadowRayVisibility(shadowOrigin, L, shadowDistance);
		}

		/* Lambert diffuse + GGX specular (Cook-Torrance), energy split by metalness. */
		vec3 H = normalize(L + V);
		float NdotH = max(dot(hitNormal, H), 0.0);
		float NdotV = max(dot(hitNormal, V), 0.0001);
		float VdotH = max(dot(V, H), 0.0);

		float D = distributionGGX(NdotH, roughnessHit);
		float G = geometrySmith(NdotV, NdotL, roughnessHit);
		vec3 F = fresnelSchlick(VdotH, F0);

		vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
		/* ⚠️ LAMBERT NORMALISATION. The outgoing luminance of a diffuse surface is `albedo * E / PI`,
		 * and that is what the raster emits (`kD * albedo / 3.14159265`, LightGenerator.PBR.cpp) and
		 * what RTGI emits (`albedo / PI`). RTR was the only member of the family without it, so every
		 * reflected surface came out PI times too bright — measured 2026-09-13 on light-and-shadow-debug,
		 * where the mirror floor showed the brick cube at 2.26x the luminance of the cube ITSELF while
		 * the screen-space lane, which re-reads the rendered image, showed the physical 0.40x. A passive
		 * mirror cannot outshine its source: the contract is that a reflected surface matches the
		 * rendered one. */
		vec3 diffuse = albedo * (1.0 - metalnessHit) * (vec3(1.0) - F) / PI;

		totalLight += lightColor * (diffuse + specular) * NdotL * attenuation * visibility;
	}

	return totalLight;
}
)GLSL" R"GLSL(

void main()
{
	/* Use texelFetch (no bilinear filtering) to avoid interpolating
	 * depth/normals across geometric edges at half-resolution. */
	vec2 fullResSize = vec2(textureSize(depthTex, 0));
	ivec2 fullResCoord = ivec2(vUV * fullResSize);
	/* Cone width defaults to 0 (sharp): every early return below and the miss branch keep it. */
	ivec2 coneCoord = ivec2(gl_FragCoord.xy);
	imageStore(coneImage, coneCoord, vec4(0.0));
	float depth = texelFetch(depthTex, fullResCoord, 0).r;

	/* Skip far-plane fragments. */
	if (depth >= 1.0)
	{
		outReflection = vec4(0.0);
		return;
	}

	/* Read view-space normal and packed roughness+metalness from MRT.
	 * Alpha encoding: alpha = roughness + metalness * 2.0
	 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0; */
	vec4 normalData = texelFetch(normalTex, fullResCoord, 0);
	vec3 rawN = normalData.rgb;
	float packedRM = normalData.a;
	float originMetalness = packedRM >= 2.0 ? 1.0 : 0.0;
	float roughness = packedRM - originMetalness * 2.0;

	if (dot(rawN, rawN) < 0.0001)
	{
		outReflection = vec4(0.0);
		return;
	}

	/* Progressive roughness fade-out instead of a hard cutoff. With the cone-scaled
	 * bilateral blur (radius ∝ roughness², see the blur pass), mid-roughness surfaces
	 * keep a physically blurred reflection — only the truly diffuse tail retires. */
	float roughnessFade = 1.0 - smoothstep(0.6, 0.9, roughness);

	if (roughnessFade <= 0.0)
	{
		outReflection = vec4(0.0);
		return;
	}

	/* Reconstruct world-space position from NDC + depth via inverse VP.
	 * ⚠️ The NDC of the TEXEL whose depth was fetched, not of this half-res pixel's centre: the two
	 * differ by half a full-res pixel, and unprojecting one with the depth of the other lands off the
	 * surface by that offset times the depth slope -- on a floor seen at a grazing angle, a ray origin
	 * biased along the view ray on every pixel (fixed 2026-09-13). */
	vec2 ndc = ((vec2(fullResCoord) + 0.5) / fullResSize) * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 wp = invViewProj * clipPos;
	vec3 worldPos = wp.xyz / wp.w;

	/* Transform view-space normal to world space. */
	mat3 invViewRot = mat3(invViewCol0, invViewCol1, invViewCol2);
	vec3 worldNormal = normalize(invViewRot * normalize(rawN));

	/* Compute world-space reflection direction. */
	vec3 cameraPos = vec3(viewPosX, viewPosY, viewPosZ);
	vec3 viewDir = normalize(worldPos - cameraPos);
	vec3 reflDir = reflect(viewDir, worldNormal);

	/* Fresnel (Schlick), PRIMARY surface, F0 as a COLOR: a metal tints its reflection by
	 * its albedo (gold reflects gold, not a colorless mirror), a dielectric reflects the
	 * physical 4% head-on — the former scalar mix(0.15, 0.9, metalness) made every metal
	 * a WHITE mirror and boosted dielectrics 4x (measured: a dark-teal metal dome rendered
	 * at ~70% of the sky's luminance with the sky's own chromaticity).
	 * The scalar lobe weight keeps feeding the premultiplied confidence pipeline; the
	 * NORMALIZED tint rides on the traced color instead.
	 * Computed before the trace: it applies to both hit and environment-miss paths. */
	/* ⚠️ .rgb of the albedo attachment is the BASE colour (its .a is the diffuse weight the GI
	 * combines apply): a diffuse albedo here reads 0 for every metal and kills its reflection. */
	vec3 originAlbedo = texelFetch(albedoTex, fullResCoord, 0).rgb;
	vec3 F0 = mix(vec3(0.04), originAlbedo, originMetalness);
	float NdotV = max(dot(worldNormal, -viewDir), 0.0);
	vec3 fresnelColor = fresnelSchlick(NdotV, F0);
	float fresnel = max(fresnelColor.r, max(fresnelColor.g, fresnelColor.b));
	vec3 fresnelTint = fresnelColor / max(fresnel, 0.001);

	/* Offset ray origin along normal to prevent self-intersection. */
	vec3 rayOrigin = worldPos + worldNormal * 0.01;

	/* GLOSSY LOBE and camera distance, read twice in the hit branch: by the ray-cone texture LOD
	 * and by the cone-width map the combine sizes its pyramid lookup from. GGX with alpha =
	 * roughness²: the half-vector distribution falls to half its peak at tan(thetaH) = 0.6436 alpha,
	 * and the REFLECTED direction deviates by 2 thetaH, so the lobe is tan(2 thetaH) wide
	 * (half-width) around the mirror direction. */
	float coneAlpha = roughness * roughness;
	float coneTanHalf = 0.6436 * coneAlpha;
	float coneTan = 2.0 * coneTanHalf / max(1.0 - coneTanHalf * coneTanHalf, 1e-3);
	float coneDistCam = length(worldPos - cameraPos);

	/* Trace reflection ray.
	 * Ray flag is NoneEXT (not OpaqueEXT) so candidate intersections on TLAS instances
	 * flagged FORCE_NO_OPAQUE (alpha-test materials) are returned to us for confirmation
	 * via rayQueryProceedEXT. We then sample the opacity texture at the candidate's
	 * barycentrics and confirm only if the texel is above the material's alphaCutoff —
	 * letting rays pass through the transparent texels of foliage, sprites, etc. */
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(
		rayQuery, topLevelAS, gl_RayFlagsNoneEXT, 0xFF,
		rayOrigin, 0.001, reflDir, maxDistance
	);

	while (rayQueryProceedEXT(rayQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(rayQuery) R"GLSL(
	}

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
		uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
		uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
		vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

		/* Unpack mesh data. The sub-geometry decides BOTH which triangle is read (its first
		 * index rebases the geometry-relative primitive index onto the shared index buffer)
		 * and which material shades it — a BLAS may hold several, e.g. palm trunk +
		 * alpha-test leaves. */
		uint geomIdx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
		MeshAccessor mesh = getMeshAccessor(instanceIndex, geomIdx, primitiveIndex);
		uint materialIndex = rtHitMaterialIndex(instanceIndex, geomIdx);
		uint matBase = materialIndex * 7u;

		vec3 albedo = materialSSBO.materials[matBase].rgb;
		uint flags = floatBitsToUint(materialSSBO.materials[matBase + 4u].w);

		/* Compute world-space hit position and geometric normal. */
		vec3 hitPos = rayOrigin + reflDir * hitT;

		/* Transform object-space normal to world space.
		 * VBO normals are already in engine convention (Y-UP, right-handed).
		 * Apply objectToWorld for rotated/scaled instances. */
		vec3 objectNormal = getHitNormal(mesh, barycentrics);
		mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
		vec3 worldNormal = mat3(objectToWorld) * objectNormal;
		float worldNormalLengthSquared = dot(worldNormal, worldNormal);
		/* A degenerate normal (zero attribute, zero-scale instance) faces the ray: finite, so the
		 * shadow rays below stay legal (see getHitNormal). */
		vec3 hitNormal = worldNormalLengthSquared > 1e-12 ? worldNormal * inversesqrt(worldNormalLengthSquared) : -reflDir;

		/* Reject true numerical self-intersection: hit normal nearly identical to origin
		 * normal AND hitT minuscule (ray hits the same triangle it started from due to
		 * imperfect normal-offset). Real reflections from other geometry with parallel
		 * normals — cube tops reflected in floor below, ceilings in floor, walls in
		 * parallel walls — are LEGITIMATE and must not be rejected. */
		if (dot(hitNormal, worldNormal) > 0.99 && hitT < 0.05)
		{
			outReflection = vec4(0.0);
			return;
		}

		vec2 hitUV = getHitUV(mesh, barycentrics);

		/* RAY-CONE TEXTURE LOD (rtTriangleLodBase). The cone leaves the camera one trace texel wide
		 * (1 / coneScale radians), reaches the reflector at coneDistCam, keeps its spread across a
		 * planar mirror bounce and widens by the GGX lobe (2 x the lobe's half-width tangent, the same
		 * quantity the cone-width map is built from) over the reflected leg hitT. Divided by the
		 * incidence cosine on the hit triangle -- the GEOMETRIC normal, before any normal mapping,
		 * which is what the footprint stretches on. The reflector is taken planar, as for the cone
		 * map: a curved one would add its own spread. Every textured read of this hit goes through
		 * textureLod() with this footprint -- see the note on rtTriangleLodBase(). */
		float coneWidthAtHit = (coneDistCam + hitT) / coneScale + 2.0 * coneTan * hitT;
		float hitLod = rtTriangleLodBase(mesh, objectToWorld) + log2(max(coneWidthAtHit, 1e-6)) - log2(max(abs(dot(hitNormal, reflDir)), 0.05));

		/* Sample bindless albedo texture if available. */
		if ((flags & HasAlbedoTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].x);

			if (texIndex >= 0)
			{
				albedo = textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, rtTextureLod(texIndex, hitLod)).rgb;
			}
		}

		/* ---- Enriched hit shading (uber-shader): the FULL material model, data-driven
		 * from the RT material SSBO — no program duplication, one parametric BRDF. ---- */

		/* Normal mapping at the hit: perturb the geometric normal through the material's
		 * normal texture when the mesh carries tangent space. The engine vertex layout is
		 * Position(3)-Tangent(3)-Binormal(3)-Normal(3) whenever TBN is present, so
		 * normalOffsetFloats == 9 IS the TBN presence signal — the same layout contract
		 * SceneMetaData's offset computation and the skinning mirror already rely on
		 * (tangent at float 3, binormal at float 6). Decode matches the raster
		 * (StandardResource): raw = rgb * 2 - 1, XY scaled by the material normalScale. */
		if ((flags & HasNormalTexture) != 0u && mesh.normalOffsetFloats == 9u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].y);

			if (texIndex >= 0)
			{
				vec3 rawNormal = textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, rtTextureLod(texIndex, hitLod)).rgb * 2.0 - 1.0;
				float normalScale = materialSSBO.materials[matBase + 6u].w;
				vec3 tangentSpaceNormal = normalize(vec3(rawNormal.xy * normalScale, rawNormal.z));

				vec3 hitTangent = normalize(mat3(objectToWorld) * getHitAttributeVec3(mesh, barycentrics, 3u));
				vec3 hitBinormal = normalize(mat3(objectToWorld) * getHitAttributeVec3(mesh, barycentrics, 6u));

				hitNormal = normalize(hitTangent * tangentSpaceNormal.x + hitBinormal * tangentSpaceNormal.y + hitNormal * tangentSpaceNormal.z);
			}
		}

		/* Roughness / metalness: the scalar is the value when no texture drives the
		 * property, and the MULTIPLYING factor otherwise (glTF 'factor * texel' contract,
		 * same as the raster components). The texel source channel is carried in the
		 * flags (glTF packed metallic-roughness: roughness = G, metalness = B). */
		float hitRoughness = materialSSBO.materials[matBase + 1u].x;
		float hitMetalness = materialSSBO.materials[matBase + 1u].y;

		if ((flags & HasRoughnessTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].z);

			if (texIndex >= 0)
			{
				float roughnessTexel = textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, rtTextureLod(texIndex, hitLod))[(flags >> RoughnessChannelShift) & ChannelMask];

				/* Smoothness/gloss source: invert before the factor applies (raster parity). */
				hitRoughness *= ((flags & RoughnessTexInverted) != 0u) ? (1.0 - roughnessTexel) : roughnessTexel;
			}
		}

		if ((flags & HasMetalnessTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].w);

			if (texIndex >= 0)
			{
				hitMetalness *= textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, rtTextureLod(texIndex, hitLod))[(flags >> MetalnessChannelShift) & ChannelMask];
			}
		}

		vec3 hitF0 = mix(vec3(0.04), albedo, hitMetalness);
		vec3 hitV = -reflDir;
		float hitNdotV = max(dot(hitNormal, hitV), 0.0001);

		/* Direct lighting: Lambert diffuse + GGX specular per light, shadow-ray gated
		 * (same microfacet family as the raster pass — the reflection matches the image). */
		vec3 litColor = computeDirectLighting(hitPos, hitNormal, hitV, albedo, hitRoughness, hitMetalness, hitF0);

		/* Ambient at hit: the scene scalar ambient, the INDIRECT DIFFUSE the hit receives from the
		 * rest of the scene, and the sky IBL's prefiltered specular tap (slot 2, roughness-driven
		 * LOD, scaled by the sky luminance — a normalized source).
		 * The indirect diffuse comes from the irradiance probe volume (Sep 2026): the reflected world
		 * had NONE before — a GI-lit surface reflected as black in a closed room (measured on
		 * global-illumination: the green column's face at (0,65,0) in the direct view, 0.07 in the
		 * mirror). The probes integrate the sky with visibility, so while they run the raster IBL
		 * diffuse leg (slot 1, unoccluded sky) is theirs — adding both would count the sky twice, the
		 * same ownership rule RTGI applies to the primary surfaces (iblDiffuseWeight). When the
		 * volume is disabled the IBL leg is back, exactly as before. */
		{
			vec3 hitR = reflect(reflDir, hitNormal);
			vec3 prefiltered = textureLod(texturesCube[nonuniformEXT(2)], hitR, clamp(hitRoughness, 0.0, 1.0) * PrefilteredMaxLod).rgb;
			vec3 iblSpecular = prefiltered * fresnelSchlick(hitNdotV, hitF0);

			/* Coverage: 1 inside the volume, fading to 0 over its last cell, 0 when disabled. Where the
			 * probes cannot answer (a hit outside the camera-centred volume) the IBL diffuse leg stays,
			 * as before the volume existed: an unoccluded sky beats black. */
			float probeCoverage = probeVolume.skyAmbient.y * probeVolumeWeight(hitPos);
			vec3 diffuseAlbedo = albedo * (1.0 - hitMetalness);
			/* The query returns energy; the IndirectDiffuse intensity the primary surfaces get is applied here. */
			vec3 probeDiffuse = diffuseAlbedo * probeIrradiance(hitPos, hitNormal, hitV) * probeVolume.ambientColor.w;
			vec3 iblDiffuse = diffuseAlbedo * texture(texturesCube[nonuniformEXT(1)], hitNormal).rgb * ambientLight.w * (1.0 - probeCoverage);

			/* The scene ambient is an ILLUMINANCE in lux, so a Lambertian surface sends back
			 * `albedo * E / PI` — the same 1/PI the raster applies (`iblBaseColor * 0.3183098862`,
			 * LightGenerator.cpp). The two irradiance sources below already carry theirs: the probe
			 * query and the bindless irradiance cube both store E/PI. */
			litColor += albedo * ambientLight.rgb / PI;
			litColor += probeDiffuse + iblDiffuse + iblSpecular * ambientLight.w;
		}

		/* Emission: the material's own light, texture-modulated when present. */
		if ((flags & IsEmissive) != 0u)
		{
			vec3 emission = materialSSBO.materials[matBase + 3u].rgb * materialSSBO.materials[matBase + 4u].x;

			if ((flags & HasEmissionTexture) != 0u)
			{
				int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 6u].x);

				if (texIndex >= 0)
				{
					emission *= textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, rtTextureLod(texIndex, hitLod)).rgb;
				}
			}

			litColor += emission;
		}

		/* Distance fade: reflection fades as hit gets further from the surface. */
		float distFade = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);

		float confidence = distFade * fresnel * roughnessFade;

		/* GLOSSY CONE v2 — the width of the pyramid lookup, per pixel, in TRACE texels (the lobe
		 * tangent and the camera distance are computed before the trace, see above). The lobe paints a
		 * footprint hitT * tan(theta) on what it hits; seen through the reflector from a camera at
		 * distance dCam, the footprint shrinks by dCam / (dCam + hitT) on the reflector and projects
		 * to coneScale / dCam texels per metre — hence tan(theta) * hitT * coneScale / (dCam + hitT).
		 * A contact reflection (hitT -> 0) stays sharp; a distant one tends to the lobe's own
		 * angular size. The v1 cone was screen-uniform (2 x 0.15 x traceHeight x roughness²) and
		 * measured 2-3x too sharp for hits 6-16 m away while blurring contacts that must stay sharp
		 * (projet-alpha post-processor-effect-debug bench, 2026-08-30). Flat reflector assumed: a
		 * curved one compresses the image and would want the normal's screen-space derivative. */
		float coneWidth = 2.0 * coneTan * hitT * coneScale / max(coneDistCam + hitT, 1e-3);
		imageStore(coneImage, coneCoord, vec4(coneWidth, hitT, 0.0, 0.0));

		outReflection = vec4(litColor * fresnelTint * confidence, confidence);
	}
	else
	{
		/* Ray escaped the scene: reflect the ACTIVE SCENE's prefiltered environment
		 * (bindless reserved cube slot 2, always current), roughness-driven LOD, scaled
		 * by the sky luminance — a normalized source becomes nits. The former dedicated
		 * envCubemap binding fell back to the renderer DEFAULT cubemap when the caller
		 * passed none (dark sky in every reflection, measured on the bench). */
		/* ENGINE CUBEMAP CONVENTION (Y-UP): a world direction samples the cubemap RAW. reflDir IS
		 * a world direction — it is built from worldNormal and (worldPos - cameraPos), and it is
		 * handed straight to rayQueryInitializeEXT against the world-space TLAS above.
		 * ⚠️ This site kept a `vec3(D.x, -D.y, D.z)` negation until Aug 2026, long after the Y-up
		 * flip deleted its twins in SSR, RTGI, the skybox and the LightGenerator. It survived
		 * because it only affects the MISS path: rays that escape the scene. Measured on
		 * `reflexion-debug --demo-options 0,5,0` with a clear-horizon sky, the mirror sphere's TOP
		 * — where rays leave toward the sky — reflected the dark GROUND instead (luminance 85
		 * against 174 for the real sky, blue-minus-green -2.8 against +9.6). */
		vec3 envColor = textureLod(texturesCube[nonuniformEXT(2)], reflDir, clamp(roughness, 0.0, 1.0) * PrefilteredMaxLod).rgb * ambientLight.w;
		float confidence = fresnel * roughnessFade;

		/* A sky miss reflects content at infinity: the temporal reprojection treats it as a far
		 * point (rotation-consistent, no parallax), which is what maxDistance gives it. */
		imageStore(coneImage, coneCoord, vec4(0.0, maxDistance, 0.0, 0.0));

		outReflection = vec4(envColor * fresnelTint * confidence, confidence);
	}
}
)GLSL";

	/* Reflection pyramid downsample: 4 bilinear taps at the corners of the destination
	 * texel's source footprint — a 4x4 tent, converging toward a gaussian across the
	 * chain (same pre-convolution as the SSR color pyramid). Operates on the
	 * PREMULTIPLIED trace output (color·confidence, confidence): the composite's
	 * division by the filtered confidence renormalizes edge bleed. */
	constexpr auto RTRPyramidDownsampleComputeShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D srcColor;
layout(set = 0, binding = 1, rgba16f) uniform writeonly image2D dstMip;

layout(push_constant) uniform PushConstants
{
	int destWidth;
	int destHeight;
	int sourceMaxX;
	int sourceMaxY;
};

void main()
{
	ivec2 p = ivec2(gl_GlobalInvocationID.xy);

	if (p.x >= destWidth || p.y >= destHeight)
	{
		return;
	}

	vec2 srcSize = vec2(float(sourceMaxX + 1), float(sourceMaxY + 1));
	vec2 invSrc = 1.0 / srcSize;
	vec2 uv = (vec2(p) * 2.0 + 1.0) * invSrc;

	vec4 color = 0.25 * (
		texture(srcColor, uv + vec2(-0.5, -0.5) * invSrc) +
		texture(srcColor, uv + vec2( 0.5, -0.5) * invSrc) +
		texture(srcColor, uv + vec2(-0.5,  0.5) * invSrc) +
		texture(srcColor, uv + vec2( 0.5,  0.5) * invSrc));

	imageStore(dstMip, p, color);
}
)GLSL";

	/* The trace input descriptor set layout (set 1) is the effect's own: 4 samplers + the cone map. */
	constexpr auto TraceInputLayoutId{"RTR_TraceInput"};
	constexpr uint32_t ConeImageBinding{4U};
	constexpr uint32_t TraceParamsBinding{5U};

	/* Push constants of the pyramid build dispatches. */
	struct PyramidPushConstants
	{
		int32_t destWidth;
		int32_t destHeight;
		int32_t sourceMaxX;
		int32_t sourceMaxY;
	};
}

namespace EmEn::Graphics::Effects::Lighting
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RTR::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* Pixel doubling: half-res for performance (default), full-res for quality.
		 * NOTE: this alone cannot sharpen the reflection of a glossy surface — the cone width
		 * below is expressed in TRACE texels and scales with the trace height, so the cone LOD
		 * exactly cancels the resolution gain. Use the GlossyCone knobs for that. */
		const auto pixelDoubling = settings.getOrSetDefault< bool >(GraphicsPPReflectionsRTPixelDoublingKey, DefaultGraphicsPPReflectionsRTPixelDoubling);
		const auto halfW = pixelDoubling ? (width > 1 ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? (height > 1 ? height / 2 : 1U) : height;

		/* Glossy cone controls (bench knobs — see SettingKeys.hpp for the full rationale). */
		m_coneEnabled = settings.getOrSetDefault< bool >(GraphicsPPReflectionsRTGlossyConeEnabledKey, DefaultGraphicsPPReflectionsRTGlossyConeEnabled);
		m_coneBlendStart = std::max(0.0F, settings.getOrSetDefault< float >(GraphicsPPReflectionsRTGlossyConeBlendStartKey, DefaultGraphicsPPReflectionsRTGlossyConeBlendStart));
		m_coneBlendFull = std::max(m_coneBlendStart, settings.getOrSetDefault< float >(GraphicsPPReflectionsRTGlossyConeBlendFullKey, DefaultGraphicsPPReflectionsRTGlossyConeBlendFull));
		m_coneMaxLod = std::max(0.0F, settings.getOrSetDefault< float >(GraphicsPPReflectionsRTGlossyConeMaxLodKey, DefaultGraphicsPPReflectionsRTGlossyConeMaxLod));

		/* Temporal accumulation knobs (SettingKeys.hpp § Reflections/RayTracing/Temporal). */
		m_parameters.temporalEnabled = settings.getOrSetDefault< bool >(GraphicsPPReflectionsRTTemporalEnabledKey, DefaultGraphicsPPReflectionsRTTemporalEnabled);
		m_parameters.temporalAlpha = std::clamp(settings.getOrSetDefault< float >(GraphicsPPReflectionsRTTemporalAlphaKey, DefaultGraphicsPPReflectionsRTTemporalAlpha), 0.01F, 1.0F);
		m_parameters.temporalDepthTolerance = std::max(0.001F, settings.getOrSetDefault< float >(GraphicsPPReflectionsRTTemporalDepthToleranceKey, DefaultGraphicsPPReflectionsRTTemporalDepthTolerance));
		m_parameters.temporalNormalThreshold = std::clamp(settings.getOrSetDefault< float >(GraphicsPPReflectionsRTTemporalNormalThresholdKey, DefaultGraphicsPPReflectionsRTTemporalNormalThreshold), -1.0F, 1.0F);
		m_parameters.temporalVarianceGamma = std::max(0.0F, settings.getOrSetDefault< float >(GraphicsPPReflectionsRTTemporalVarianceGammaKey, DefaultGraphicsPPReflectionsRTTemporalVarianceGamma));
		m_parameters.temporalMaxAccumulation = std::max(1U, settings.getOrSetDefault< uint32_t >(GraphicsPPReflectionsRTTemporalMaxAccumulationKey, DefaultGraphicsPPReflectionsRTTemporalMaxAccumulation));

		/* Trace target (half-res by default, RGBA16F: reflected color RGB + confidence A). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTR_Trace") )
		{
			TraceError{ClassId} << "Failed to create RTR trace target !";

			return false;
		}

		/* The temporal accumulation of the RAW trace: the shared GI denoiser in REFLECTION mode
		 * (see GIDenoiser::Parameters::reflectionMode), at the trace resolution, without its à-trous
		 * — the RTR keeps its own roughness-scaled bilateral and the glossy pyramid downstream, both
		 * now fed by an integrated signal. Variance clipping ON: the trace is deterministic, so the
		 * clip costs no convergence and bounds the ghosting of a reflected object that moves. */
		m_denoiser.setTemporalEnabled(m_parameters.temporalEnabled);
		m_denoiser.setParameters(GIDenoiser::Parameters{
			.atrousIterations = 0,
			.temporalAlpha = m_parameters.temporalAlpha,
			.temporalDepthTolerance = m_parameters.temporalDepthTolerance,
			.temporalNormalThreshold = m_parameters.temporalNormalThreshold,
			.temporalVarianceGamma = m_parameters.temporalVarianceGamma,
			.maxAccumulation = m_parameters.temporalMaxAccumulation,
			.reflectionMode = true,
			.temporalNeighborhoodClamp = true,
			.temporalAnimatedNoise = false,
			.accumulationCounter = true
		});

		if ( !m_denoiser.create(halfW, halfH) )
		{
			TraceError{ClassId} << "Failed to create the RTR temporal denoiser component !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		/* ---- Hit data map: one RG16F texel per trace pixel (R = glossy cone width, G = hit
		 * distance), written by the trace fragment shader (storage image), sampled by the combine
		 * (R) and by the temporal accumulation (G). Same lifecycle as the pyramid. ---- */
		{
			m_coneImage = std::make_shared< Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_R16G16_SFLOAT,
				VkExtent3D{
					.width = halfW,
					.height = halfH,
					.depth = 1U
				},
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				1U
			);
			m_coneImage->setIdentifier(ClassId, "GlossyConeWidth", "Image");

			if ( !m_coneImage->createOnHardware() )
			{
				TraceError{ClassId} << "Failed to create the glossy cone width image !";

				return false;
			}

			/* Declared in the layout the combine samples it in; each frame moves it UNDEFINED ->
			 * GENERAL before the trace and GENERAL -> SHADER_READ_ONLY after (see recordPreDenoisePasses). */
			m_coneImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			m_coneView = std::make_shared< ImageView >(
				m_coneImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0U,
					.levelCount = 1U,
					.baseArrayLayer = 0U,
					.layerCount = 1U
				}
			);
			m_coneView->setIdentifier(ClassId, "GlossyConeWidth", "ImageView");

			if ( !m_coneView->createOnHardware() )
			{
				return false;
			}

			m_coneSampler = renderer.getSampler("RTRConeWidth", [] (Settings &, VkSamplerCreateInfo & samplerCreateInfo) {
				samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.anisotropyEnable = VK_FALSE;
				samplerCreateInfo.maxLod = 0.0F;
			});

			if ( m_coneSampler == nullptr )
			{
				return false;
			}
		}

		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTR_BlurH") )
		{
			TraceError{ClassId} << "Failed to create RTR blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTR_BlurV") )
		{
			TraceError{ClassId} << "Failed to create RTR blur V target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals + environment cubemap + albedo — 4 combined image
		 * samplers — plus the glossy cone width map the trace WRITES (binding 4, storage image).
		 * Own layout, shared through the layout manager under its own identifier. */
		auto traceInputLayout = layoutManager.getDescriptorSetLayout(TraceInputLayoutId);

		if ( traceInputLayout == nullptr )
		{
			traceInputLayout = layoutManager.prepareNewDescriptorSetLayout(TraceInputLayoutId);
			traceInputLayout->setIdentifier(ClassId, TraceInputLayoutId, "DescriptorSetLayout");

			for ( uint32_t bindingIndex = 0; bindingIndex < 4; ++bindingIndex )
			{
				traceInputLayout->declareCombinedImageSampler(bindingIndex, VK_SHADER_STAGE_FRAGMENT_BIT);
			}

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = ConeImageBinding;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				traceInputLayout->declare(binding);
			}

			/* The per-frame trace parameters (binding 5): a UBO, because the block is 148 bytes
			 * and push constants only guarantee 128. */
			traceInputLayout->declareUniformBuffer(TraceParamsBinding, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(traceInputLayout) )
			{
				TraceError{ClassId} << "Unable to create the trace input descriptor set layout !";

				return false;
			}
		}

		/* RT descriptor set layout (set 0) — from the Renderer. */
		auto rtLayout = renderer.rtDescriptorSetLayout();

		if ( rtLayout == nullptr )
		{
			TraceError{ClassId} << "RT descriptor set layout not available !";

			return false;
		}

		/* Bindless texture descriptor set layout (set 2) — from BindlessTextureManager. */
		auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( bindlessLayout == nullptr )
		{
			TraceError{ClassId} << "Bindless texture descriptor set layout not available !";

			return false;
		}

		/* Irradiance probe volume (set 3) — the renderer creates it with the acceleration structure
		 * builder, so it exists whenever this effect can. The reflected world takes its indirect
		 * diffuse from it: without it the effect is not created (the stack falls back to SSR). */
		const auto * probeVolume = renderer.irradianceProbeVolume();

		if ( probeVolume == nullptr || !probeVolume->usable() )
		{
			TraceError{ClassId} << "The irradiance probe volume is not available: the ray-traced reflections need it for the indirect diffuse at their hits !";

			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			/* Trace: set 0 = RT data, set 1 = depth + normals, set 2 = bindless textures, set 3 = probes. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);
			sets.emplace_back(probeVolume->descriptorSetLayout());

			/* No push constant range: the trace parameters live in the set 1 UBO above. */
			m_traceLayout = layoutManager.getPipelineLayout(sets, {});
		}

		if ( m_traceLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTR_Trace_FS", ShaderType::FragmentShader, RTRTraceFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTR shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "RTR_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);

		if ( m_tracePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		/* Trace: set 1 reads depth + normals (updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Binding 4: the glossy cone width map, written by the trace (storage image, GENERAL). */
		{
			VkDescriptorImageInfo coneInfo{};
			coneInfo.sampler = VK_NULL_HANDLE;
			coneInfo.imageView = m_coneView->handle();
			coneInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			for ( const auto & descriptorSet : m_tracePerFrame )
			{
				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSet->handle();
				write.dstBinding = ConeImageBinding;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.descriptorCount = 1;
				write.pImageInfo = &coneInfo;

				vkUpdateDescriptorSets(renderer.device()->handle(), 1, &write, 0, nullptr);
			}
		}

		/* Binding 5: the per-frame trace parameters. The image bindings are rewritten every
		 * frame, this one only once: the buffer handle never changes, only its content. */
		m_traceFrameUBOs = this->createPerFrameUniformBuffers(sizeof(TraceFrameUBOData), ClassId, "RTR_Trace_Frame_UBO");

		if ( m_traceFrameUBOs.size() != m_tracePerFrame.size() )
		{
			TraceError{ClassId} << "Failed to create the per-frame trace parameter buffers !";

			return false;
		}

		for ( size_t frameIndex = 0; frameIndex < m_tracePerFrame.size(); ++frameIndex )
		{
			if ( !m_tracePerFrame[frameIndex]->writeUniformBufferObject(TraceParamsBinding, *m_traceFrameUBOs[frameIndex]) )
			{
				TraceError{ClassId} << "Failed to bind the trace parameter buffer for frame " << frameIndex << " !";

				return false;
			}
		}

		/* Binding 2: environment cubemap for ray misses (fixed).
		 * NOTE: The scene cubemap may still be loading asynchronously at
		 * post-process setup time — binding a texture without its GPU image
		 * ready stalls the device. Fall back to the renderer default cubemap
		 * whenever the provided one is not created yet. */
		{
			auto cubemap = m_environmentCubemap != nullptr && m_environmentCubemap->isCreated()
				? m_environmentCubemap
				: renderer.getDefaultTextureCubemap();

			if ( cubemap == nullptr || !cubemap->isCreated() )
			{
				TraceError{ClassId} << "No environment cubemap available for the trace miss fallback !";

				return false;
			}

			for ( const auto & descriptorSet : m_tracePerFrame )
			{
				if ( !descriptorSet->writeCombinedImageSampler(2, *cubemap) )
				{
					return false;
				}
			}
		}

		/* ---- Pre-convolved reflection pyramid (glossy cone approximation) ---- */
		{
			const auto localDevice = renderer.device();

			const uint32_t pyramidBaseW = std::max(1U, m_traceTarget.width() / 2U);
			const uint32_t pyramidBaseH = std::max(1U, m_traceTarget.height() / 2U);
			m_pyramidMipCount = std::clamp(static_cast< uint32_t >(std::bit_width(std::min(pyramidBaseW, pyramidBaseH))) - 3U, 1U, 8U);

			m_pyramidImage = std::make_shared< Image >(
				localDevice,
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VkExtent3D{
					.width = pyramidBaseW,
					.height = pyramidBaseH,
					.depth = 1U
				},
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				m_pyramidMipCount
			);
			m_pyramidImage->setIdentifier(ClassId, "ReflectionPyramid", "Image");

			if ( !m_pyramidImage->createOnHardware() )
			{
				TraceError{ClassId} << "Failed to create the reflection pyramid image !";

				return false;
			}

			/* The combine pass binds the pyramid through the TextureInterface adapter, whose
			 * descriptor write reads Image::currentImageLayout(). The per-frame build cycle
			 * (UNDEFINED -> GENERAL -> SHADER_READ_ONLY, explicit-layout barriers) always
			 * leaves the pyramid in SHADER_READ_ONLY when the combine samples it — declare
			 * it once, like IntermediateRenderTarget does for its own image. */
			m_pyramidImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			m_pyramidMipViews.reserve(m_pyramidMipCount);

			for ( uint32_t mip = 0; mip < m_pyramidMipCount; mip++ )
			{
				auto view = std::make_shared< ImageView >(
					m_pyramidImage,
					VK_IMAGE_VIEW_TYPE_2D,
					VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = mip,
						.levelCount = 1U,
						.baseArrayLayer = 0U,
						.layerCount = 1U
					}
				);
				view->setIdentifier(ClassId, "ReflectionPyramidMip" + std::to_string(mip), "ImageView");

				if ( !view->createOnHardware() )
				{
					return false;
				}

				m_pyramidMipViews.emplace_back(view);
			}

			m_pyramidFullView = std::make_shared< ImageView >(
				m_pyramidImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0U,
					.levelCount = m_pyramidMipCount,
					.baseArrayLayer = 0U,
					.layerCount = 1U
				}
			);
			m_pyramidFullView->setIdentifier(ClassId, "ReflectionPyramidFull", "ImageView");

			if ( !m_pyramidFullView->createOnHardware() )
			{
				return false;
			}

			m_pyramidSampler = renderer.getSampler("RTRPyramid", [] (Settings &, VkSamplerCreateInfo & samplerCreateInfo) {
				samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				samplerCreateInfo.anisotropyEnable = VK_FALSE;
				samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
			});

			if ( m_pyramidSampler == nullptr )
			{
				return false;
			}

			/* Compute DS layout: binding 0 = sampled source, binding 1 = storage dest. */
			m_pyramidDSLayout = std::make_shared< DescriptorSetLayout >(localDevice, "RTRPyramidDSLayout");

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 0;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_pyramidDSLayout->declare(binding);
			}

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				m_pyramidDSLayout->declare(binding);
			}

			if ( !m_pyramidDSLayout->createOnHardware() )
			{
				return false;
			}

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(PyramidPushConstants);

			m_pyramidPipelineLayout = std::make_shared< PipelineLayout >(
				localDevice,
				"RTRPyramidPipelineLayout",
				StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_pyramidDSLayout},
				StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
			);

			if ( !m_pyramidPipelineLayout->createOnHardware() )
			{
				return false;
			}

			const auto downsampleModule = renderer.shaderManager().getShaderModuleFromSourceCode(localDevice, "RTR_PyramidDownsample_CS", ShaderType::ComputeShader, RTRPyramidDownsampleComputeShader);

			if ( downsampleModule == nullptr )
			{
				TraceError{ClassId} << "Failed to compile the reflection pyramid downsample shader !";

				return false;
			}

			m_pyramidDownsamplePipeline = std::make_unique< ComputePipeline >(m_pyramidPipelineLayout);
			m_pyramidDownsamplePipeline->setShaderModule(downsampleModule->handle());

			if ( !m_pyramidDownsamplePipeline->createOnHardware() )
			{
				return false;
			}

			/* One set per chain mip (1..N-1) plus the two base (mip 0) sets, one per source parity. */
			const auto pyramidSetCount = (m_pyramidMipCount > 0U ? m_pyramidMipCount - 1U : 0U) + static_cast< uint32_t >(m_pyramidBaseSets.size());

			const std::vector< VkDescriptorPoolSize > poolSizes{
				{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = pyramidSetCount},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = pyramidSetCount}
			};

			m_pyramidDescriptorPool = std::make_shared< DescriptorPool >(localDevice, poolSizes, pyramidSetCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

			if ( !m_pyramidDescriptorPool->createOnHardware() )
			{
				return false;
			}

			/* Base (mip 0) sets: their source is the TEMPORALLY RESOLVED reflection, whose ping-pong
			 * parity flips every frame — written on first use by pyramidBaseSlot(), never in flight. */
			for ( size_t slot = 0; slot < m_pyramidBaseSets.size(); ++slot )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_pyramidDescriptorPool, m_pyramidDSLayout);
				descriptorSet->setIdentifier(ClassId, "Pyramid_Base_DescSet" + std::to_string(slot), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					return false;
				}

				m_pyramidBaseSets[slot] = std::move(descriptorSet);
				m_pyramidBaseSources[slot] = nullptr;
			}

			/* Chain sets: previous mip (sampled in GENERAL during the chain) -> this mip. */
			m_pyramidSets.reserve(m_pyramidMipCount > 0U ? m_pyramidMipCount - 1U : 0U);

			for ( uint32_t mip = 1; mip < m_pyramidMipCount; mip++ )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_pyramidDescriptorPool, m_pyramidDSLayout);
				descriptorSet->setIdentifier(ClassId, "Pyramid_DescSet" + std::to_string(mip), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					return false;
				}

				this->writePyramidSet(*descriptorSet, m_pyramidMipViews[mip - 1]->handle(), VK_IMAGE_LAYOUT_GENERAL, m_pyramidSampler->handle(), *m_pyramidMipViews[mip]);

				m_pyramidSets.emplace_back(std::move(descriptorSet));
			}
		}

		return true;
	}

	void
	RTR::writePyramidSet (const DescriptorSet & descriptorSet, VkImageView sourceView, VkImageLayout sourceLayout, VkSampler sourceSampler, const ImageView & destView) const noexcept
	{
		VkDescriptorImageInfo sourceInfo{};
		sourceInfo.sampler = sourceSampler;
		sourceInfo.imageView = sourceView;
		sourceInfo.imageLayout = sourceLayout;

		VkDescriptorImageInfo destInfo{};
		destInfo.sampler = VK_NULL_HANDLE;
		destInfo.imageView = destView.handle();
		destInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		std::array< VkWriteDescriptorSet, 2 > writes{};

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet.handle();
		writes[0].dstBinding = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo = &sourceInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet.handle();
		writes[1].dstBinding = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &destInfo;

		vkUpdateDescriptorSets(this->renderer().device()->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);
	}

	size_t
	RTR::pyramidBaseSlot (const TextureInterface & source) noexcept
	{
		for ( size_t slot = 0; slot < m_pyramidBaseSources.size(); ++slot )
		{
			if ( m_pyramidBaseSources[slot] == &source )
			{
				return slot;
			}
		}

		for ( size_t slot = 0; slot < m_pyramidBaseSources.size(); ++slot )
		{
			if ( m_pyramidBaseSources[slot] == nullptr )
			{
				/* Sources are IRTs sampled in SHADER_READ_ONLY after their render pass. */
				this->writePyramidSet(*m_pyramidBaseSets[slot], source.imageView()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_pyramidSampler->handle(), *m_pyramidMipViews[0]);
				m_pyramidBaseSources[slot] = &source;

				return slot;
			}
		}

		/* A third distinct source cannot happen (two history parities, or the trace target alone).
		 * Should it ever, rewrite the slot of the frame parity and say so — a set rewritten in
		 * flight is a hazard worth a trace, not a silent one. */
		const size_t slot = this->renderer().currentFrameIndex() & 1U;

		TraceWarning{ClassId} << "A third reflection pyramid source appeared; rewriting base set #" << slot << " in flight !";

		this->writePyramidSet(*m_pyramidBaseSets[slot], source.imageView()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_pyramidSampler->handle(), *m_pyramidMipViews[0]);
		m_pyramidBaseSources[slot] = &source;

		return slot;
	}

	void
	RTR::destroy () noexcept
	{
		m_denoiser.destroy();
		m_temporalOutput = nullptr;

		for ( auto & baseSet : m_pyramidBaseSets )
		{
			baseSet.reset();
		}

		m_pyramidBaseSources.fill(nullptr);

		m_pyramidSets.clear();
		m_pyramidDescriptorPool.reset();
		m_pyramidDownsamplePipeline.reset();
		m_pyramidPipelineLayout.reset();
		m_pyramidDSLayout.reset();
		m_pyramidSampler.reset();
		m_pyramidFullView.reset();
		m_pyramidMipViews.clear();
		m_pyramidImage.reset();
		m_coneSampler.reset();
		m_coneView.reset();
		m_coneImage.reset();

		m_tracePerFrame.clear();

		m_traceFrameUBOs.clear();
		m_tracePipeline.reset();
		m_traceLayout.reset();

		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();
	}

	void
	RTR::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto * lightSet = context.lightSet;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Primary-surface albedo: requiresAlbedo() guarantees the attachment exists and the
		 * PostProcessor skips the effect while the pointer is null. */
		if ( context.albedo != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(3, *context.albedo));
		}

		/* Upgrade from the default cubemap once the scene environment finishes
		 * its asynchronous load (only the current frame's set is written — the
		 * other frames' sets may still be in flight). */
		if ( m_environmentCubemap != nullptr && m_environmentCubemap->isCreated() )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, *m_environmentCubemap));
		}

		/* ---- Pass 1: Ray Trace ---- */
		{
			/* Compute inverse view-projection for bulletproof NDC → world reconstruction.
			 * CRITICAL: Use the readStateIndex to get the SAME view matrix that produced
			 * the depth buffer. Using logicState would read the next tick's camera position,
			 * causing world position mismatch → flickering reflections. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
			const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
			const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
			const auto invViewProj = (projMat * viewMat).inverse();
			const auto * ivp = invViewProj.data();

			/* Inverse view rotation for normal transformation (view → world). */
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			/* Scene ambient (color × intensity), matching what the raster surfaces receive.
			 * ⚠️ The intensity is the EFFECTIVE one (FrameContext::ambientIlluminance), never
			 * LightSet::ambientLightIntensity(): when the sky drives the ambient the raster reads
			 * the irradiance cubemap and its scalar is ZERO, while the LightSet still holds the
			 * manifest's 17 000 lx. Reading the LightSet here added that flat ambient to every
			 * hit point ON TOP of the IBL below — the reflected world was brighter than the world
			 * it reflected, which breaks the "the reflection matches the raster" contract.
			 * Falls back to the previous neutral 0.15 grey when no light set is available. */
			const auto ambientColor = lightSet != nullptr ? lightSet->ambientLightColor() : Base::PixelFactory::Color< float >{0.15F, 0.15F, 0.15F, 1.0F};
			const auto ambientIntensity = lightSet != nullptr ? context.ambientIlluminance : 1.0F;

			const TraceFrameUBOData traceData{
				.invViewProj = {
					ivp[0], ivp[1], ivp[2], ivp[3],
					ivp[4], ivp[5], ivp[6], ivp[7],
					ivp[8], ivp[9], ivp[10], ivp[11],
					ivp[12], ivp[13], ivp[14], ivp[15]
				},
				.invViewCol0 = {inv[0], inv[1], inv[2]},
				.viewPosX = inv[12],
				.invViewCol1 = {inv[4], inv[5], inv[6]},
				.viewPosY = inv[13],
				.invViewCol2 = {inv[8], inv[9], inv[10]},
				.viewPosZ = inv[14],
				.maxDistance = m_parameters.maxDistance,
				.intensity = m_parameters.intensity,
				.fadeScreenEdge = m_parameters.fadeScreenEdge,
				.lightCount = this->renderer().rtLightCount(),
				.ambientR = ambientColor.red() * ambientIntensity,
				.ambientG = ambientColor.green() * ambientIntensity,
				.ambientB = ambientColor.blue() * ambientIntensity,
				.skyLuminance = context.skyLuminance,
				/* |P[1][1]| = 1 / tan(vFOV / 2) (column-major, element 5): focal length in trace texels. */
				.coneScale = std::abs(projMat.data()[5]) * 0.5F * static_cast< float >(m_traceTarget.height())
			};

			if ( !updateUniformBufferData(*m_traceFrameUBOs[frameIndex], &traceData, sizeof(traceData)) )
			{
				TraceError{ClassId} << "Failed to update the trace parameter buffer !";

				return;
			}

			/* Cone width map: UNDEFINED -> GENERAL for the trace's storage writes (content rewritten). */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_coneImage,
					0,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL
				};
				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}

			/* Custom recording: bind set 0 (RT) from Renderer, set 1 (input textures) per-frame. */
			m_traceTarget.beginRenderPass(commandBuffer);

			commandBuffer.bind(*m_tracePipeline);

			const VkViewport viewport{
				.x = 0.0F,
				.y = 0.0F,
				.width = static_cast< float >(m_traceTarget.width()),
				.height = static_cast< float >(m_traceTarget.height()),
				.minDepth = 0.0F,
				.maxDepth = 1.0F
			};

			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {
					.x = 0,
					.y = 0
				},
				.extent = {
					.width = m_traceTarget.width(),
					.height = m_traceTarget.height()
				}
			};

			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			/* Bind set 0: RT descriptor set (TLAS + SSBOs). */
			if ( const auto * rtDescSet = this->renderer().rtDescriptorSet(); rtDescSet != nullptr )
			{
				commandBuffer.bind(*rtDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			}

			/* Bind set 1: Input textures (depth + normals). */
			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

			/* Bind set 2: Bindless textures. */
			if ( const auto * bindlessDescSet = this->renderer().bindlessTextureManager().descriptorSet(); bindlessDescSet != nullptr )
			{
				commandBuffer.bind(*bindlessDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
			}

			/* Bind set 3: the irradiance probe volume of the frame (refreshed by the renderer before
			 * this chain runs; its parameters carry the enabled flag the shader consults). */
			if ( const auto * probeVolume = this->renderer().irradianceProbeVolume(); probeVolume != nullptr )
			{
				if ( const auto * probeSet = probeVolume->descriptorSet(frameIndex); probeSet != nullptr )
				{
					commandBuffer.bind(*probeSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 3);
				}
			}

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);

			/* Cone width map written -> sampled by the combine fragment shader. */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_coneImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};
				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}

		/* ---- Pass 1a: temporal accumulation of the RAW trace (2026-09-13) ----
		 * The shared GI denoiser in REFLECTION mode: history reprojected through the VIRTUAL position
		 * of the reflected point (P + V·hitT, hitT from the hit-data map the trace just wrote), blended
		 * toward the surface reprojection as the roughness grows, validated on the virtual distance,
		 * variance-clipped against the current 3x3. The bilateral blur and the glossy pyramid consume
		 * its output, so the whole glossy path integrates a STABLE signal. Why it exists: the trace is
		 * deterministic but half-res and fed by a G-buffer the TAA jitters — a reflected silhouette
		 * aliased at half resolution, and TAA cannot reproject a reflection (SettingKeys.hpp § Temporal).
		 * Returns the raw trace when the chain is off. */
		static_cast< void >(m_denoiser.updateFrameData(frameIndex, context, GIDenoiser::FrameInputs{}));
		m_temporalOutput = m_denoiser.recordResolve(commandBuffer, m_traceTarget, context, &m_coneTexture);

		/* ---- Pass 1b: pre-convolved reflection pyramid build (glossy cone source) ---- */
		if ( m_pyramidImage != nullptr )
		{
			/* The pyramid's base is the TEMPORALLY RESOLVED reflection (the raw trace when the chain is
			 * off): its parity flips every frame, so the mip-0 set is picked per source. */
			const auto * pyramidSource = m_temporalOutput != nullptr ? m_temporalOutput : static_cast< const TextureInterface * >(&m_traceTarget);
			const auto baseSlot = this->pyramidBaseSlot(*pyramidSource);

			/* Trace render pass writes -> compute sampling of the trace target. */
			{
				VkMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					commandBuffer.handle(),
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					1, &barrier,
					0, nullptr,
					0, nullptr
				);
			}

			/* Whole pyramid: UNDEFINED -> GENERAL (previous content discarded, fully rewritten). */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_pyramidImage,
					0,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
			}

			const auto & pyramidExtent = m_pyramidImage->createInfo().extent;
			const auto baseWidth = static_cast< int32_t >(pyramidExtent.width);
			const auto baseHeight = static_cast< int32_t >(pyramidExtent.height);

			commandBuffer.bind(*m_pyramidDownsamplePipeline);

			for ( uint32_t mip = 0; mip < m_pyramidMipCount; mip++ )
			{
				if ( mip > 0 )
				{
					/* Previous mip written -> readable by this downsample. */
					const Sync::ImageMemoryBarrier barrier{
						*m_pyramidImage,
						VK_ACCESS_SHADER_WRITE_BIT,
						VK_ACCESS_SHADER_READ_BIT,
						VK_IMAGE_LAYOUT_GENERAL,
						VK_IMAGE_LAYOUT_GENERAL
					};

					commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
				}

				const auto destWidth = std::max(1, baseWidth >> mip);
				const auto destHeight = std::max(1, baseHeight >> mip);
				const auto sourceWidth = mip == 0 ? static_cast< int32_t >(m_traceTarget.width()) : std::max(1, baseWidth >> (mip - 1));
				const auto sourceHeight = mip == 0 ? static_cast< int32_t >(m_traceTarget.height()) : std::max(1, baseHeight >> (mip - 1));

				const PyramidPushConstants pc{
					.destWidth = destWidth,
					.destHeight = destHeight,
					.sourceMaxX = sourceWidth - 1,
					.sourceMaxY = sourceHeight - 1
				};

				commandBuffer.bind(mip == 0 ? *m_pyramidBaseSets[baseSlot] : *m_pyramidSets[mip - 1], *m_pyramidPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
				vkCmdPushConstants(commandBuffer.handle(), m_pyramidPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PyramidPushConstants), &pc);
				commandBuffer.dispatch((destWidth + 7) / 8, (destHeight + 7) / 8, 1);
			}

			/* Pyramid complete: GENERAL -> SHADER_READ_ONLY for the composite fragment shader. */
			{
				const Sync::ImageMemoryBarrier barrier{
					*m_pyramidImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}
	}

	IndirectPostProcessEffect::DenoiseContribution
	RTR::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "rtr";
		/* SVGF order: the spatial bilateral runs on the TEMPORALLY integrated trace (the raw trace
		 * when the chain is off). */
		contribution.source = m_temporalOutput != nullptr ? m_temporalOutput : static_cast< const TextureInterface * >(&m_traceTarget);
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);
		contribution.needsDepth = true;
		contribution.needsNormals = true;
		contribution.dynamics = Base::Math::Vector< 4, float >{m_parameters.depthSigma, m_parameters.normalSigma, static_cast< float >(m_parameters.blurRadius), 0.0F};

		/* Same depth/normal-aware bilateral kernel as the retired RTR_Blur_FS pass,
		 * including the roughness²-driven cone-scaled radius (GGX lobe footprint:
		 * polished surfaces keep mirror-sharp reflections, brushed metal gets a real
		 * satin spread; blurRadius is the MAXIMUM, reached near roughness 0.7).
		 * Dynamics0: x = depthSigma, y = normalSigma, z = blurRadius (maximum). */
		contribution.code =
			"\tvec2 rtrDnTexel = 1.0 / vec2(textureSize(rtrSrc, 0));\n"
			"\tvec4 rtrDnCenter = texture(rtrSrc, vUV);\n"
			"\tfloat rtrDnCenterDepth = texture(emDepth, vUV).r;\n"
			"\tvec3 rtrDnCenterNormal = texture(emNormals, vUV).rgb;\n"
			"\tvec4 rtrResult = rtrDnCenter;\n"
			"\t/* Skip far-plane fragments. */\n"
			"\tif (rtrDnCenterDepth < 1.0)\n"
			"\t{\n"
			"\t\tfloat rtrDnPackedRM = texture(emNormals, vUV).a;\n"
			"\t\tfloat rtrDnRoughness = rtrDnPackedRM >= 2.0 ? rtrDnPackedRM - 2.0 : rtrDnPackedRM;\n"
			"\t\tfloat rtrDnConeScale = clamp((rtrDnRoughness * rtrDnRoughness) / 0.49, 0.0, 1.0);\n"
			"\t\tint rtrDnRadius = max(1, int(emDyn.rtrDynamics0.z * rtrDnConeScale));\n"
			"\t\tfloat rtrDnSpatialSigma = float(rtrDnRadius) * 0.5;\n"
			"\t\tfloat rtrDnInvSpatialSigma2 = 1.0 / (2.0 * rtrDnSpatialSigma * rtrDnSpatialSigma);\n"
			"\t\tfloat rtrDnInvDepthSigma2 = 1.0 / (2.0 * emDyn.rtrDynamics0.x * emDyn.rtrDynamics0.x);\n"
			"\t\tvec4 rtrDnSum = vec4(0.0);\n"
			"\t\tfloat rtrDnTotalWeight = 0.0;\n"
			"\t\tfor (int rtrDnI = -rtrDnRadius; rtrDnI <= rtrDnRadius; rtrDnI++)\n"
			"\t\t{\n"
			"\t\t\tvec2 rtrDnUV = vUV + emDenoiseDir * rtrDnTexel * float(rtrDnI);\n"
			"\t\t\tvec4 rtrDnSample = texture(rtrSrc, rtrDnUV);\n"
			"\t\t\tfloat rtrDnDepth = texture(emDepth, rtrDnUV).r;\n"
			"\t\t\tvec3 rtrDnNormal = texture(emNormals, rtrDnUV).rgb;\n"
			"\t\t\tfloat rtrDnSpatialW = exp(-float(rtrDnI * rtrDnI) * rtrDnInvSpatialSigma2);\n"
			"\t\t\tfloat rtrDnDepthDiff = abs(rtrDnCenterDepth - rtrDnDepth);\n"
			"\t\t\tfloat rtrDnDepthW = exp(-rtrDnDepthDiff * rtrDnDepthDiff * rtrDnInvDepthSigma2);\n"
			"\t\t\tfloat rtrDnNormalDot = max(dot(rtrDnCenterNormal, rtrDnNormal), 0.0);\n"
			"\t\t\tfloat rtrDnNormalW = pow(rtrDnNormalDot, 1.0 / max(emDyn.rtrDynamics0.y, 0.001));\n"
			"\t\t\tfloat rtrDnW = rtrDnSpatialW * rtrDnDepthW * rtrDnNormalW;\n"
			"\t\t\trtrDnSum += rtrDnSample * rtrDnW;\n"
			"\t\t\trtrDnTotalWeight += rtrDnW;\n"
			"\t\t}\n"
			"\t\tif (rtrDnTotalWeight > 0.0)\n"
			"\t\t{\n"
			"\t\t\trtrResult = rtrDnSum / rtrDnTotalWeight;\n"
			"\t\t}\n"
			"\t}\n";

		return contribution;
	}

	IndirectPostProcessEffect::CombineContribution
	RTR::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		/* The cone width comes per pixel from the trace (cone width map, TRACE texels — hit
		 * distance, roughness and camera distance folded in, see the trace shader). The scale is
		 * 1, or 0 to disable the pyramid lookup altogether (raw traced reflection, the sharpness
		 * reference of the bench). */
		const float coneWidthScale = m_coneEnabled ? 1.0F : 0.0F;
		const float pyramidLodOffset = -std::log2(static_cast< float >(m_traceTarget.width()) / static_cast< float >(std::max(1U, m_pyramidImage != nullptr ? m_pyramidImage->createInfo().extent.width : m_traceTarget.width())));
		const float pyramidMaxLod = std::min(m_coneMaxLod, static_cast< float >(m_pyramidMipCount > 0U ? m_pyramidMipCount - 1U : 0U));

		CombineContribution contribution;
		contribution.prefix = "rtr";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_blurVTarget});
		contribution.samplers.emplace_back(CombineSamplerInput{"Pyramid", &m_pyramidTexture});
		contribution.samplers.emplace_back(CombineSamplerInput{"Cone", &m_coneTexture});
		contribution.needsDepth = true;
		contribution.needsNormals = true;
		contribution.needsMaterialProperties = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, coneWidthScale, pyramidLodOffset, pyramidMaxLod});
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_coneBlendStart, m_coneBlendFull, 0.0F, 0.0F});

		/* Depth-aware upsample of the half-res reflection (4 taps, depth-similarity weights —
		 * plain bilinear bleeds across discontinuities), then the GLOSSY CONE: a DISK GATHER over
		 * the pre-convolved pyramid whose radius is the per-pixel cone width (v3, Aug 2026) —
		 * 16 taps on a Vogel spiral rotated per pixel (interleaved gradient noise, Jimenez 2014),
		 * Gaussian weights with FWHM = cone width, the disk cut at 2.5 σ (a 2 σ cut lost 12 % of the
		 * second moment, measured), sampled on the mip ~2x FINER than the kernel (log2(radius) - 1:
		 * 4x finer left a visible grain, σ 4.4/255 on the bench; each tap must integrate its share).
		 * The premultiplied (color·confidence, confidence) sums keep the
		 * confidence renormalization below exact. The v2 single textureLod at log2(width) picked
		 * a mip whose texel WAS the kernel: a 64-128 px block whose width depended on where the
		 * reflected feature fell in the texel grid (±30 %, measured on the bench), and the pyramid's
		 * last mip capped the blur at σ ≈ 76 px where the roughness asked for 93-277. The cross-fade
		 * (pure trace under blendStart, pure gather from blendFull) is unchanged, then the
		 * reflectivity-modulated, confidence-renormalized application. Per-pixel rotation only —
		 * no frame term — so a static frame is deterministic (the bench relies on it). */
		contribution.code =
			"\tvec2 rtrHalfTexel = 1.0 / vec2(textureSize(rtrTex, 0));\n"
			"\tfloat rtrCenterDepth = texture(emDepth, vUV).r;\n"
			"\tconst vec2 rtrOffsets[4] = vec2[](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5));\n"
			"\tvec4 rtrData = vec4(0.0);\n"
			"\tfloat rtrTotalWeight = 0.0;\n"
			"\tfor (int rtrI = 0; rtrI < 4; rtrI++)\n"
			"\t{\n"
			"\t\tvec2 rtrUV = vUV + rtrOffsets[rtrI] * rtrHalfTexel;\n"
			"\t\tfloat rtrD = texture(emDepth, rtrUV).r;\n"
			"\t\tfloat rtrW = exp(-abs(rtrD - rtrCenterDepth) * 512.0) + 1e-4;\n"
			"\t\trtrData += texture(rtrTex, rtrUV) * rtrW;\n"
			"\t\trtrTotalWeight += rtrW;\n"
			"\t}\n"
			"\trtrData /= rtrTotalWeight;\n"
			"\t/* The pixel's OWN trace confidence drives the final blend: a gathered (mean) confidence\n"
			"\t * would dim every reflection whose kernel overlaps a non-reflective neighbour. */\n"
			"\tfloat rtrOwnConfidence = rtrData.a;\n"
			"\tfloat rtrConeWidthTexels = texture(rtrCone, vUV).r * emDyn.rtrDynamics0.y;\n"
			"\tif (rtrConeWidthTexels > emDyn.rtrDynamics1.x)\n"
			"\t{\n"
			"\t\t/* Disk gather: FWHM = cone width, on a mip ~2x finer than the kernel, disk cut at 2.5 sigma. */\n"
			"\t\tfloat rtrRadiusP = 0.5 * rtrConeWidthTexels * exp2(emDyn.rtrDynamics0.z);\n"
			"\t\tfloat rtrSigmaP = rtrRadiusP * 0.8493;\n"
			"\t\tfloat rtrConeLOD = clamp(log2(max(rtrRadiusP, 1.0)) - 1.0, 0.0, emDyn.rtrDynamics0.w);\n"
			"\t\tvec2 rtrPyramidTexel = 1.0 / vec2(textureSize(rtrPyramid, 0));\n"
			"\t\tfloat rtrAngle = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n"
			"\t\tfloat rtrDiskRadius = 2.5 * rtrSigmaP;\n"
			"\t\tfloat rtrInvTwoSigma2 = 0.5 / max(rtrSigmaP * rtrSigmaP, 1e-4);\n"
			"\t\tconst int rtrConeTaps = 24;\n"
			"\t\tvec4 rtrConeData = vec4(0.0);\n"
			"\t\tfloat rtrConeWeightSum = 0.0;\n"
			"\t\tfor (int rtrI = 0; rtrI < rtrConeTaps; rtrI++)\n"
			"\t\t{\n"
			"\t\t\tfloat rtrR = sqrt((float(rtrI) + 0.5) / float(rtrConeTaps)) * rtrDiskRadius;\n"
			"\t\t\tfloat rtrA = rtrAngle + float(rtrI) * 2.39996323;\n"
			"\t\t\tvec2 rtrOff = vec2(cos(rtrA), sin(rtrA)) * rtrR;\n"
			"\t\t\tfloat rtrW = exp(-dot(rtrOff, rtrOff) * rtrInvTwoSigma2);\n"
			"\t\t\trtrConeData += textureLod(rtrPyramid, vUV + rtrOff * rtrPyramidTexel, rtrConeLOD) * rtrW;\n"
			"\t\t\trtrConeWeightSum += rtrW;\n"
			"\t\t}\n"
			"\t\trtrConeData /= max(rtrConeWeightSum, 1e-4);\n"
			"\t\tfloat rtrConeBlend = clamp((rtrConeWidthTexels - emDyn.rtrDynamics1.x) / max(emDyn.rtrDynamics1.y - emDyn.rtrDynamics1.x, 1e-3), 0.0, 1.0);\n"
			"\t\trtrData = mix(rtrData, rtrConeData, rtrConeBlend);\n"
			"\t}\n"
			"\tfloat rtrReflectivity = float(uint(texture(emMaterialProps, vUV).r * 255.0) >> 4u) / 15.0;\n"
			"\tif (rtrOwnConfidence > 0.001 && rtrReflectivity > 0.0)\n"
			"\t{\n"
			"\t\t/* Colour renormalized by the (possibly gathered) confidence it was premultiplied with. */\n"
			"\t\tem_Color.rgb = mix(em_Color.rgb, rtrData.rgb / max(rtrData.a, 0.001), rtrOwnConfidence * emDyn.rtrDynamics0.x * rtrReflectivity);\n"
			"\t}\n";

		return contribution;
	}
}
