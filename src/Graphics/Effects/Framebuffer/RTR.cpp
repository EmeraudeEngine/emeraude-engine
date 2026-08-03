/*
 * src/Graphics/Effects/Framebuffer/RTR.cpp
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

/* STL inclusions. */
#include <bit>
#include <cmath>

/* Local inclusions. */
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
	 *
	 * Descriptor set 1 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: environment cubemap (miss fallback)
	 *
	 * Descriptor set 2 (bindless textures — from BindlessTextureManager):
	 *   binding 1: sampler2D[] (2D texture array)
	 */
	static constexpr auto RTRTraceFragmentShader = R"GLSL(
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

/* Input textures (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform samplerCube envCubemap;

/* Bindless textures (set 2). Binding 1 = 2D texture array, binding 3 = cube array
 * (reserved slots: 1 = scene irradiance E/pi, 2 = GGX-prefiltered environment). */
layout(set = 2, binding = 1) uniform sampler2D textures2D[];
layout(set = 2, binding = 3) uniform samplerCube texturesCube[];

layout(push_constant) uniform PushConstants
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

const float PI = 3.14159265359;
/* Prefiltered environment mip count - 1 (IBLTexture::PrefilteredMipLevels). */
const float PrefilteredMaxLod = 5.0;

/* ENGINE CUBEMAP CONVENTION: world direction D samples at (D.x, -D.y, D.z). */
vec3 cubeDir (vec3 d)
{
	return vec3(d.x, -d.y, d.z);
}

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
 *   [2] = materialIndices[4]   — one per sub-geometry in the BLAS, looked up via
 *								 rayQueryGetIntersectionGeometryIndexEXT */
MeshAccessor getMeshAccessor (uint instanceIndex, uint primitiveIndex)
{
	MeshAccessor m;

	uvec4 meta0 = meshSSBO.meshEntries[instanceIndex * 3u];
	uvec4 meta1 = meshSSBO.meshEntries[instanceIndex * 3u + 1u];

	m.vb = VertexBuffer(uvec2(meta0.x, meta0.y));
	m.ib = IndexBuffer(uvec2(meta0.z, meta0.w));
	m.strideFloats = meta1.x / 4u;
	m.uvOffsetFloats = meta1.y / 4u;
	m.normalOffsetFloats = meta1.z / 4u;

	m.idx0 = m.ib.i[primitiveIndex * 3u];
	m.idx1 = m.ib.i[primitiveIndex * 3u + 1u];
	m.idx2 = m.ib.i[primitiveIndex * 3u + 2u];

	return m;
}

/* Look up the materialIndex for a hit by combining the instance and sub-geometry.
 * For single-sub-geometry BLAS the geomIdx is always 0 (materialIndices[0]).
 * For multi-sub-geometry BLAS (e.g. palm trunk + leaves) each sub-geometry maps to
 * its own material — no more cross-sub-geo material aliasing.
 *
 * Some BLAS may have many more sub-geometries than the renderable has "logical"
 * material slots — typically animated sprites where the procedural quad builder
 * creates one group per animation frame slot, even when only a few frames are
 * actually used. In that case subGeometryCount (meta1.w) is 1 and we clamp the
 * BLAS-side geomIdx to 0 so we always read the renderable's only material. */
uint getHitMaterialIndex (uint instanceIndex, uint geomIdx)
{
	uint subGeoCount = meshSSBO.meshEntries[instanceIndex * 3u + 1u].w;
	uint effectiveIdx = (geomIdx < subGeoCount) ? geomIdx : 0u;
	return meshSSBO.meshEntries[instanceIndex * 3u + 2u][effectiveIdx];
}

/* Interpolate a vec3 vertex attribute at the hit point (barycentric). */
vec3 getHitAttributeVec3 (MeshAccessor m, vec2 bary, uint offsetFloats)
{
	vec3 a0 = readVertexVec3(m.vb, m.idx0, m.strideFloats, offsetFloats);
	vec3 a1 = readVertexVec3(m.vb, m.idx1, m.strideFloats, offsetFloats);
	vec3 a2 = readVertexVec3(m.vb, m.idx2, m.strideFloats, offsetFloats);

	return a0 * (1.0 - bary.x - bary.y) + a1 * bary.x + a2 * bary.y;
}

/* Interpolate geometric normal at hit point. */
vec3 getHitNormal (MeshAccessor m, vec2 bary)
{
	return normalize(getHitAttributeVec3(m, bary, m.normalOffsetFloats));
}

/* Interpolate UV at hit point. */
vec2 getHitUV (MeshAccessor m, vec2 bary)
{
	vec2 uv0 = readVertexVec2(m.vb, m.idx0, m.strideFloats, m.uvOffsetFloats);
	vec2 uv1 = readVertexVec2(m.vb, m.idx1, m.strideFloats, m.uvOffsetFloats);
	vec2 uv2 = readVertexVec2(m.vb, m.idx2, m.strideFloats, m.uvOffsetFloats);

	return uv0 * (1.0 - bary.x - bary.y) + uv1 * bary.x + uv2 * bary.y;
}

/* Compute direct lighting at hit point (simple Lambert diffuse). */
/* Shadow ray: returns 1.0 when the path from the surface toward the light is unoccluded,
 * 0.0 otherwise. TerminateOnFirstHit: any-hit is enough for a visibility test. */
float shadowRayVisibility (vec3 origin, vec3 direction, float maxT)
{
	rayQueryEXT shadowQuery;
	rayQueryInitializeEXT(
		shadowQuery, topLevelAS,
		gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF,
		origin, 0.0, direction, maxT
	);

	while (rayQueryProceedEXT(shadowQuery)) {}

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

			/* Distance attenuation with radius falloff. */
			float radius = posRadius.w;

			if (radius > 0.0)
			{
				attenuation = clamp(1.0 - (dist / radius), 0.0, 1.0);
				attenuation *= attenuation;
			}
			else
			{
				/* Inverse-square falloff. */
				attenuation = 1.0 / (1.0 + dist * dist);
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
		vec3 diffuse = albedo * (1.0 - metalnessHit) * (vec3(1.0) - F);

		totalLight += lightColor * (diffuse + specular) * NdotL * attenuation * visibility;
	}

	return totalLight;
}
)GLSL" R"GLSL(

void main()
{
	/* Use texelFetch (no bilinear filtering) to avoid interpolating
	 * depth/normals across geometric edges at half-resolution. */
	ivec2 fullResCoord = ivec2(vUV * vec2(textureSize(depthTex, 0)));
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

	/* Reconstruct world-space position from NDC + depth via inverse VP. */
	vec2 ndc = vUV * 2.0 - 1.0;
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

	/* Fresnel (Schlick): stronger reflections at grazing angles.
	 * F0 floor at 0.15 so dielectrics viewed head-on still show visible reflections.
	 * Computed before the trace: it applies to both hit and environment-miss paths. */
	float F0 = mix(0.15, 0.9, originMetalness);
	float NdotV = max(dot(worldNormal, -viewDir), 0.0);
	float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

	/* Offset ray origin along normal to prevent self-intersection. */
	vec3 rayOrigin = worldPos + worldNormal * 0.01;

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
		/* Only triangle candidates need our handling. */
		if (rayQueryGetIntersectionTypeEXT(rayQuery, false) != gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			continue;
		}

		uint candidateInstanceIndex  = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
		uint candidateGeomIdx		= rayQueryGetIntersectionGeometryIndexEXT(rayQuery, false);
		uint candidateMaterialIndex  = getHitMaterialIndex(candidateInstanceIndex, candidateGeomIdx);
		uint candidateMatBase		= candidateMaterialIndex * 7u;
		uint candidateFlags		  = floatBitsToUint(materialSSBO.materials[candidateMatBase + 4u].w);

		/* Non-alpha-test materials: confirm immediately (BLAS-default behaviour). */
		if ((candidateFlags & IsAlphaTest) == 0u)
		{
			rayQueryConfirmIntersectionEXT(rayQuery);
			continue;
		}

		/* Alpha-test path: sample the opacity (or albedo alpha) at the hit UV. */
		uint candidatePrimitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
		vec2 candidateBary		   = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
		MeshAccessor candidateMesh   = getMeshAccessor(candidateInstanceIndex, candidatePrimitiveIndex);
		vec2 candidateUV			 = getHitUV(candidateMesh, candidateBary);
		float candidateAlpha		 = 1.0;

		if ((candidateFlags & HasOpacityTexture) != 0u)
		{
			int opacityIdx = floatBitsToInt(materialSSBO.materials[candidateMatBase + 6u].y);
			if (opacityIdx >= 0)
			{
				candidateAlpha = texture(textures2D[nonuniformEXT(opacityIdx)], candidateUV).r;
			}
		}
		else if ((candidateFlags & HasAlbedoTexture) != 0u)
		{
			int albedoIdx = floatBitsToInt(materialSSBO.materials[candidateMatBase + 5u].x);
			if (albedoIdx >= 0)
			{
				candidateAlpha = texture(textures2D[nonuniformEXT(albedoIdx)], candidateUV).a;
			}
		}
		else
		{
			/* No texture to sample — fall back to scalar albedo alpha. */
			candidateAlpha = materialSSBO.materials[candidateMatBase].a;
		}

		float cutoff = materialSSBO.materials[candidateMatBase + 6u].z;

		if (candidateAlpha >= cutoff)
		{
			rayQueryConfirmIntersectionEXT(rayQuery);
		}
		/* else: do not confirm — ray continues past this triangle. */
	}

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
		uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
		uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
		vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

		/* Unpack mesh data. */
		MeshAccessor mesh = getMeshAccessor(instanceIndex, primitiveIndex);

		/* Look up material per sub-geometry — the BLAS may have several sub-geometries
		 * with distinct materials (e.g. palm trunk + alpha-test leaves) and we pick the
		 * one matching the actual hit triangle's sub-geometry. */
		uint geomIdx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
		uint materialIndex = getHitMaterialIndex(instanceIndex, geomIdx);
		uint matBase = materialIndex * 7u;

		vec3 albedo = materialSSBO.materials[matBase].rgb;
		uint flags = floatBitsToUint(materialSSBO.materials[matBase + 4u].w);

		/* Sample bindless albedo texture if available. */
		if ((flags & HasAlbedoTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].x);

			if (texIndex >= 0)
			{
				vec2 hitUV = getHitUV(mesh, barycentrics);
				albedo = texture(textures2D[nonuniformEXT(texIndex)], hitUV).rgb;
			}
		}

		/* Compute world-space hit position and geometric normal. */
		vec3 hitPos = rayOrigin + reflDir * hitT;

		/* Transform object-space normal to world space.
		 * VBO normals are already in engine convention (Y+ = down, "Vulkan world axis").
		 * Apply objectToWorld for rotated/scaled instances. */
		vec3 objectNormal = getHitNormal(mesh, barycentrics);
		mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
		vec3 hitNormal = normalize(mat3(objectToWorld) * objectNormal);

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

		/* ---- Enriched hit shading (uber-shader): the FULL material model, data-driven
		 * from the RT material SSBO — no program duplication, one parametric BRDF. ---- */
		vec2 hitUV = getHitUV(mesh, barycentrics);

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
				vec3 rawNormal = texture(textures2D[nonuniformEXT(texIndex)], hitUV).rgb * 2.0 - 1.0;
				float normalScale = materialSSBO.materials[matBase + 6u].w;
				vec3 tangentSpaceNormal = normalize(vec3(rawNormal.xy * normalScale, rawNormal.z));

				vec3 hitTangent = normalize(mat3(objectToWorld) * getHitAttributeVec3(mesh, barycentrics, 3u));
				vec3 hitBinormal = normalize(mat3(objectToWorld) * getHitAttributeVec3(mesh, barycentrics, 6u));

				hitNormal = normalize(hitTangent * tangentSpaceNormal.x + hitBinormal * tangentSpaceNormal.y + hitNormal * tangentSpaceNormal.z);
			}
		}

		/* Roughness / metalness: scalar, overridden by their textures when present. */
		float hitRoughness = materialSSBO.materials[matBase + 1u].x;
		float hitMetalness = materialSSBO.materials[matBase + 1u].y;

		if ((flags & HasRoughnessTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].z);

			if (texIndex >= 0)
			{
				hitRoughness = texture(textures2D[nonuniformEXT(texIndex)], hitUV).r;
			}
		}

		if ((flags & HasMetalnessTexture) != 0u)
		{
			int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].w);

			if (texIndex >= 0)
			{
				hitMetalness = texture(textures2D[nonuniformEXT(texIndex)], hitUV).r;
			}
		}

		vec3 hitF0 = mix(vec3(0.04), albedo, hitMetalness);
		vec3 hitV = -reflDir;
		float hitNdotV = max(dot(hitNormal, hitV), 0.0001);

		/* Direct lighting: Lambert diffuse + GGX specular per light, shadow-ray gated
		 * (same microfacet family as the raster pass — the reflection matches the image). */
		vec3 litColor = computeDirectLighting(hitPos, hitNormal, hitV, albedo, hitRoughness, hitMetalness, hitF0);

		/* Ambient at hit: the scene scalar ambient PLUS the sky IBL — diffuse irradiance
		 * (reserved cube slot 1, stores E/pi) and a prefiltered specular tap (slot 2,
		 * roughness-driven LOD), both scaled by the sky luminance (normalized sources). */
		{
			vec3 irradiance = texture(texturesCube[nonuniformEXT(1)], cubeDir(hitNormal)).rgb;
			vec3 iblDiffuse = albedo * (1.0 - hitMetalness) * irradiance;

			vec3 hitR = reflect(reflDir, hitNormal);
			vec3 prefiltered = textureLod(texturesCube[nonuniformEXT(2)], cubeDir(hitR), clamp(hitRoughness, 0.0, 1.0) * PrefilteredMaxLod).rgb;
			vec3 iblSpecular = prefiltered * fresnelSchlick(hitNdotV, hitF0);

			litColor += albedo * ambientLight.rgb;
			litColor += (iblDiffuse + iblSpecular) * ambientLight.w;
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
					emission *= texture(textures2D[nonuniformEXT(texIndex)], hitUV).rgb;
				}
			}

			litColor += emission;
		}

		/* Distance fade: reflection fades as hit gets further from the surface. */
		float distFade = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);

		float confidence = distFade * fresnel * roughnessFade;

		outReflection = vec4(litColor * confidence, confidence);
	}
	else
	{
		/* Ray escaped the scene: reflect the ACTIVE SCENE's prefiltered environment
		 * (bindless reserved cube slot 2, always current), roughness-driven LOD, scaled
		 * by the sky luminance — a normalized source becomes nits. The former dedicated
		 * envCubemap binding fell back to the renderer DEFAULT cubemap when the caller
		 * passed none (dark sky in every reflection, measured on the bench). */
		vec3 envColor = textureLod(texturesCube[nonuniformEXT(2)], vec3(reflDir.x, -reflDir.y, reflDir.z), clamp(roughness, 0.0, 1.0) * PrefilteredMaxLod).rgb * ambientLight.w;
		float confidence = fresnel * roughnessFade;

		outReflection = vec4(envColor * confidence, confidence);
	}
}
)GLSL";

	/* Blur shader — identical to SSR blur. */
	/* Bilateral blur shader — depth/normal-aware separable filter for reflections.
	 * Preserves sharp reflection edges at geometric boundaries. */
	static constexpr auto RTRBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outBlur;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float directionX;
	float directionY;
	float depthSigma;
	float normalSigma;
	int blurRadius;
	float padding;
};

void main()
{
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec2 dir = vec2(directionX, directionY);

	vec4 centerVal = texture(inputTex, vUV);
	float centerDepth = texture(depthTex, vUV).r;
	vec3 centerNormal = texture(normalTex, vUV).rgb;

	if (centerDepth >= 1.0)
	{
		outBlur = centerVal;
		return;
	}

	vec4 result = vec4(0.0);
	float totalWeight = 0.0;

	/* Cone-scaled radius: the GGX lobe tangent grows with alpha = roughness², so the
	 * blur footprint follows the SQUARE of the roughness — polished surfaces (water,
	 * onyx) keep mirror-sharp reflections, brushed metal gets a real satin spread.
	 * blurRadius is the MAXIMUM (reached near roughness 0.7); v1 approximation: the
	 * footprint ignores the per-pixel hit distance (uniform cone) — the stochastic
	 * + temporal successor will replace it (Frostbite SSSR). */
	float packedRM = texture(normalTex, vUV).a;
	float centerRoughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;
	float coneScale = clamp((centerRoughness * centerRoughness) / 0.49, 0.0, 1.0);
	int effectiveRadius = max(1, int(float(blurRadius) * coneScale));

	float spatialSigma = float(effectiveRadius) * 0.5;
	float invSpatialSigma2 = 1.0 / (2.0 * spatialSigma * spatialSigma);
	float invDepthSigma2 = 1.0 / (2.0 * depthSigma * depthSigma);

	for (int i = -effectiveRadius; i <= effectiveRadius; i++)
	{
		vec2 sampleUV = vUV + dir * texelSize * float(i);
		vec4 sampleVal = texture(inputTex, sampleUV);
		float sampleDepth = texture(depthTex, sampleUV).r;
		vec3 sampleNormal = texture(normalTex, sampleUV).rgb;

		float spatialW = exp(-float(i * i) * invSpatialSigma2);
		float depthDiff = abs(centerDepth - sampleDepth);
		float depthW = exp(-depthDiff * depthDiff * invDepthSigma2);
		float normalDot = max(dot(centerNormal, sampleNormal), 0.0);
		float normalW = pow(normalDot, 1.0 / max(normalSigma, 0.001));

		float w = spatialW * depthW * normalW;
		result += sampleVal * w;
		totalWeight += w;
	}

	outBlur = (totalWeight > 0.0) ? result / totalWeight : centerVal;
}
)GLSL";

	/* Reflection pyramid downsample: 4 bilinear taps at the corners of the destination
	 * texel's source footprint — a 4x4 tent, converging toward a gaussian across the
	 * chain (same pre-convolution as the SSR color pyramid). Operates on the
	 * PREMULTIPLIED trace output (color·confidence, confidence): the composite's
	 * division by the filtered confidence renormalizes edge bleed. */
	static constexpr auto RTRPyramidDownsampleComputeShader = R"GLSL(
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

	/* Push constants of the pyramid build dispatches. */
	struct PyramidPushConstants
	{
		int32_t destWidth;
		int32_t destHeight;
		int32_t sourceMaxX;
		int32_t sourceMaxY;
	};

	/* Composite shader — blends ray-traced reflections with the scene,
	 * modulated by the per-pixel reflectivity from the material properties G-buffer. */
	static constexpr auto RTRCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D rtrTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;
layout(set = 0, binding = 3) uniform sampler2D depthTex;
layout(set = 0, binding = 4) uniform sampler2D normalTex;
/* Pre-convolved reflection pyramid (premultiplied color + confidence). */
layout(set = 0, binding = 5) uniform sampler2D pyramidTex;

layout(push_constant) uniform PushConstants
{
	float intensity;
	float coneWidthScale;
	float pyramidLodOffset;
	float pyramidMaxLod;
};

void main()
{
	vec4 color = texture(colorTex, vUV);

	/* Depth-aware upsample of the half-res reflection buffer: 4 taps around the
	 * pixel, each weighted by depth similarity. Plain bilinear filtering bleeds
	 * reflections across depth discontinuities (halos around thin geometry). */
	vec2 halfTexel = 1.0 / vec2(textureSize(rtrTex, 0));
	float centerDepth = texture(depthTex, vUV).r;

	const vec2 offsets[4] = vec2[](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5));

	vec4 rtrData = vec4(0.0);
	float totalWeight = 0.0;

	for (int i = 0; i < 4; i++)
	{
		vec2 uv = vUV + offsets[i] * halfTexel;
		float d = texture(depthTex, uv).r;
		float w = exp(-abs(d - centerDepth) * 512.0) + 1e-4;

		rtrData += texture(rtrTex, uv) * w;
		totalWeight += w;
	}

	rtrData /= totalWeight;

	/* Glossy cone approximation: the GGX lobe of a rough surface spreads the traced
	 * reflection over a cone whose footprint grows with alpha = roughness². The
	 * separable bilateral tops out at a few texels; beyond that the pre-convolved
	 * pyramid delivers an O(1) blur of ANY width. v1: the cone assumes a representative
	 * hit distance (coneWidthScale) — the per-pixel hit distance lives in the future
	 * stochastic + temporal successor (Frostbite SSSR). */
	float compositePackedRM = texture(normalTex, vUV).a;
	float compositeRoughness = compositePackedRM >= 2.0 ? compositePackedRM - 2.0 : compositePackedRM;
	float coneWidthTexels = coneWidthScale * compositeRoughness * compositeRoughness;


	if (coneWidthTexels > 1.0)
	{
		float coneLOD = clamp(log2(coneWidthTexels) + pyramidLodOffset, 0.0, pyramidMaxLod);
		vec4 coneData = textureLod(pyramidTex, vUV, coneLOD);

		rtrData = mix(rtrData, coneData, clamp(coneWidthTexels - 1.0, 0.0, 1.0));
	}

	/* Decode reflectivity from the material properties G-buffer (R channel, high nibble). */
	vec4 mp = texture(materialPropsTex, vUV);
	uint rPacked = uint(mp.r * 255.0);
	float reflectivity = float(rPacked >> 4u) / 15.0;

	/* rtrData.rgb = blurred reflected color, rtrData.a = blurred confidence. */
	float confidence = rtrData.a;

	if (confidence > 0.001 && reflectivity > 0.0)
	{
		color.rgb = mix(color.rgb, rtrData.rgb / max(confidence, 0.001), confidence * intensity * reflectivity);
	}

	outColor = color;
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RTR::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Pixel doubling: half-res for performance (default), full-res for quality. */
		const auto pixelDoubling = renderer.primaryServices().settings().getOrSetDefault< bool >(GraphicsRayTracingReflectionPixelDoublingKey, DefaultGraphicsRayTracingReflectionPixelDoubling);
		const auto halfW = pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? ((height > 1) ? height / 2 : 1U) : height;

		/* Trace target (half-res by default, RGBA16F: reflected color RGB + confidence A). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTR_Trace") )
		{
			TraceError{ClassId} << "Failed to create RTR trace target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
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

		/* Composite target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "RTR_Output") )
		{
			TraceError{ClassId} << "Failed to create RTR output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals + environment cubemap — 3 combined image samplers. */
		auto traceInputLayout = this->getInputLayout(3);

		/* Single input (blur): 1 combined image sampler. */
		auto blurInputLayout = this->getInputLayout(3);

		/* Composite input (color + blurred RTR + material properties + depth): 4 combined image samplers. */
		auto compositeLayout = this->getInputLayout(6);

		if ( traceInputLayout == nullptr || blurInputLayout == nullptr || compositeLayout == nullptr )
		{
			return false;
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

		/* ---- Pipeline layouts ---- */
		{
			/* Trace: set 0 = RT data, set 1 = depth + normals, set 2 = bindless textures. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(blurInputLayout);

			m_blurLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(compositeLayout);

			m_compositeLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			});
		}

		if ( m_traceLayout == nullptr || m_blurLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTR_Trace_FS", ShaderType::FragmentShader, RTRTraceFragmentShader);
		const auto blurFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTR_Blur_FS", ShaderType::FragmentShader, RTRBlurFragmentShader);
		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTR_Composite_FS", ShaderType::FragmentShader, RTRCompositeFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr || blurFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTR shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "RTR_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_blurPipeline = this->createFullscreenPipeline(ClassId, "RTR_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "RTR_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_blurPipeline == nullptr || m_compositePipeline == nullptr )
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

		/* Blur H: reads trace result + depth + normals (per-frame). */
		m_blurHPerFrame = this->createPerFrameDescriptorSets(blurInputLayout, ClassId, "BlurH_DescSet");

		if ( m_blurHPerFrame.empty() )
		{
			return false;
		}

		for ( auto & ds : m_blurHPerFrame )
		{
			if ( !ds->writeCombinedImageSampler(0, m_traceTarget) )
			{
				return false;
			}
		}

		/* Blur V: reads blur H result + depth + normals (per-frame). */
		m_blurVPerFrame = this->createPerFrameDescriptorSets(blurInputLayout, ClassId, "BlurV_DescSet");

		if ( m_blurVPerFrame.empty() )
		{
			return false;
		}

		for ( auto & ds : m_blurVPerFrame )
		{
			if ( !ds->writeCombinedImageSampler(0, m_blurHTarget) )
			{
				return false;
			}
		}

		/* Composite: reads color (per-frame) + blurred RTR (fixed). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(compositeLayout, ClassId, "Composite_DescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		/* ---- Pre-convolved reflection pyramid (glossy cone approximation) ---- */
		{
			const auto device = renderer.device();

			const uint32_t pyramidBaseW = std::max(1U, static_cast< uint32_t >(m_traceTarget.width()) / 2U);
			const uint32_t pyramidBaseH = std::max(1U, static_cast< uint32_t >(m_traceTarget.height()) / 2U);
			m_pyramidMipCount = std::clamp(static_cast< uint32_t >(std::bit_width(std::min(pyramidBaseW, pyramidBaseH))) - 3U, 1U, 8U);

			m_pyramidImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				VK_FORMAT_R16G16B16A16_SFLOAT,
				VkExtent3D{pyramidBaseW, pyramidBaseH, 1U},
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

			m_pyramidMipViews.reserve(m_pyramidMipCount);

			for ( uint32_t mip = 0; mip < m_pyramidMipCount; mip++ )
			{
				auto view = std::make_shared< ImageView >(
					m_pyramidImage,
					VK_IMAGE_VIEW_TYPE_2D,
					VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, mip, 1U, 0U, 1U}
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
				VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0U, m_pyramidMipCount, 0U, 1U}
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
			m_pyramidDSLayout = std::make_shared< DescriptorSetLayout >(device, "RTRPyramidDSLayout");

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
				device, "RTRPyramidPipelineLayout",
				StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_pyramidDSLayout},
				StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
			);

			if ( !m_pyramidPipelineLayout->createOnHardware() )
			{
				return false;
			}

			const auto downsampleModule = renderer.shaderManager().getShaderModuleFromSourceCode(device, "RTR_PyramidDownsample_CS", ShaderType::ComputeShader, RTRPyramidDownsampleComputeShader);

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

			const std::vector< VkDescriptorPoolSize > poolSizes{
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_pyramidMipCount},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_pyramidMipCount}
			};

			m_pyramidDescriptorPool = std::make_shared< DescriptorPool >(device, poolSizes, m_pyramidMipCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

			if ( !m_pyramidDescriptorPool->createOnHardware() )
			{
				return false;
			}

			const auto writeComputeSet = [&device, this] (const DescriptorSet & descriptorSet, VkImageView sourceView, VkImageLayout sourceLayout, VkSampler sourceSampler, const ImageView & destView) {
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

				vkUpdateDescriptorSets(device->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);
			};

			m_pyramidSets.reserve(m_pyramidMipCount);

			for ( uint32_t mip = 0; mip < m_pyramidMipCount; mip++ )
			{
				auto descriptorSet = std::make_unique< DescriptorSet >(m_pyramidDescriptorPool, m_pyramidDSLayout);
				descriptorSet->setIdentifier(ClassId, "Pyramid_DescSet" + std::to_string(mip), "DescriptorSet");

				if ( !descriptorSet->create() )
				{
					return false;
				}

				if ( mip == 0 )
				{
					/* Trace target (fixed IRT, SHADER_READ_ONLY after its render pass) -> mip 0. */
					writeComputeSet(*descriptorSet, m_traceTarget.imageView()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_pyramidSampler->handle(), *m_pyramidMipViews[0]);
				}
				else
				{
					/* Previous mip (sampled in GENERAL during the chain) -> this mip. */
					writeComputeSet(*descriptorSet, m_pyramidMipViews[mip - 1]->handle(), VK_IMAGE_LAYOUT_GENERAL, m_pyramidSampler->handle(), *m_pyramidMipViews[mip]);
				}

				m_pyramidSets.emplace_back(std::move(descriptorSet));
			}

			/* The composite reads the full chain (binding 5), same view every frame. */
			for ( const auto & descriptorSet : m_compositePerFrame )
			{
				VkDescriptorImageInfo pyramidInfo{};
				pyramidInfo.sampler = m_pyramidSampler->handle();
				pyramidInfo.imageView = m_pyramidFullView->handle();
				pyramidInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet write{};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.dstSet = descriptorSet->handle();
				write.dstBinding = 5;
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				write.descriptorCount = 1;
				write.pImageInfo = &pyramidInfo;

				vkUpdateDescriptorSets(device->handle(), 1, &write, 0, nullptr);
			}
		}

		return true;
	}

	void
	RTR::destroy () noexcept
	{
		m_pyramidSets.clear();
		m_pyramidDescriptorPool.reset();
		m_pyramidDownsamplePipeline.reset();
		m_pyramidPipelineLayout.reset();
		m_pyramidDSLayout.reset();
		m_pyramidSampler.reset();
		m_pyramidFullView.reset();
		m_pyramidMipViews.clear();
		m_pyramidImage.reset();

		m_compositePerFrame.clear();
		m_tracePerFrame.clear();
		m_blurVPerFrame.clear();
		m_blurHPerFrame.clear();

		m_compositePipeline.reset();
		m_blurPipeline.reset();
		m_tracePipeline.reset();
		m_compositeLayout.reset();
		m_blurLayout.reset();
		m_traceLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();
	}

	const TextureInterface &
	RTR::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto * inputMaterialProperties = context.materialProperties;
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

		/* Upgrade from the default cubemap once the scene environment finishes
		 * its asynchronous load (only the current frame's set is written — the
		 * other frames' sets may still be in flight). */
		if ( m_environmentCubemap != nullptr && m_environmentCubemap->isCreated() )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, *m_environmentCubemap));
		}

		/* Update color descriptor for composite pass. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for composite pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* Update depth descriptor for the composite depth-aware upsample. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(3, *inputDepth));
		}

		/* Roughness (normals alpha) drives the composite's glossy cone LOD. */
		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(4, *inputNormals));
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
			 * Falls back to the previous neutral 0.15 grey when no light set is available. */
			const auto ambientColor = lightSet != nullptr ? lightSet->ambientLightColor() : Base::PixelFactory::Color< float >{0.15F, 0.15F, 0.15F, 1.0F};
			const auto ambientIntensity = lightSet != nullptr ? lightSet->ambientLightIntensity() : 1.0F;

			const TracePushConstants pc{
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
				.skyLuminance = context.skyLuminance
			};

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
				.offset = {0, 0},
				.extent = {m_traceTarget.width(), m_traceTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			vkCmdPushConstants(
				commandBuffer.handle(),
				m_traceLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TracePushConstants),
				&pc
			);

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

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 1b: pre-convolved reflection pyramid build (glossy cone source) ---- */
		if ( m_pyramidImage != nullptr )
		{
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
				Sync::ImageMemoryBarrier barrier{
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
					Sync::ImageMemoryBarrier barrier{
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

				commandBuffer.bind(*m_pyramidSets[mip], *m_pyramidPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
				vkCmdPushConstants(commandBuffer.handle(), m_pyramidPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PyramidPushConstants), &pc);
				commandBuffer.dispatch((destWidth + 7) / 8, (destHeight + 7) / 8, 1);
			}

			/* Pyramid complete: GENERAL -> SHADER_READ_ONLY for the composite fragment shader. */
			{
				Sync::ImageMemoryBarrier barrier{
					*m_pyramidImage,
					VK_ACCESS_SHADER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}

		/* Update depth + normals descriptors for blur passes (per-frame). */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_blurHPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
			static_cast< void >(m_blurVPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_blurHPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputNormals));
			static_cast< void >(m_blurVPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputNormals));
		}

		/* ---- Pass 2: Bilateral Blur Horizontal ---- */
		{
			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F,
				.depthSigma = m_parameters.depthSigma,
				.normalSigma = m_parameters.normalSigma,
				.blurRadius = static_cast< int32_t >(m_parameters.blurRadius),
				.padding = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurHTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurHPerFrame[frameIndex],
				&blurH,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 3: Bilateral Blur Vertical ---- */
		{
			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F,
				.depthSigma = m_parameters.depthSigma,
				.normalSigma = m_parameters.normalSigma,
				.blurRadius = static_cast< int32_t >(m_parameters.blurRadius),
				.padding = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurVTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurVPerFrame[frameIndex],
				&blurV,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 4: Composite ---- */
		{
			const CompositePushConstants comp{
				.intensity = m_parameters.intensity,
				/* Cone width per unit of GGX alpha: 2 x assumed hit fraction (0.15 of the
				 * trace height). v1 — no per-pixel hit distance available. */
				.coneWidthScale = 0.3F * static_cast< float >(m_traceTarget.height()),
				.pyramidLodOffset = -std::log2(static_cast< float >(m_traceTarget.width()) / static_cast< float >(std::max(1U, m_pyramidImage != nullptr ? m_pyramidImage->createInfo().extent.width : static_cast< uint32_t >(m_traceTarget.width())))),
				.pyramidMaxLod = static_cast< float >(m_pyramidMipCount > 0U ? m_pyramidMipCount - 1U : 0U)
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer, m_outputTarget, *m_compositePipeline, *m_compositeLayout,
				*m_compositePerFrame[frameIndex], &comp, sizeof(CompositePushConstants)
			);
		}

		return m_outputTarget;
	}
}
