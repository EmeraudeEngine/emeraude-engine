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
#include "RTAlphaTestGLSL.hpp"

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
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = animated-noise frame index (R2). */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags (bit0 variance clip, bit1 animated noise). */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z = variance-clip gamma, w = unused. */
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

	/* ENGINE CUBEMAP CONVENTION (Y-UP): a world direction D samples the cubemap RAW, as D itself.
	 * ⚠️ The comment here used to justify a `(D.x, -D.y, D.z)` negation that the Y-up flip deleted —
	 * the code below already sampled the raw direction, so the text described a compensation that
	 * was no longer there. Same contract as the skybox (Material/Helpers.cpp) and the material
	 * reflections: no negation anywhere. */
	return texture(texturesCube[nonuniformEXT(EnvironmentCubemapSlot)], direction).rgb * skyParams.x;
}

/* NOTE: GLSL has no built-in PI constant. */
const float PI = 3.14159265;

/* Material flag bits (must match GPURTMaterialData). */
const uint HasAlbedoTexture = 1u;
const uint HasMetalnessTexture = 1u << 3;
const uint HasOpacityTexture = 1u << 7;
const uint IsAlphaTest = 1u << 8;

/* Packed texel channel index (0:R, 1:G, 2:B, 3:A) — matches
 * GPURTMaterialData::MetalnessChannelShift. */
const uint MetalnessChannelShift = 18u;
const uint ChannelMask = 3u;

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

)GLSL" EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(
/* Shadow ray: returns 1.0 when the path from the surface toward the light is unoccluded,
 * 0.0 otherwise. TerminateOnFirstHit: the first CONFIRMED candidate ends the traversal.
 * ⚠️ NOT gl_RayFlagsOpaqueEXT: that flag accepts every triangle of an alpha-tested instance
 * whole, and the ivy of Sponza cast a SOLID shadow at every bounce hit while the raster drew
 * leaves. The candidates are judged by the shared alpha-test rule (RTAlphaTestGLSL.hpp). */
float shadowRayVisibility (vec3 origin, vec3 direction, float maxT)
{
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

	/* Temporal decorrelation (flag bit 1, requires the temporal resolve): advance the
	 * per-pixel rotation every frame along the R2 low-discrepancy sequence (M. Roberts,
	 * "The Unreasonable Effectiveness of Quasirandom Sequences", 2018) so the temporal
	 * EMA AVERAGES the estimator error over the frames instead of freezing it as a
	 * static pattern — which TAA's sub-pixel resampling turns into visible shimmer
	 * (owner-isolated, 2026-08-05: TAA -> FXAA froze the noise). The flag is NEVER set
	 * when the temporal chain is off: animated noise without accumulation boils. */
	if ((uint(temporalParams.w) & 2u) != 0u)
	{
		noiseVec = fract(noiseVec + traceParams.w * vec2(0.7548776662, 0.5698402909));
	}

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
		/* ⚠️ NOT gl_RayFlagsOpaqueEXT: a cutout instance (foliage, fences) is FORCE_NO_OPAQUE and
		 * hands its triangles over as candidates for the shader to judge — the opaque flag
		 * overrode that and a leaf blocked the sky as a solid quad. Same rule as RTR. */
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(
			rayQuery, topLevelAS, gl_RayFlagsNoneEXT, 0xFF,
			rayOrigin, 0.001, sampleDir, skyDistance
		);

	while (rayQueryProceedEXT(rayQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(rayQuery) R"GLSL(
	}

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

			/* Material per sub-geometry, clamped to the renderable's slots (shared RT rule). */
			uint materialIndex = rtHitMaterialIndex(instanceIndex, rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true));
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

			/* A bounce is a DIFFUSE event, so the base color must become the DIFFUSE albedo: a
			 * metal has no diffuse lobe (its base color is the F0 of its specular one), and
			 * shading one as a Lambertian sheet re-emits energy it never scatters that way —
			 * gold (baseColor 1.00/0.72/0.32, metalness 1) bounced 72% of the light back into
			 * the scene. Same convention as the receiver side (the albedo G-buffer carries the
			 * diffuse albedo too) and as NVIDIA MathLib: baseColor * saturate(1 - metalness).
			 * ⚠️ It also damps the multi-bounce series through the SAME albedo product, which is
			 * what keeps that geometric series convergent. */
			float hitMetalness = materialSSBO.materials[matBase + 1u].y;

			if ((flags & HasMetalnessTexture) != 0u)
			{
				int metalTexIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].w);

				if (metalTexIndex >= 0)
				{
					vec2 metalUV = getHitUV(mesh, barycentrics);
					hitMetalness *= texture(textures2D[nonuniformEXT(metalTexIndex)], metalUV)[(flags >> MetalnessChannelShift) & ChannelMask];
				}
			}

			albedo *= 1.0 - clamp(hitMetalness, 0.0, 1.0);

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
		m_parameters.depthSigma = settings.getOrSetDefault< float >(GraphicsRayTracingGIDepthSigmaKey, DefaultGraphicsRayTracingGIDepthSigma);
		m_parameters.normalSigma = settings.getOrSetDefault< float >(GraphicsRayTracingGINormalSigmaKey, DefaultGraphicsRayTracingGINormalSigma);
		m_parameters.luminanceSigma = settings.getOrSetDefault< float >(GraphicsRayTracingGIDenoiserLuminanceSigmaKey, DefaultGraphicsRayTracingGIDenoiserLuminanceSigma);
		m_parameters.atrousIterations = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingGIDenoiserIterationsKey, DefaultGraphicsRayTracingGIDenoiserIterations);
		m_parameters.denoiserMaxAccumulation = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingGIDenoiserMaxAccumulationKey, DefaultGraphicsRayTracingGIDenoiserMaxAccumulation);
		m_parameters.denoiserAccumulationCounter = settings.getOrSetDefault< bool >(GraphicsRayTracingGIDenoiserAccumulationCounterKey, DefaultGraphicsRayTracingGIDenoiserAccumulationCounter);
		m_parameters.temporalAlpha = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalAlphaKey, DefaultGraphicsRayTracingGITemporalAlpha);
		m_parameters.temporalDepthTolerance = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalDepthToleranceKey, DefaultGraphicsRayTracingGITemporalDepthTolerance);
		m_parameters.temporalNormalThreshold = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalNormalThresholdKey, DefaultGraphicsRayTracingGITemporalNormalThreshold);
		m_parameters.temporalVarianceGamma = settings.getOrSetDefault< float >(GraphicsRayTracingGITemporalVarianceGammaKey, DefaultGraphicsRayTracingGITemporalVarianceGamma);
		m_parameters.multiBounceStrength = settings.getOrSetDefault< float >(GraphicsRayTracingGIMultiBounceStrengthKey, DefaultGraphicsRayTracingGIMultiBounceStrength);
		m_parameters.multiBounceClamp = settings.getOrSetDefault< float >(GraphicsRayTracingGIMultiBounceClampKey, DefaultGraphicsRayTracingGIMultiBounceClamp);
		m_parameters.denoiserDebugView = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingGIDenoiserDebugViewKey, DefaultGraphicsRayTracingGIDenoiserDebugView);
		m_parameters.temporalEnabled = settings.getOrSetDefault< bool >(GraphicsRayTracingGITemporalEnabledKey, DefaultGraphicsRayTracingGITemporalEnabled);
		m_parameters.temporalNeighborhoodClamp = settings.getOrSetDefault< bool >(GraphicsRayTracingGITemporalNeighborhoodClampKey, DefaultGraphicsRayTracingGITemporalNeighborhoodClamp);
		m_parameters.temporalAnimatedNoise = settings.getOrSetDefault< bool >(GraphicsRayTracingGITemporalAnimatedNoiseKey, DefaultGraphicsRayTracingGITemporalAnimatedNoise);
		m_parameters.multiBounceEnabled = settings.getOrSetDefault< bool >(GraphicsRayTracingGIMultiBounceEnabledKey, DefaultGraphicsRayTracingGIMultiBounceEnabled);

		/* Trace target (half-res, RGBA16F: indirect radiance RGB). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_Trace") )
		{
			TraceError{ClassId} << "Failed to create RTGI trace target !";

			return false;
		}

		/* The denoiser component (temporal resolve + moments + à-trous + histories + frame
		 * UBO). Created BEFORE the trace descriptor sets: they bind its frame UBO and, when
		 * the temporal chain is active, its history texture. */
		m_denoiser.setTemporalEnabled(m_parameters.temporalEnabled);
		m_denoiser.setParameters(GIDenoiser::Parameters{
			.depthSigma = m_parameters.depthSigma,
			.normalSigma = m_parameters.normalSigma,
			.luminanceSigma = m_parameters.luminanceSigma,
			.atrousIterations = m_parameters.atrousIterations,
			.temporalAlpha = m_parameters.temporalAlpha,
			.temporalDepthTolerance = m_parameters.temporalDepthTolerance,
			.temporalNormalThreshold = m_parameters.temporalNormalThreshold,
			.temporalVarianceGamma = m_parameters.temporalVarianceGamma,
			.maxAccumulation = m_parameters.denoiserMaxAccumulation,
			.temporalNeighborhoodClamp = m_parameters.temporalNeighborhoodClamp,
			.temporalAnimatedNoise = m_parameters.temporalAnimatedNoise,
			.accumulationCounter = m_parameters.denoiserAccumulationCounter
		});

		if ( !m_denoiser.create(halfW, halfH) )
		{
			TraceError{ClassId} << "Failed to create the RTGI denoiser component !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals + GI history samplers, plus the frame UBO
		 * (the per-frame data outgrew the 128-byte push constant minimum). The receiver
		 * albedo is NOT read here anymore — the trace outputs demodulated irradiance and
		 * the combine pass re-applies the albedo at full resolution. */
		auto traceInputLayout = this->getInputLayout(3, 1);

		if ( traceInputLayout == nullptr )
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

		if ( m_traceLayout == nullptr )
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

		/* ---- Create descriptor sets ---- */

		/* Trace: set 1 reads depth + normals + history (updated per-frame),
		 * plus the denoiser's frame UBO (written once here, rewritten CPU-side every frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		for ( size_t f = 0; f < m_tracePerFrame.size(); ++f )
		{
			if ( !m_tracePerFrame[f]->writeUniformBufferObject(3, m_denoiser.frameUBO(static_cast< uint32_t >(f))) )
			{
				return false;
			}

			/* The history binding must always hold a VALID descriptor (the shader statically
			 * uses it even when the feedback is disabled at runtime). When the temporal chain
			 * is off, bind the trace target as an inert placeholder (strength is 0). */
			if ( !m_denoiser.temporalActive() )
			{
				if ( !m_tracePerFrame[f]->writeCombinedImageSampler(2, m_traceTarget) )
				{
					return false;
				}
			}
		}

		/* Combine source default: the raw trace. recordOverlayPasses() retargets it to the
		 * denoiser output every frame when the temporal chain is active. */
		m_combineSource = &m_traceTarget;

		return true;
	}

	void
	RTGI::destroy () noexcept
	{
		m_combineSource = nullptr;

		m_tracePerFrame.clear();

		m_tracePipeline.reset();
		m_traceLayout.reset();

		m_denoiser.destroy();

		m_traceTarget.destroy();
	}

	void
	RTGI::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;

		const auto frameIndex = this->renderer().currentFrameIndex();

		const bool temporalActive = m_denoiser.temporalActive();

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
			/* History ping-pong: this frame reads the denoiser's read texture. The flip only
			 * happens inside GIDenoiser::recordResolve(), so the binding is stable here. */
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, m_denoiser.historyReadTexture()));
		}

		/* ---- Frame UBO (assembled by the denoiser: matrices + temporal parameters;
		 * RTGI only supplies its trace scalars) ----
		 * THE SKY IS A LIGHT SOURCE. The luminance comes from the scene background
		 * (0 = no background, no sky light). The sky distance is how far a ray must
		 * travel before "hit nothing" may be read as "sees the sky": the far plane is
		 * the frame's own "nothing beyond this exists" bound, so distant geometry can
		 * never be mistaken for open sky. */
		static_cast< void >(m_denoiser.updateFrameData(frameIndex, context, GIDenoiser::FrameInputs{
			.traceMaxDistance = m_parameters.maxDistance,
			.traceBias = m_parameters.bias,
			.traceSampleCount = static_cast< float >(m_parameters.sampleCount),
			.bounceStrength = m_parameters.multiBounceEnabled ? m_parameters.multiBounceStrength : 0.0F,
			.bounceClamp = m_parameters.multiBounceClamp,
			.skyLuminance = context.skyLuminance,
			.skyDistance = context.constants.farPlane
		}));

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

		/* ---- Denoise chain (SVGF order): temporal resolve on the RAW trace + moments
		 * accumulation + normal history, then the variance-guided à-trous iterations.
		 * The combine consumes the denoiser output (the raw trace when the temporal
		 * chain is off — diagnostic mode, no spatial filter without its variance guide). */
		m_combineSource = m_denoiser.recordResolve(commandBuffer, m_traceTarget, context);
	}

	bool
	RTGI::providesIndirectDiffuse () const noexcept
	{
		const auto & renderer = this->renderer();

		/* ⚠️ The SAME gate the post-process executor skips this effect on (PostProcessor.cpp):
		 * hardware, user setting, and a TLAS that can actually be consumed this frame. Claiming
		 * the indirect diffuse while the trace cannot run would leave the frame with NO diffuse
		 * sky at all — the scene hands its own IBL leg over to an effect that draws nothing. */
		return renderer.device()->rayTracingEnabled() && renderer.isRayTracingSettingEnabled() && renderer.isRayTracingReady();
	}

	IndirectPostProcessEffect::CombineContribution
	RTGI::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		/* Denoiser debug views (diagnostic): draw the denoiser internals INSTEAD of the GI
		 * contribution. */
		if ( m_parameters.denoiserDebugView != 0U && m_denoiser.temporalActive() )
		{
			return m_denoiser.debugCombineContribution("rtgi", m_parameters.denoiserDebugView);
		}

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
