/*
 * src/Graphics/Effects/Framebuffer/RTGI.cpp
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

#include "RTGI.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	using namespace EmEn;

	/* RTGI trace pass: one traced diffuse bounce per frame, plus the multi-bounce
	 * temporal feedback. For each pixel, casts hemisphere rays against the TLAS. On hit,
	 * samples the surface albedo (bindless texture or scalar), computes direct lighting
	 * at the hit point, and re-injects the hit surface's accumulated indirect irradiance
	 * from the previous frame's history (geometric series → multi-bounce at steady state).
	 * The output is DEMODULATED: no receiver albedo — the whole denoise/temporal chain
	 * carries irradiance and the combine pass re-applies the albedo at full resolution.
	 *
	 * Descriptor set 0 (RT data — bound from Renderer::rtDescriptorSet()):
	 *   binding 0: accelerationStructureEXT (TLAS)
	 *   binding 1: RTMeshMetaData SSBO
	 *   binding 2: RTMaterialData SSBO
	 *   binding 3: RTLightData SSBO
	 *
	 * Descriptor set 1 (input textures + frame UBO — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: GI history texture (previous resolved frame)
	 *   binding 3: frame UBO (matrices + parameters — exceeds the 128-byte push constant minimum)
	 *
	 * Descriptor set 2 (bindless textures — from BindlessTextureManager):
	 *   binding 1: sampler2D[] (2D texture array)
	 */
	constexpr auto RTGITraceFragmentShader = R"GLSL(
#version 460
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outIndirect;

/* Buffer reference types for vertex/index data access via device addresses. */
layout(buffer_reference, scalar) readonly buffer VertexBuffer { float v[]; };
layout(buffer_reference, scalar) readonly buffer IndexBuffer { uint i[]; };

/* RT data (set 0). */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 1) readonly buffer MeshMetaData
{
	uvec4 meshEntries[];
} meshSSBO;

layout(set = 0, binding = 2) readonly buffer MaterialData
{
	vec4 materials[];
} materialSSBO;

layout(set = 0, binding = 3) readonly buffer LightData
{
	vec4 lights[];
} lightSSBO;

/* Input textures + frame UBO (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;
layout(set = 1, binding = 2) uniform sampler2D historyTex;

layout(set = 1, binding = 3, std140) uniform FrameData
{
	mat4 invViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = unused. */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags. */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z/w = unused. */
	vec4 skyParams;		/* x = sky luminance in nits (0 = no sky), y = sky ray distance, z/w = unused. */
};

/* Bindless textures (set 2). Binding 1 = 2D texture array, binding 3 = cubemap array whose
 * reserved slot 0 always holds the ACTIVE SCENE's environment cubemap (the
 * BindlessTextureManager keeps it in sync and parks the engine default when a scene has none
 * — hence the luminance guard below rather than a null test). */
layout(set = 2, binding = 1) uniform sampler2D textures2D[];
layout(set = 2, binding = 3) uniform samplerCube texturesCube[];

/* Reserved bindless slot of the scene environment cubemap (BindlessTextureManager). */
const uint EnvironmentCubemapSlot = 0u;

/* Radiance of the sky in a given direction, in nits.
 * The cubemap is LDR (display-referred colour), the physical scale is the background's declared
 * luminance — the same value the skybox renders with, so the lighting and the visible sky cannot
 * disagree. THE SKY IS A LIGHT SOURCE: without this, a ray that escapes contributes nothing and
 * every shadow is lit by bounces alone (Sponza on the Moon).
 * @note No sun-disc exclusion: an LDR cubemap clamps a painted sun to the sky's own luminance,
 * and a ~0.5-degree disc is ~2e-5 of a cosine-weighted hemisphere — 0.002% of the sky's
 * irradiance, far below the sampling noise. An HDR sky would need one. */
vec3 skyRadiance (vec3 direction)
{
	if (skyParams.x <= 0.0)
	{
		return vec3(0.0);
	}

	/* ENGINE CUBEMAP CONVENTION: a world direction D samples the cubemap at
	 * vec3(D.x, -D.y, D.z) — the engine world is Y-down (UP = -Y) while the cubemap is
	 * stored Y-up. Same negation as the skybox (Material/Helpers.cpp) and the material
	 * reflections (PBR/Standard); sampling the raw direction reads the sky upside-down. */
	return texture(texturesCube[nonuniformEXT(EnvironmentCubemapSlot)], vec3(direction.x, -direction.y, direction.z)).rgb * skyParams.x;
}

/* NOTE: GLSL has no built-in PI constant. */
const float PI = 3.14159265;

/* Material flag bits (must match GPURTMaterialData). */
const uint HasAlbedoTexture = 1u;

/* Light type constants. */
const float LIGHT_DIRECTIONAL = 0.0;
const float LIGHT_POINT = 1.0;
const float LIGHT_SPOT = 2.0;

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

/* Shared mesh data unpacking. */
struct MeshAccessor
{
	VertexBuffer vb;
	IndexBuffer ib;
	uint strideFloats;
	uint normalOffsetFloats;
	uint uvOffsetFloats;
	uint idx0, idx1, idx2;
};

/* GPUMeshMetaData layout: 3 uvec4 per instance (see RTR.cpp for details). */
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

/* Interpolate geometric normal at hit point. */
vec3 getHitNormal (MeshAccessor m, vec2 bary)
{
	vec3 n0 = readVertexVec3(m.vb, m.idx0, m.strideFloats, m.normalOffsetFloats);
	vec3 n1 = readVertexVec3(m.vb, m.idx1, m.strideFloats, m.normalOffsetFloats);
	vec3 n2 = readVertexVec3(m.vb, m.idx2, m.strideFloats, m.normalOffsetFloats);

	return normalize(n0 * (1.0 - bary.x - bary.y) + n1 * bary.x + n2 * bary.y);
}

/* Interpolate UV at hit point. */
vec2 getHitUV (MeshAccessor m, vec2 bary)
{
	vec2 uv0 = readVertexVec2(m.vb, m.idx0, m.strideFloats, m.uvOffsetFloats);
	vec2 uv1 = readVertexVec2(m.vb, m.idx1, m.strideFloats, m.uvOffsetFloats);
	vec2 uv2 = readVertexVec2(m.vb, m.idx2, m.strideFloats, m.uvOffsetFloats);

	return uv0 * (1.0 - bary.x - bary.y) + uv1 * bary.x + uv2 * bary.y;
}

/* PCG integer hash → decorrelated white noise from integer pixel coordinates.
 * The previous fract(sin(dot(...))) hash is a deterministic function of screen position with
 * float-precision beating: it produced a fixed, scene-independent grid/banding pattern, identical
 * on every GPU, that the spatial denoiser cannot remove. PCG gives proper per-pixel white noise. */
uint pcgHash (uint v)
{
	v = v * 747796405u + 2891336453u;
	uint s = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
	return (s >> 22u) ^ s;
}

vec2 hash2 (uvec2 p)
{
	uint h = pcgHash(p.x + pcgHash(p.y));
	return vec2(float(h & 0xffffu), float((h >> 16u) & 0xffffu)) * (1.0 / 65535.0);
}

/* Generate a cosine-weighted hemisphere sample direction. */
vec3 hemispherePoint (uint i, uint sampleCount, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

/* Multi-bounce feedback: fetch the accumulated indirect irradiance the hit surface had in
 * the previous resolved frame. The history stores DEMODULATED irradiance (E / PI — the
 * receiver albedo is applied at full resolution in the combine pass), so the CALLER must
 * multiply this value by the HIT surface's albedo to turn it into outgoing radiance; the
 * geometric series is damped by that albedo product and converges as long as the albedo is
 * physical (< 1). Validated against the camera distance stored in the history alpha channel
 * (0 = invalid/sky), clamped against fireflies. */
vec3 historyFeedback (vec3 hitPos)
{
	if (bounceParams.x <= 0.0)
	{
		return vec3(0.0);
	}

	vec4 prevClip = prevViewProj * vec4(hitPos, 1.0);

	if (prevClip.w <= 0.0)
	{
		return vec3(0.0);
	}

	vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;

	if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
	{
		return vec3(0.0);
	}

	vec4 history = texture(historyTex, prevUV);

	/* Disocclusion test: the camera distance is rotation-invariant, so a simple relative
	 * comparison rejects histories belonging to another surface. */
	float expectedDistance = length(hitPos - prevCamPos.xyz);

	if (history.a <= 0.0 || abs(history.a - expectedDistance) > temporalParams.y * expectedDistance)
	{
		return vec3(0.0);
	}

	return min(history.rgb, vec3(bounceParams.y)) * bounceParams.x;
}

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

/* Compute direct lighting at hit point (Lambert diffuse over all scene lights).
 * lightCount is derived from push constants but stored in the SSBO header.
 * We pass it via the last push constant field (sampleCount shares the uint slot).
 * Each contribution is gated by a shadow ray: without the occlusion test, every hit
 * point receives the light straight through walls and the indirect pass floods
 * shadowed areas (the raster direct pass is shadow-mapped, the GI must match). */
vec3 computeDirectLighting (vec3 hitPos, vec3 hitNormal, uint lightCount)
{
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
			/* Directional light. */
			L = normalize(-dirType.xyz);
		}
		else
		{
			/* Point or spot light. */
			vec3 toLight = posRadius.xyz - hitPos;
			float dist = length(toLight);
			L = toLight / max(dist, 0.0001);
			shadowDistance = dist;

			float radius = posRadius.w;

			if (radius > 0.0)
			{
				attenuation = clamp(1.0 - (dist / radius), 0.0, 1.0);
				attenuation *= attenuation;
			}
			else
			{
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
		 * on screen, and the indirect bounce must match the rendered scene. */
		if (lightSSBO.lights[base + 3u].z > 0.5)
		{
			/* traceParams.y = bias (frame UBO). */
			vec3 shadowOrigin = hitPos + hitNormal * max(traceParams.y, 0.001);
			visibility = shadowRayVisibility(shadowOrigin, L, shadowDistance);
		}

		totalLight += lightColor * NdotL * attenuation * visibility;
	}

	return totalLight;
}

)GLSL" R"GLSL(
void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (depth >= 1.0)
	{
		outIndirect = vec4(0.0);
		return;
	}

	/* Read view-space normal from MRT. */
	vec4 normalData = texture(normalTex, vUV);
	vec3 rawN = normalData.rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outIndirect = vec4(0.0);
		return;
	}

	/* Reconstruct world-space position from NDC + depth via inverse VP. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 wp = invViewProj * clipPos;
	vec3 worldPos = wp.xyz / wp.w;

	/* Unpack the frame UBO scalars. */
	float maxDistance = traceParams.x;
	float bias = traceParams.y;
	uint sampleCount = uint(traceParams.z);
	/* How far a ray must travel before "nothing hit" may be called sky (see the gather). */
	float skyDistance = max(skyParams.y, maxDistance);
	vec3 viewPos = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);

	/* Transform view-space normal to world space. */
	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRot * normalize(rawN));

	/* Build a tangent frame (TBN) around the world normal for hemisphere sampling. */
	vec3 up = abs(worldNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, worldNormal));
	vec3 bitangent = cross(worldNormal, tangent);
	mat3 TBN = mat3(tangent, bitangent, worldNormal);

	/* Per-pixel random rotation to break banding. */
	vec2 noiseVec = hash2(uvec2(gl_FragCoord.xy));

	/* Adaptive bias: scale with camera distance AND grazing angle.
	 * Distance: pixel footprint grows → needs larger offset.
	 * NdotV: at grazing angles, rays easily clip the surface → needs extra offset. */
	vec3 viewDir = normalize(worldPos - viewPos);
	float cameraDist = length(worldPos - viewPos);
	float NdotV = max(abs(dot(worldNormal, -viewDir)), 0.001);
	float grazingFactor = 1.0 / NdotV;
	float adaptiveBias = bias * max(1.0, cameraDist) * min(grazingFactor, 10.0);

	/* Offset ray origin along normal to prevent self-intersection. */
	vec3 rayOrigin = worldPos + worldNormal * adaptiveBias;

	/* Determine light count from the light SSBO.
	 * We use the same approach as RTR: lightCount is encoded as an extra push constant
	 * field. Since RTGI's TracePushConstants doesn't have a separate lightCount field,
	 * we read the total number of lights from the SSBO length heuristic.
	 * For simplicity, we hard-limit to 16 lights for GI bounces. */
	uint lightCount = min(uint(lightSSBO.lights.length()) / 4u, 16u);

	/* Accumulate indirect radiance. */
	vec3 indirectLight = vec3(0.0);

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, sampleCount, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, worldNormal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Trace ONE ray that answers both questions at once — the bounce and the sky.
		 * tMax is the SKY distance, not the bounce distance: "nothing hit within 8 m" does not
		 * mean "sees the sky", it means the bounce range is empty. Stopping there and calling
		 * it sky would light the far end of a corridor through 15 m of building. So the ray
		 * runs to the sky distance and the three outcomes are read from the hit distance:
		 *   miss              -> the sky is visible in that direction (a light source);
		 *   hit beyond range  -> geometry blocks the sky, too far to carry a bounce (nothing);
		 *   hit within range  -> a real bounce, shaded below.
		 * tMin is a tiny CONSTANT, decoupled from the adaptive origin offset (same fix as
		 * RTAO): a tMin equal to the adaptive bias skips real geometry closer than it and
		 * leaks light at wall/floor creases. */
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(
			rayQuery, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF,
			rayOrigin, 0.001, sampleDir, skyDistance
		);

		while (rayQueryProceedEXT(rayQuery)) {}

		if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
		{
			/* The ray escaped: this direction sees the sky. Cosine weighting is implicit in the
			 * hemisphere distribution, so the radiance enters the estimator unmodified (the
			 * 1/N is applied once after the loop; the receiver albedo at the combine). */
			indirectLight += skyRadiance(sampleDir);

			continue;
		}

		{
			float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);

			/* Geometry occludes the sky but sits outside the bounce range: it contributes no
			 * light, and that ABSENCE is the shadowing — this is what carves the vertical
			 * gradient of a courtyard out of the sky term. */
			if (hitT > maxDistance)
			{
				continue;
			}

			uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
			uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
			vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

			/* Unpack mesh data. */
			MeshAccessor mesh = getMeshAccessor(instanceIndex, primitiveIndex);

			/* Look up material per sub-geometry via geometryIndex from the ray query.
			 * Clamp to subGeometryCount to handle BLAS with more sub-geometries than the
			 * renderable has material slots (e.g. animated sprite frame groups). */
			uint geomIdx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
			uint subGeoCount = meshSSBO.meshEntries[instanceIndex * 3u + 1u].w;
			uint effectiveGeomIdx = (geomIdx < subGeoCount) ? geomIdx : 0u;
			uint materialIndex = meshSSBO.meshEntries[instanceIndex * 3u + 2u][effectiveGeomIdx];
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
			vec3 hitPos = rayOrigin + sampleDir * hitT;

			vec3 objectNormal = getHitNormal(mesh, barycentrics);
			mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
			vec3 hitNormal = normalize(mat3(objectToWorld) * objectNormal);

			/* Compute direct lighting at the hit point. */
			vec3 lighting = computeDirectLighting(hitPos, hitNormal, lightCount);

			/* The indirect radiance is the hit surface's albedo lit by direct light
			 * (one traced bounce), PLUS the indirect radiance the hit surface itself
			 * accumulated in the previous resolved frame (multi-bounce feedback).
			 * The feedback IS multiplied by the hit albedo: the history stores
			 * DEMODULATED irradiance (receiver albedo deferred to the combine pass),
			 * so the hit albedo converts it back into outgoing radiance here.
			 * Cosine-weighted by the hemisphere sampling (implicit in the distribution).
			 *
			 * Range fade: the transfer itself is already governed by the solid angle, so a
			 * fade proportional to the distance is NOT physical — it used to run linearly from
			 * the surface, halving a bounce found at mid-range and costing about a factor two
			 * of indirect energy against the screen-space path (measured on Sponza). It now
			 * only smooths the LAST FIFTH of the range, whose sole purpose is to keep geometry
			 * from popping as it crosses the maxDistance boundary. */
			float distFade = 1.0 - smoothstep(maxDistance * 0.8, maxDistance, hitT);

			/* Lambert BRDF energy conservation: divide by PI. */
			indirectLight += ((albedo / PI) * lighting + albedo * historyFeedback(hitPos)) * distFade;
		}
	}

	/* Normalize by sample count. The signal stays DEMODULATED (irradiance estimate, NO receiver
	 * albedo): the blur/temporal chain then denoises a smooth signal, and the albedo is
	 * re-applied at FULL resolution in the combine pass — half-res + bilateral blur no longer
	 * destroy the texture detail in GI-dominated (dark) areas. Standard albedo demodulation,
	 * as in SVGF (Schied et al. 2017, HPG) and NVIDIA NRD. */
	indirectLight = indirectLight / float(sampleCount);

	outIndirect = vec4(indirectLight, 1.0);
}
)GLSL";

	/* Temporal resolve pass: exponential moving average between the current blurred GI and
	 * the reprojected history. The history UV is found by projecting the pixel's world
	 * position through the PREVIOUS frame's view-projection (static-geometry reprojection —
	 * per-object motion vectors come later with the dedicated MRT attachment). History is
	 * rejected on disocclusion (camera-distance mismatch, normal mismatch) and optionally
	 * clamped to the current 3x3 neighbourhood range (anti-ghosting).
	 * Output: RGB = resolved DEMODULATED indirect irradiance (receiver albedo applied at
	 * the combine), A = camera distance (0 = invalid/sky).
	 *
	 * Descriptor set 0:
	 *   binding 0: current blurred GI (blur V output)
	 *   binding 1: depth texture
	 *   binding 2: normals texture (view space)
	 *   binding 3: GI history texture (previous resolved frame)
	 *   binding 4: world-normal history texture (previous frame)
	 *   binding 5: velocity texture (RG16F NDC-delta motion vectors)
	 *   binding 6: frame UBO (shared with the trace pass)
	 */
	constexpr auto RTGITemporalFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolved;

layout(set = 0, binding = 0) uniform sampler2D giTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 3) uniform sampler2D historyTex;
layout(set = 0, binding = 4) uniform sampler2D historyNormalTex;
layout(set = 0, binding = 5) uniform sampler2D velocityTex;

layout(set = 0, binding = 6, std140) uniform FrameData
{
	mat4 invViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = unused. */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags. */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z/w = unused. */
};

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky/far-plane: no surface, invalid history marker (a = 0). */
	if (depth >= 1.0)
	{
		outResolved = vec4(0.0);
		return;
	}

	vec3 current = texture(giTex, vUV).rgb;

	/* Reconstruct world-space position from NDC + depth via inverse VP. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 wp = invViewProj * clipPos;
	vec3 worldPos = wp.xyz / wp.w;

	vec3 viewPos = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);
	float cameraDistance = length(worldPos - viewPos);

	/* Current world-space normal, for the history normal comparison. */
	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRot * normalize(texture(normalTex, vUV).rgb));

	float alpha = temporalParams.x;

	/* Reproject into the previous frame through the velocity buffer (per-object motion
	 * vectors, NDC delta = current - previous). Velocity DILATION: use the velocity of
	 * the 3x3 neighbour closest to the camera, so thin foreground silhouettes drag their
	 * motion over the background edge pixels instead of smearing. */
	vec2 texelD = 1.0 / vec2(textureSize(depthTex, 0));
	vec2 closestOffset = vec2(0.0);
	float closestDepth = depth;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec2 offset = vec2(x, y) * texelD;
			float d = texture(depthTex, vUV + offset).r;

			if (d < closestDepth)
			{
				closestDepth = d;
				closestOffset = offset;
			}
		}
	}

	vec2 velocity = texture(velocityTex, vUV + closestOffset).rg;
	vec2 prevUV = vUV - velocity * 0.5;

	if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
	{
		/* Off-screen: no history, full weight on the current estimate. */
		outResolved = vec4(current, cameraDistance);
		return;
	}

	vec4 history = texture(historyTex, prevUV);

	/* Disocclusion test 1: camera-distance mismatch (rotation-invariant). */
	float expectedDistance = length(worldPos - prevCamPos.xyz);
	bool distanceValid = history.a > 0.0 && abs(history.a - expectedDistance) <= temporalParams.y * expectedDistance;

	/* Disocclusion test 2: world-normal mismatch (silhouettes, grazing surfaces). */
	vec3 prevNormal = texture(historyNormalTex, prevUV).xyz;
	bool normalValid = dot(prevNormal, worldNormal) >= temporalParams.z;

	if (!distanceValid || !normalValid)
	{
		outResolved = vec4(current, cameraDistance);
		return;
	}

	/* Neighborhood clamp (flag bit 0): bound the history to the current 3x3 range so a
	 * stale-but-plausible history cannot drag the result far from what is observed now. */
	if ((uint(temporalParams.w) & 1u) != 0u)
	{
		vec2 texel = 1.0 / vec2(textureSize(giTex, 0));
		vec3 nbMin = current;
		vec3 nbMax = current;

		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				vec3 nb = texture(giTex, vUV + vec2(x, y) * texel).rgb;
				nbMin = min(nbMin, nb);
				nbMax = max(nbMax, nb);
			}
		}

		history.rgb = clamp(history.rgb, nbMin, nbMax);
	}

	outResolved = vec4(mix(history.rgb, current, alpha), cameraDistance);
}
)GLSL";

	/* Normal history pass: converts the current view-space normals G-buffer to world space
	 * (camera-rotation invariant) and stores it at history resolution for the NEXT frame's
	 * temporal validation. The normals MRT attachment is rewritten every frame, so the
	 * previous frame's normals must be explicitly retained.
	 *
	 * Descriptor set 0:
	 *   binding 0: normals texture (view space, current frame)
	 *   binding 1: frame UBO (shared with the trace pass)
	 */
	constexpr auto RTGINormalCopyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outWorldNormal;

layout(set = 0, binding = 0) uniform sampler2D normalTex;

layout(set = 0, binding = 1, std140) uniform FrameData
{
	mat4 invViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = unused. */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags. */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z/w = unused. */
};

void main()
{
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outWorldNormal = vec4(0.0);
		return;
	}

	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);

	outWorldNormal = vec4(normalize(invViewRot * normalize(rawN)), 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RTGI::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* Pixel doubling: half-res for performance (default), full-res for quality. */
		const auto pixelDoubling = settings.getOrSetDefault< bool >(GraphicsRayTracingGIPixelDoublingKey, DefaultGraphicsRayTracingGIPixelDoubling);
		const auto halfW = pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? ((height > 1) ? height / 2 : 1U) : height;

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		m_parameters.maxDistance = settings.getOrSetDefault< float >(GraphicsRayTracingGIMaxDistanceKey, DefaultGraphicsRayTracingGIMaxDistance);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsRayTracingGIIntensityKey, DefaultGraphicsRayTracingGIIntensity);
		m_parameters.bias = settings.getOrSetDefault< float >(GraphicsRayTracingGIBiasKey, DefaultGraphicsRayTracingGIBias);
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingGISampleCountKey, DefaultGraphicsRayTracingGISampleCount);
		m_parameters.blurRadius = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingGIBlurRadiusKey, DefaultGraphicsRayTracingGIBlurRadius);
		m_parameters.depthSigma = settings.getOrSetDefault< float >(GraphicsRayTracingGIDepthSigmaKey, DefaultGraphicsRayTracingGIDepthSigma);
		m_parameters.normalSigma = settings.getOrSetDefault< float >(GraphicsRayTracingGINormalSigmaKey, DefaultGraphicsRayTracingGINormalSigma);
		m_parameters.temporalAlpha = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalAlphaKey, DefaultGraphicsRayTracingGITemporalAlpha);
		m_parameters.temporalDepthTolerance = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalDepthToleranceKey, DefaultGraphicsRayTracingGITemporalDepthTolerance);
		m_parameters.temporalNormalThreshold = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalNormalThresholdKey, DefaultGraphicsRayTracingGITemporalNormalThreshold);
		m_parameters.multiBounceStrength = settings.getOrSetDefault< float >(GraphicsRayTracingGIMultiBounceStrengthKey, DefaultGraphicsRayTracingGIMultiBounceStrength);
		m_parameters.multiBounceClamp = settings.getOrSetDefault< float >(GraphicsRayTracingGIMultiBounceClampKey, DefaultGraphicsRayTracingGIMultiBounceClamp);
		m_parameters.temporalEnabled = settings.getOrSetDefault< bool >(GraphicsRayTracingGITemporalEnabledKey, DefaultGraphicsRayTracingGITemporalEnabled);
		m_parameters.temporalNeighborhoodClamp = settings.getOrSetDefault< bool >(GraphicsRayTracingGITemporalNeighborhoodClampKey, DefaultGraphicsRayTracingGITemporalNeighborhoodClamp);
		m_parameters.multiBounceEnabled = settings.getOrSetDefault< bool >(GraphicsRayTracingGIMultiBounceEnabledKey, DefaultGraphicsRayTracingGIMultiBounceEnabled);

		/* History starts invalid: the first frame after (re)creation must not read the
		 * uninitialized ping-pong images (alpha forced to 1, no multi-bounce feedback). */
		m_historyValid = false;
		m_historyWriteIndex = 0;

		/* Trace target (half-res, RGBA16F: indirect radiance RGB). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_Trace") )
		{
			TraceError{ClassId} << "Failed to create RTGI trace target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_BlurH") )
		{
			TraceError{ClassId} << "Failed to create RTGI blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_BlurV") )
		{
			TraceError{ClassId} << "Failed to create RTGI blur V target !";

			return false;
		}

		/* Temporal history targets (half-res, ping-pong). Only allocated when the temporal
		 * accumulation is enabled, so the disabled path costs no VRAM. */
		if ( m_parameters.temporalEnabled )
		{
			for ( size_t index = 0; index < 2; ++index )
			{
				const auto suffix = std::to_string(index);

				if ( !m_historyTargets[index].create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_History" + suffix) )
				{
					TraceError{ClassId} << "Failed to create RTGI history target #" << index << " !";

					return false;
				}

				if ( !m_normalHistoryTargets[index].create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_NormalHistory" + suffix) )
				{
					TraceError{ClassId} << "Failed to create RTGI normal history target #" << index << " !";

					return false;
				}
			}
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals + GI history samplers, plus the frame UBO
		 * (the per-frame data outgrew the 128-byte push constant minimum). The receiver
		 * albedo is NOT read here anymore — the trace outputs demodulated irradiance and
		 * the combine pass re-applies the albedo at full resolution. */
		auto traceInputLayout = this->getInputLayout(3, 1);

		/* Temporal resolve input: GI + depth + normals + history + normal history, plus the frame UBO. */
		auto temporalInputLayout = this->getInputLayout(6, 1);

		/* Normal history input: normals, plus the frame UBO. */
		auto normalCopyInputLayout = this->getInputLayout(1, 1);

		if ( traceInputLayout == nullptr || temporalInputLayout == nullptr || normalCopyInputLayout == nullptr )
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
			/* Trace: set 0 = RT data, set 1 = input textures + frame UBO, set 2 = bindless textures.
			 * No push constants: the per-frame data lives in the UBO. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {});
		}

		{
			/* Temporal resolve: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(temporalInputLayout);

			m_temporalLayout = layoutManager.getPipelineLayout(sets, {});
		}

		{
			/* Normal history: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(normalCopyInputLayout);

			m_normalCopyLayout = layoutManager.getPipelineLayout(sets, {});
		}

		if ( m_traceLayout == nullptr || m_temporalLayout == nullptr || m_normalCopyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTGI_Trace_FS", ShaderType::FragmentShader, RTGITraceFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTGI shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "RTGI_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);

		if ( m_tracePipeline == nullptr )
		{
			return false;
		}

		if ( m_parameters.temporalEnabled )
		{
			const auto temporalFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTGI_Temporal_FS", ShaderType::FragmentShader, RTGITemporalFragmentShader);
			const auto normalCopyFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTGI_NormalCopy_FS", ShaderType::FragmentShader, RTGINormalCopyFragmentShader);

			if ( temporalFragment == nullptr || normalCopyFragment == nullptr )
			{
				TraceError{ClassId} << "Failed to compile RTGI temporal shaders !";

				return false;
			}

			m_temporalPipeline = this->createFullscreenPipeline(ClassId, "RTGI_Temporal", vertexModule, temporalFragment, m_temporalLayout, m_historyTargets[0]);
			m_normalCopyPipeline = this->createFullscreenPipeline(ClassId, "RTGI_NormalCopy", vertexModule, normalCopyFragment, m_normalCopyLayout, m_normalHistoryTargets[0]);

			if ( m_temporalPipeline == nullptr || m_normalCopyPipeline == nullptr )
			{
				return false;
			}
		}

		/* ---- Per-frame UBOs (shared by trace/temporal/normal-copy passes) ---- */
		m_frameUBOs = this->createPerFrameUniformBuffers(sizeof(FrameUBOData), ClassId, "Frame_UBO");

		if ( m_frameUBOs.empty() )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Trace: set 1 reads depth + normals + history (updated per-frame),
		 * plus the frame UBO (written once here, rewritten CPU-side every frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		for ( size_t f = 0; f < m_tracePerFrame.size(); ++f )
		{
			if ( !m_tracePerFrame[f]->writeUniformBufferObject(3, *m_frameUBOs[f]) )
			{
				return false;
			}

			/* The history binding must always hold a VALID descriptor (the shader statically
			 * uses it even when the feedback is disabled at runtime). When the temporal chain
			 * is off, bind the trace target as an inert placeholder (strength is 0). */
			if ( !m_parameters.temporalEnabled )
			{
				if ( !m_tracePerFrame[f]->writeCombinedImageSampler(2, m_traceTarget) )
				{
					return false;
				}
			}
		}

		/* Temporal resolve + normal history sets (per-frame; texture bindings are
		 * rewritten every frame because of the history ping-pong). */
		if ( m_parameters.temporalEnabled )
		{
			m_temporalPerFrame = this->createPerFrameDescriptorSets(temporalInputLayout, ClassId, "Temporal_DescSet");
			m_normalCopyPerFrame = this->createPerFrameDescriptorSets(normalCopyInputLayout, ClassId, "NormalCopy_DescSet");

			if ( m_temporalPerFrame.empty() || m_normalCopyPerFrame.empty() )
			{
				return false;
			}

			for ( size_t f = 0; f < m_temporalPerFrame.size(); ++f )
			{
				if ( !m_temporalPerFrame[f]->writeCombinedImageSampler(0, m_blurVTarget) )
				{
					return false;
				}

				if ( !m_temporalPerFrame[f]->writeUniformBufferObject(6, *m_frameUBOs[f]) )
				{
					return false;
				}

				if ( !m_normalCopyPerFrame[f]->writeUniformBufferObject(1, *m_frameUBOs[f]) )
				{
					return false;
				}
			}
		}

		/* Combine source default: the blurred trace. When the temporal chain is active,
		 * recordPostDenoisePasses() retargets it to the freshly resolved history every frame. */
		m_combineSource = &m_blurVTarget;

		return true;
	}

	void
	RTGI::destroy () noexcept
	{
		m_combineSource = nullptr;

		m_normalCopyPerFrame.clear();
		m_temporalPerFrame.clear();
		m_tracePerFrame.clear();

		m_frameUBOs.clear();

		m_normalCopyPipeline.reset();
		m_temporalPipeline.reset();
		m_tracePipeline.reset();
		m_normalCopyLayout.reset();
		m_temporalLayout.reset();
		m_traceLayout.reset();

		for ( auto & target : m_normalHistoryTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_historyTargets )
		{
			target.destroy();
		}

		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();

		m_historyValid = false;
		m_historyWriteIndex = 0;
	}

	void
	RTGI::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;

		const auto frameIndex = this->renderer().currentFrameIndex();

		const bool temporalActive = m_parameters.temporalEnabled && m_temporalPipeline != nullptr;

		/* ---- Per-frame descriptor updates (trace pass) ---- */

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		if ( temporalActive )
		{
			/* History ping-pong: this frame reads [1 - writeIdx]. The flip only happens at
			 * the end of recordPostDenoisePasses(), so m_historyWriteIndex is stable here. */
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, m_historyTargets[1U - m_historyWriteIndex]));
		}

		/* ---- Frame UBO (shared by trace/temporal/normal-copy passes) ---- */
		{
			/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
			const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
			/* JITTERED (the default projectionMatrix contract): the depth buffer was rasterized
			 * with the TAA jitter, so unprojecting with the same matrix is geometrically exact.
			 * NOTE (measured 2026-08-05): swapping this for unjitteredProjectionMatrix() — to
			 * cancel the reprojection error against the unjittered previousProjectionMatrix() —
			 * had NO measurable effect on the temporal peak-to-peak (runs within the ×1.85
			 * run-to-run envelope). The GI temporal noise is content/RT-driven, not matrix-driven. */
			const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
			const auto invViewProj = (projMat * viewMat).inverse();
			const auto * ivp = invViewProj.data();

			/* Inverse view rotation for normal transformation (view → world). */
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			/* Previous rendered frame (ViewMatrices frame-history contract). Identity
			 * until the first frame is archived — irrelevant then, since the history is
			 * flagged invalid (alpha forced to 1, feedback strength forced to 0). */
			const auto & prevViewMat = viewMatrices.previousViewMatrix();
			const auto prevViewProj = viewMatrices.previousProjectionMatrix() * prevViewMat;
			const auto * pvp = prevViewProj.data();

			const auto prevInvView = prevViewMat.inverse();
			const auto * pinv = prevInvView.data();

			const bool historyUsable = temporalActive && m_historyValid;

			const FrameUBOData ubo{
				.invViewProj = {
					ivp[0], ivp[1], ivp[2], ivp[3],
					ivp[4], ivp[5], ivp[6], ivp[7],
					ivp[8], ivp[9], ivp[10], ivp[11],
					ivp[12], ivp[13], ivp[14], ivp[15]
				},
				.prevViewProj = {
					pvp[0], pvp[1], pvp[2], pvp[3],
					pvp[4], pvp[5], pvp[6], pvp[7],
					pvp[8], pvp[9], pvp[10], pvp[11],
					pvp[12], pvp[13], pvp[14], pvp[15]
				},
				.invViewCol0 = {inv[0], inv[1], inv[2]},
				.viewPosX = inv[12],
				.invViewCol1 = {inv[4], inv[5], inv[6]},
				.viewPosY = inv[13],
				.invViewCol2 = {inv[8], inv[9], inv[10]},
				.viewPosZ = inv[14],
				.prevCamPos = {pinv[12], pinv[13], pinv[14], 0.0F},
				.traceParams = {m_parameters.maxDistance, m_parameters.bias, static_cast< float >(m_parameters.sampleCount), 0.0F},
				.temporalParams = {
					historyUsable ? m_parameters.temporalAlpha : 1.0F,
					m_parameters.temporalDepthTolerance,
					m_parameters.temporalNormalThreshold,
					m_parameters.temporalNeighborhoodClamp ? 1.0F : 0.0F
				},
				.bounceParams = {
					historyUsable && m_parameters.multiBounceEnabled ? m_parameters.multiBounceStrength : 0.0F,
					m_parameters.multiBounceClamp,
					0.0F,
					0.0F
				},
				/* THE SKY IS A LIGHT SOURCE. The luminance comes from the scene background
				 * (0 = no background, no sky light). The sky distance is how far a ray must
				 * travel before "hit nothing" may be read as "sees the sky": the far plane is
				 * the frame's own "nothing beyond this exists" bound, so distant geometry can
				 * never be mistaken for open sky. */
				.skyParams = {context.skyLuminance, context.constants.farPlane, 0.0F, 0.0F}
			};

			if ( !IndirectPostProcessEffect::updateUniformBufferData(*m_frameUBOs[frameIndex], &ubo, sizeof(FrameUBOData)) )
			{
				TraceError{ClassId} << "Failed to update the RTGI frame UBO !";
			}
		}

		/* ---- Pass 1: Ray Trace GI ---- */
		{
			/* Custom recording: bind set 0 (RT) from Renderer, set 1 (input textures + UBO) per-frame. */
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

			/* Bind set 1: Input textures + frame UBO. */
			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

			/* Bind set 2: Bindless textures. */
			if ( const auto * bindlessDescSet = this->renderer().bindlessTextureManager().descriptorSet(); bindlessDescSet != nullptr )
			{
				commandBuffer.bind(*bindlessDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
			}

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);
		}
	}

	IndirectPostProcessEffect::DenoiseContribution
	RTGI::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "rtgi";
		contribution.source = &m_traceTarget;
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);
		contribution.needsDepth = true;
		contribution.needsNormals = true;
		contribution.dynamics = Base::Math::Vector< 4, float >{m_parameters.depthSigma, m_parameters.normalSigma, static_cast< float >(m_parameters.blurRadius), 0.0F};

		/* Same depth/normal-aware bilateral kernel as the retired RTGI_Blur_FS pass.
		 * Dynamics0: x = depthSigma, y = normalSigma, z = blurRadius. */
		contribution.code =
			"\tvec2 rtgiTexel = 1.0 / vec2(textureSize(rtgiSrc, 0));\n"
			"\tvec4 rtgiCenter = texture(rtgiSrc, vUV);\n"
			"\tfloat rtgiCenterDepth = texture(emDepth, vUV).r;\n"
			"\tvec3 rtgiCenterNormal = texture(emNormals, vUV).rgb;\n"
			"\tvec4 rtgiResult = rtgiCenter;\n"
			"\t/* Skip far-plane fragments. */\n"
			"\tif (rtgiCenterDepth < 1.0)\n"
			"\t{\n"
			"\t\tint rtgiRadius = int(emDyn.rtgiDynamics0.z);\n"
			"\t\tfloat rtgiSpatialSigma = float(rtgiRadius) * 0.5;\n"
			"\t\tfloat rtgiInvSpatialSigma2 = 1.0 / (2.0 * rtgiSpatialSigma * rtgiSpatialSigma);\n"
			"\t\tfloat rtgiInvDepthSigma2 = 1.0 / (2.0 * emDyn.rtgiDynamics0.x * emDyn.rtgiDynamics0.x);\n"
			"\t\tvec4 rtgiSum = vec4(0.0);\n"
			"\t\tfloat rtgiTotalWeight = 0.0;\n"
			"\t\tfor (int rtgiI = -rtgiRadius; rtgiI <= rtgiRadius; rtgiI++)\n"
			"\t\t{\n"
			"\t\t\tvec2 rtgiUV = vUV + emDenoiseDir * rtgiTexel * float(rtgiI);\n"
			"\t\t\tvec4 rtgiSample = texture(rtgiSrc, rtgiUV);\n"
			"\t\t\tfloat rtgiDepth = texture(emDepth, rtgiUV).r;\n"
			"\t\t\tvec3 rtgiNormal = texture(emNormals, rtgiUV).rgb;\n"
			"\t\t\tfloat rtgiSpatialW = exp(-float(rtgiI * rtgiI) * rtgiInvSpatialSigma2);\n"
			"\t\t\tfloat rtgiDepthDiff = abs(rtgiCenterDepth - rtgiDepth);\n"
			"\t\t\tfloat rtgiDepthW = exp(-rtgiDepthDiff * rtgiDepthDiff * rtgiInvDepthSigma2);\n"
			"\t\t\tfloat rtgiNormalDot = max(dot(rtgiCenterNormal, rtgiNormal), 0.0);\n"
			"\t\t\tfloat rtgiNormalW = pow(rtgiNormalDot, 1.0 / max(emDyn.rtgiDynamics0.y, 0.001));\n"
			"\t\t\tfloat rtgiW = rtgiSpatialW * rtgiDepthW * rtgiNormalW;\n"
			"\t\t\trtgiSum += rtgiSample * rtgiW;\n"
			"\t\t\trtgiTotalWeight += rtgiW;\n"
			"\t\t}\n"
			"\t\tif (rtgiTotalWeight > 0.0)\n"
			"\t\t{\n"
			"\t\t\trtgiResult = rtgiSum / rtgiTotalWeight;\n"
			"\t\t}\n"
			"\t}\n";

		return contribution;
	}

	void
	RTGI::recordPostDenoisePasses (const CommandBuffer & commandBuffer, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		const bool temporalActive = m_parameters.temporalEnabled && m_temporalPipeline != nullptr;
		const uint32_t writeIdx = m_historyWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		/* The combine snippet consumes the freshly resolved history when the temporal
		 * chain is active — [writeIdx] is written THIS frame, captured here BEFORE the
		 * ping-pong flip below — and the blurred trace otherwise. */
		m_combineSource = temporalActive ? static_cast< const TextureInterface * >(&m_historyTargets[writeIdx]) : &m_blurVTarget;

		if ( !temporalActive )
		{
			return;
		}

		/* ---- Per-frame descriptor updates (temporal chain) ---- */

		/* History ping-pong: this frame reads [readIdx] and writes [writeIdx]. */
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(3, m_historyTargets[readIdx]));
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(4, m_normalHistoryTargets[readIdx]));

		if ( context.depth != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));
		}

		if ( context.normals != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.normals));
			static_cast< void >(m_normalCopyPerFrame[frameIndex]->writeCombinedImageSampler(0, *context.normals));
		}

		if ( context.velocity != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(5, *context.velocity));
		}

		/* ---- Pass 4: Temporal resolve + Pass 5: Normal history ---- */
		{
			/* NOTE: The pipelines were created against the [0] targets; recording into [1]
			 * relies on Vulkan render pass compatibility (identical format/ops), exactly
			 * like the shared denoise pipeline recording into both blur targets. */
			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_historyTargets[writeIdx],
				*m_temporalPipeline,
				*m_temporalLayout,
				*m_temporalPerFrame[frameIndex],
				nullptr,
				0
			);

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_normalHistoryTargets[writeIdx],
				*m_normalCopyPipeline,
				*m_normalCopyLayout,
				*m_normalCopyPerFrame[frameIndex],
				nullptr,
				0
			);
		}

		/* Flip the history ping-pong for the next frame. */
		m_historyWriteIndex = readIdx;
		m_historyValid = true;
	}

	IndirectPostProcessEffect::CombineContribution
	RTGI::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "rtgi";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", m_combineSource});
		contribution.needsMaterialProperties = true;
		contribution.needsAlbedo = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* The traced signal is DEMODULATED irradiance: the receiver albedo is re-applied
		 * HERE, at full resolution, so the half-res trace + bilateral blur + temporal chain
		 * never touch the texture detail (albedo demodulation, as in SVGF / NVIDIA NRD —
		 * same convention as SSGI). Emissive surfaces reject GI (they emit their own
		 * light), then the user intensity scales the additive blend. */
		contribution.code =
			"\tvec3 rtgiGI = texture(rtgiTex, vUV).rgb;\n"
			"\trtgiGI *= texture(emAlbedo, vUV).rgb;\n"
			"\tvec4 rtgiMp = texture(emMaterialProps, vUV);\n"
			"\tfloat rtgiEmissive = float(uint(rtgiMp.b * 255.0) & 0xFu) / 15.0;\n"
			"\trtgiGI *= (1.0 - rtgiEmissive);\n"
			"\tem_Color.rgb += rtgiGI * emDyn.rtgiDynamics0.x;\n";

		return contribution;
	}
}
