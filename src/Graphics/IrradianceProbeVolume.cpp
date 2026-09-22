/*
 * src/Graphics/IrradianceProbeVolume.cpp
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

#include "IrradianceProbeVolume.hpp"

/* The shared GLSL rules this volume's own passes splice. */
#include "Graphics/Effects/Shared/IrradianceProbesGLSL.hpp"
#include "Graphics/Effects/Shared/RTAlphaTestGLSL.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/Sync/MemoryBarrier.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace
{
	using namespace EmEn;

	/* ---- Pass 1: TRACE. One invocation per (probe, ray). Set 0 = the Renderer's RT set (TLAS,
	 * mesh/material/light SSBOs), set 1 = bindless textures, set 2 = the volume's own set (ray
	 * buffer written here, atlases READ for the recursive bounce, parameters).
	 * A hit is shaded the way RTGI shades its bounce hits — Lambert under the scene's direct
	 * lights with a shadow ray per light that casts shadows in the raster, the raster's scalar
	 * ambient, the emission — PLUS the volume's own irradiance at the hit, which is what turns
	 * the single traced bounce into the converged multi-bounce solution over the frames (the
	 * DDGI feedback). A miss is the sky. A back-face hit (the probe sits inside or behind that
	 * geometry along this ray) carries no light and a SHORTENED distance, DDGI's 0.2 factor, so
	 * the Chebyshev test discounts the probe for points on the far side of that wall. */
	constexpr auto ProbeTraceComputeShaderHead = R"GLSL(
#version 460
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(local_size_x = 64) in;

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

/* Light SSBO (set 0, binding 3): 4 vec4 per light — see RTR.cpp for the layout. */
layout(set = 0, binding = 3) readonly buffer LightData { vec4 lights[]; } lightSSBO;
)GLSL" EMEN_RT_SCENE_DATA_GLSL(1) R"GLSL(
/* Bindless cube array (set 1, binding 3): slot 0 is the ACTIVE scene's environment cubemap. */
layout(set = 1, binding = 3) uniform samplerCube texturesCube[];

/* The ray buffer this pass writes: (radiance.rgb, hit distance) per (probe, ray). Negative
 * distance = back-face hit. */
layout(set = 2, binding = 0, std430) writeonly buffer ProbeRayData { vec4 rays[]; } probeRays;
)GLSL" EMEN_IRRADIANCE_PROBES_GLSL(2) R"GLSL(
/* Material flag bits and channel packing (must match GPURTMaterialData; the alpha-test rule
 * already declares HasAlbedoTexture, HasOpacityTexture and IsAlphaTest). */
const uint HasMetalnessTexture = 1u << 3;
const uint HasEmissionTexture = 1u << 4;
const uint IsEmissive = 1u << 6;
const uint MetalnessChannelShift = 18u;
const uint ChannelMask = 3u;

const uint EnvironmentCubemapSlot = 0u;
const float PI = 3.14159265359;
const float ShadowRayBias = 0.01;
const float SkyDistance = 10000.0;

vec3 readVertexVec3 (VertexBuffer vb, uint vertexIndex, uint strideFloats, uint attrOffsetFloats)
{
	uint base = vertexIndex * strideFloats + attrOffsetFloats;

	return vec3(vb.v[base], vb.v[base + 1u], vb.v[base + 2u]);
}

/* GPUMeshMetaData: meta1.z = normal byte offset (see RTR.cpp). */
vec3 getHitNormal (MeshAccessor m, uint instanceIndex, vec2 bary)
{
	uint normalOffsetFloats = meshSSBO.meshEntries[instanceIndex * 3u + 1u].z / 4u;
	vec3 n0 = readVertexVec3(m.vb, m.idx0, m.strideFloats, normalOffsetFloats);
	vec3 n1 = readVertexVec3(m.vb, m.idx1, m.strideFloats, normalOffsetFloats);
	vec3 n2 = readVertexVec3(m.vb, m.idx2, m.strideFloats, normalOffsetFloats);

	/* ⚠️ vec3(0.0) — never a NaN — on a degenerate interpolation: that NaN became the origin of
	 * the shadow rays below, and a NaN ray query is a DEVICE LOSS on NVIDIA (same rule as RTGI/RTR). */
	vec3 n = n0 * (1.0 - bary.x - bary.y) + n1 * bary.x + n2 * bary.y;
	float lengthSquared = dot(n, n);

	return lengthSquared > 1e-12 ? n * inversesqrt(lengthSquared) : vec3(0.0);
}
)GLSL" EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(
/* Shadow ray: 1.0 when the path toward the light is free. Candidates are judged by the shared
 * alpha-test rule, never gl_RayFlagsOpaqueEXT (a leaf would block the light as a solid quad). */
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
	rayQueryInitializeEXT(shadowQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, origin, 0.0, direction, maxT);

	while (rayQueryProceedEXT(shadowQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(shadowQuery) R"GLSL(
	}

	return rayQueryGetIntersectionTypeEXT(shadowQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

/* Direct IRRADIANCE at a hit (no albedo): the RTGI bounce shading, light for light. Only the
 * lights that cast shadows in the raster get a shadow ray — the others shine through geometry on
 * screen and the cached light must match the image. */
vec3 computeDirectIrradiance (vec3 hitPos, vec3 hitNormal)
{
	uint lightCount = uint(probeVolume.skyAmbient.z);
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
		float shadowDistance = 10000.0;

		if (type < 0.5)
		{
			L = normalize(-dirType.xyz);
		}
		else
		{
			vec3 toLight = posRadius.xyz - hitPos;
			float dist = length(toLight);
			L = toLight / max(dist, 0.0001);
			shadowDistance = dist;

			/* The RASTER curve, verbatim: `max(1 - dot(d/r, d/r), 0)` (LightGenerator.PBR.cpp),
			 * and no attenuation at all without a radius. The cache feeds RTGI and RTR, so it
			 * must integrate the same lights they do. Same fix in both, 2026-09-13. */
			float radius = posRadius.w;

			if (radius > 0.0)
			{
				float distanceRatio = dist / radius;
				attenuation = max(1.0 - distanceRatio * distanceRatio, 0.0);
			}

			if (type > 1.5)
			{
				vec4 spotParams = lightSSBO.lights[base + 3u];
				float cosAngle = dot(-L, normalize(dirType.xyz));
				attenuation *= clamp((cosAngle - spotParams.y) / max(spotParams.x - spotParams.y, 0.0001), 0.0, 1.0);
			}
		}

		float NdotL = max(dot(hitNormal, L), 0.0);

		if (NdotL * attenuation <= 0.0)
		{
			continue;
		}

		float visibility = 1.0;

		if (lightSSBO.lights[base + 3u].z > 0.5)
		{
			visibility = shadowRayVisibility(hitPos + hitNormal * ShadowRayBias, L, shadowDistance);
		}

		totalLight += lightColor * NdotL * attenuation * visibility;
	}

	return totalLight;
}

/* The sky is a light source: a normalized cubemap direction becomes nits through the sky luminance
 * (0 = the scene has no sky). Same slot and same raw Y-up sampling as RTGI. */
vec3 skyRadiance (vec3 direction)
{
	if (probeVolume.skyAmbient.x <= 0.0)
	{
		return vec3(0.0);
	}

	/* Explicit LOD: a compute shader has no derivatives to pick one. */
	return textureLod(texturesCube[nonuniformEXT(EnvironmentCubemapSlot)], direction, 0.0).rgb * probeVolume.skyAmbient.x;
}

void main ()
{
	uint raysPerProbe = uint(probeVolume.probeCounts.w);
	uint probeTotal = uint(probeVolume.probeCounts.x * probeVolume.probeCounts.y * probeVolume.probeCounts.z);
	uint globalIndex = gl_GlobalInvocationID.x;
	uint probeLinear = globalIndex / raysPerProbe;
	uint rayIndex = globalIndex - probeLinear * raysPerProbe;

	if (probeLinear >= probeTotal)
	{
		return;
	}

	ivec3 slot = probeSlotFromLinear(probeLinear);
	vec3 origin = probeWorldPosition(probeGridIndex(slot));
	vec3 direction = probeRayDirection(rayIndex);

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsNoneEXT, 0xFF, origin, 0.0, direction, SkyDistance);

	while (rayQueryProceedEXT(rayQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(rayQuery) R"GLSL(
	}

	vec4 result;

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		result = vec4(skyRadiance(direction), SkyDistance);
	}
	else
	{
		float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
		uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
		uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
		uint geomIdx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
		vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

		MeshAccessor mesh = getMeshAccessor(instanceIndex, geomIdx, primitiveIndex);
		mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
		vec3 worldNormal = mat3(objectToWorld) * getHitNormal(mesh, instanceIndex, barycentrics);
		float worldNormalLengthSquared = dot(worldNormal, worldNormal);
		/* A degenerate normal faces the ray: finite, so the shadow rays stay legal. */
		vec3 hitNormal = worldNormalLengthSquared > 1e-12 ? worldNormal * inversesqrt(worldNormalLengthSquared) : -direction;

		if (dot(hitNormal, direction) > 0.0)
		{
			/* Back face: no light, shortened distance (DDGI). */
			result = vec4(0.0, 0.0, 0.0, -0.2 * hitT);
		}
		else
		{
			uint matBase = rtHitMaterialIndex(instanceIndex, geomIdx) * 7u;
			uint flags = floatBitsToUint(materialSSBO.materials[matBase + 4u].w);
			vec2 hitUV = getHitUV(mesh, barycentrics);

			vec3 albedo = materialSSBO.materials[matBase].rgb;

			if ((flags & HasAlbedoTexture) != 0u)
			{
				int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].x);

				if (texIndex >= 0)
				{
					albedo = textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, 0.0).rgb;
				}
			}

			float metalness = materialSSBO.materials[matBase + 1u].y;

			if ((flags & HasMetalnessTexture) != 0u)
			{
				int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 5u].w);

				if (texIndex >= 0)
				{
					metalness *= textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, 0.0)[(flags >> MetalnessChannelShift) & ChannelMask];
				}
			}

			/* A bounce is a DIFFUSE event: a metal has no diffuse lobe (same rule as RTGI). */
			vec3 diffuseAlbedo = albedo * (1.0 - clamp(metalness, 0.0, 1.0));
			vec3 hitPos = origin + direction * hitT;

			vec3 irradiance = computeDirectIrradiance(hitPos, hitNormal);
			/* skyAmbient.w = bounce feedback weight (0 = single bounce, 1 = the full DDGI recursion). */
			vec3 radiance = diffuseAlbedo * (irradiance / PI + probeVolume.ambientColor.rgb + probeIrradiance(hitPos, hitNormal, -direction) * probeVolume.skyAmbient.w);

			if ((flags & IsEmissive) != 0u)
			{
				vec3 emission = materialSSBO.materials[matBase + 3u].rgb * materialSSBO.materials[matBase + 4u].x;

				if ((flags & HasEmissionTexture) != 0u)
				{
					int texIndex = floatBitsToInt(materialSSBO.materials[matBase + 6u].x);

					if (texIndex >= 0)
					{
						emission *= textureLod(textures2D[nonuniformEXT(texIndex)], hitUV, 0.0).rgb;
					}
				}

				radiance += emission;
			}

			result = vec4(radiance, hitT);
		}
	}

	probeRays.rays[globalIndex] = result;
}
)GLSL";

	/* ---- Pass 2: BLEND IRRADIANCE. One workgroup per probe, one invocation per interior texel of
	 * its octahedral tile. The ray set is staged in shared memory once per probe. Each texel is the
	 * cosine-weighted mean of the rays' radiance around its direction — over a full sphere of
	 * uniformly spread directions that mean IS E/π, the demodulated convention of the RTGI history
	 * — blended into the previous value with the hysteresis, or taken whole when the probe was
	 * reset (first frame, or its storage slot just wrapped around the scrolling volume). */
	constexpr auto ProbeBlendIrradianceComputeShader = R"GLSL(
#version 460

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0, std430) readonly buffer ProbeRayData { vec4 rays[]; } probeRays;
layout(set = 0, binding = 1, rgba16f) uniform image2DArray irradianceAtlas;
)GLSL" EMEN_IRRADIANCE_PROBES_GLSL(0) R"GLSL(
const uint MaxRays = 512u;

shared vec4 s_rays[MaxRays];
shared vec3 s_directions[MaxRays];

void main ()
{
	uint probeLinear = gl_WorkGroupID.x;
	uint raysPerProbe = uint(probeVolume.probeCounts.w);
	uint rayBase = probeLinear * raysPerProbe;

	for (uint r = gl_LocalInvocationIndex; r < raysPerProbe; r += 64u)
	{
		s_rays[r] = probeRays.rays[rayBase + r];
		s_directions[r] = probeRayDirection(r);
	}

	barrier();

	ivec3 slot = probeSlotFromLinear(probeLinear);
	ivec2 texel = ivec2(gl_LocalInvocationID.xy);
	vec3 texelDirection = probeOctDecode(((vec2(texel) + 0.5) / float(ProbeIrradianceTexels)) * 2.0 - 1.0);

	vec3 sum = vec3(0.0);
	float weightSum = 0.0;

	for (uint r = 0u; r < raysPerProbe; ++r)
	{
		vec4 ray = s_rays[r];

		/* A back-face hit carries no light. */
		if (ray.w < 0.0)
		{
			continue;
		}

		float weight = max(0.0, dot(texelDirection, s_directions[r]));

		sum += ray.rgb * weight;
		weightSum += weight;
	}

	vec3 result = weightSum > 1e-4 ? sum / weightSum : vec3(0.0);

	ivec3 coordinate = ivec3(probeTileOrigin(slot, ProbeIrradianceTexels) + texel, slot.y);
	vec3 previous = imageLoad(irradianceAtlas, coordinate).rgb;
	vec3 blended = probeIsReset(slot) ? result : mix(result, previous, probeVolume.biasHysteresis.z);

	imageStore(irradianceAtlas, coordinate, vec4(blended, 1.0));
}
)GLSL";

	/* ---- Pass 3: BLEND DISTANCE. Same shape, 16x16 texels, a sharp power-50 lobe: each texel stores
	 * the mean distance and the mean squared distance toward the geometry in its direction, the two
	 * moments the Chebyshev visibility test consumes. Distances are clamped to the stored maximum
	 * (a neighbouring probe's reach — beyond it the test has nothing to decide). Back-face hits
	 * enter with their shortened magnitude. */
	constexpr auto ProbeBlendDistanceComputeShader = R"GLSL(
#version 460

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, std430) readonly buffer ProbeRayData { vec4 rays[]; } probeRays;
layout(set = 0, binding = 2, rg16f) uniform image2DArray distanceAtlas;
)GLSL" EMEN_IRRADIANCE_PROBES_GLSL(0) R"GLSL(
const uint MaxRays = 512u;
const float DistanceSharpness = 50.0;

shared float s_distances[MaxRays];
shared vec3 s_directions[MaxRays];

void main ()
{
	uint probeLinear = gl_WorkGroupID.x;
	uint raysPerProbe = uint(probeVolume.probeCounts.w);
	uint rayBase = probeLinear * raysPerProbe;
	float maxDistance = probeVolume.biasHysteresis.w;

	for (uint r = gl_LocalInvocationIndex; r < raysPerProbe; r += 256u)
	{
		s_distances[r] = min(abs(probeRays.rays[rayBase + r].w), maxDistance);
		s_directions[r] = probeRayDirection(r);
	}

	barrier();

	ivec3 slot = probeSlotFromLinear(probeLinear);
	ivec2 texel = ivec2(gl_LocalInvocationID.xy);
	vec3 texelDirection = probeOctDecode(((vec2(texel) + 0.5) / float(ProbeDistanceTexels)) * 2.0 - 1.0);

	vec2 sum = vec2(0.0);
	float weightSum = 0.0;

	for (uint r = 0u; r < raysPerProbe; ++r)
	{
		float weight = pow(max(0.0, dot(texelDirection, s_directions[r])), DistanceSharpness);
		float dist = s_distances[r];

		sum += vec2(dist, dist * dist) * weight;
		weightSum += weight;
	}

	vec2 result = weightSum > 1e-6 ? sum / weightSum : vec2(maxDistance, maxDistance * maxDistance);

	ivec3 coordinate = ivec3(probeTileOrigin(slot, ProbeDistanceTexels) + texel, slot.y);
	vec2 previous = imageLoad(distanceAtlas, coordinate).rg;
	vec2 blended = probeIsReset(slot) ? result : mix(result, previous, probeVolume.biasHysteresis.z);

	imageStore(distanceAtlas, coordinate, vec4(blended, 0.0, 0.0));
}
)GLSL";

	/* ---- Pass 4: BORDERS. The 1-texel border of every tile is a copy of the interior texel the
	 * octahedral wrap lands on, so the consumers' hardware bilinear filtering stays continuous
	 * across the tile's edges. Corners take the opposite corner; an edge texel takes the mirrored
	 * texel of the first interior row/column on its side. Reads are interior, writes are border:
	 * no invocation reads what another writes. One shader body serves both atlases. */
	std::string
	probeBorderComputeShader (uint32_t texels, const char * imageDeclaration) noexcept
	{
		std::string source{"#version 460\n\nlayout(local_size_x = 8, local_size_y = 8) in;\n\n"};
		source += imageDeclaration;
		source += EMEN_IRRADIANCE_PROBES_GLSL(0);
		source += "\nconst int Texels = " + std::to_string(texels) + ";\n";
		source += R"GLSL(
void main ()
{
	ivec3 slot = probeSlotFromLinear(gl_WorkGroupID.x);
	int full = Texels + 2;
	ivec2 tile = probeTileOrigin(slot, Texels) - 1;
	uint total = uint(full * full);

	for (uint i = gl_LocalInvocationIndex; i < total; i += 64u)
	{
		ivec2 dst = ivec2(int(i) % full, int(i) / full);
		bool xEdge = dst.x == 0 || dst.x == full - 1;
		bool yEdge = dst.y == 0 || dst.y == full - 1;

		if (!xEdge && !yEdge)
		{
			continue;
		}

		ivec2 src;

		if (xEdge && yEdge)
		{
			src = ivec2(dst.x == 0 ? Texels : 1, dst.y == 0 ? Texels : 1);
		}
		else if (yEdge)
		{
			src = ivec2(full - 1 - dst.x, dst.y == 0 ? 1 : Texels);
		}
		else
		{
			src = ivec2(dst.x == 0 ? 1 : Texels, full - 1 - dst.y);
		}

		imageStore(atlas, ivec3(tile + dst, slot.y), imageLoad(atlas, ivec3(tile + src, slot.y)));
	}
}
)GLSL";

		return source;
	}

	constexpr auto IrradianceBorderImageDeclaration = "layout(set = 0, binding = 1, rgba16f) uniform image2DArray atlas;\n";
	constexpr auto DistanceBorderImageDeclaration = "layout(set = 0, binding = 2, rg16f) uniform image2DArray atlas;\n";

	/* Descriptor set bindings — the consumer-facing ones (3-5) are what EMEN_IRRADIANCE_PROBES_GLSL declares. */
	constexpr uint32_t RayBufferBinding = 0;
	constexpr uint32_t IrradianceStorageBinding = 1;
	constexpr uint32_t DistanceStorageBinding = 2;
	constexpr uint32_t ParametersBinding = 3;
	constexpr uint32_t IrradianceSamplerBinding = 4;
	constexpr uint32_t DistanceSamplerBinding = 5;

	constexpr uint32_t TraceWorkgroupSize = 64;

	/* The stored distance ceiling: 1.5 x the diagonal of a probe cell. Beyond a neighbouring probe's
	 * reach the visibility test has nothing to decide (DDGI / RTXGI convention). */
	constexpr float MaxRayDistanceCellDiagonals = 1.5F;

	[[nodiscard]]
	int32_t
	positiveModulo (int32_t value, int32_t modulus) noexcept
	{
		return ((value % modulus) + modulus) % modulus;
	}
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	IrradianceProbeVolume::IrradianceProbeVolume () noexcept = default;

	IrradianceProbeVolume::~IrradianceProbeVolume () noexcept
	{
		this->destroy();
	}

	void
	IrradianceProbeVolume::readSettings (Renderer & renderer) noexcept
	{
		auto & settings = renderer.primaryServices().settings();

		m_parameters.enabled = settings.getOrSetDefault< bool >(GraphicsRayTracingIrradianceProbesEnabledKey, DefaultGraphicsRayTracingIrradianceProbesEnabled);
		m_parameters.probeCountX = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingIrradianceProbesCountXKey, DefaultGraphicsRayTracingIrradianceProbesCountX);
		m_parameters.probeCountY = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingIrradianceProbesCountYKey, DefaultGraphicsRayTracingIrradianceProbesCountY);
		m_parameters.probeCountZ = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingIrradianceProbesCountZKey, DefaultGraphicsRayTracingIrradianceProbesCountZ);
		m_parameters.probeSpacing = settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesSpacingKey, DefaultGraphicsRayTracingIrradianceProbesSpacing);
		m_parameters.cameraHeightFraction = std::clamp(settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesCameraHeightFractionKey, DefaultGraphicsRayTracingIrradianceProbesCameraHeightFraction), 0.0F, 1.0F);
		m_parameters.raysPerProbe = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingIrradianceProbesRaysPerProbeKey, DefaultGraphicsRayTracingIrradianceProbesRaysPerProbe);
		m_parameters.hysteresis = settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesHysteresisKey, DefaultGraphicsRayTracingIrradianceProbesHysteresis);
		m_parameters.bounceFeedback = std::clamp(settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesBounceFeedbackKey, DefaultGraphicsRayTracingIrradianceProbesBounceFeedback), 0.0F, 1.0F);
		m_parameters.normalBias = settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesNormalBiasKey, DefaultGraphicsRayTracingIrradianceProbesNormalBias);
		m_parameters.viewBias = settings.getOrSetDefault< float >(GraphicsRayTracingIrradianceProbesViewBiasKey, DefaultGraphicsRayTracingIrradianceProbesViewBias);
		/* Not a key of this volume: the artistic multiplier of the IndirectDiffuse concept, so the
		 * reflected indirect follows the primary one. Read here, applied by the consumers' query. */
		m_parameters.indirectIntensity = settings.getOrSetDefault< float >(GraphicsPPIndirectDiffuseIntensityKey, DefaultGraphicsPPIndirectDiffuseIntensity);

		/* The shaders assume: at least 2 probes per axis (trilinear), a ray set that fits the
		 * shared-memory staging of the blend passes, a spacing and a hysteresis that keep the
		 * arithmetic finite. A value outside is clamped and said. */
		const auto clampCount = [] (uint32_t & count, const char * axis) {
			if ( count < 2 )
			{
				TraceWarning{ClassId} << "The probe count along " << axis << " must be at least 2, clamped.";

				count = 2;
			}
		};

		clampCount(m_parameters.probeCountX, "X");
		clampCount(m_parameters.probeCountY, "Y");
		clampCount(m_parameters.probeCountZ, "Z");

		if ( m_parameters.raysPerProbe < 8 || m_parameters.raysPerProbe > MaxRaysPerProbe )
		{
			TraceWarning{ClassId} << "The rays per probe must stay within [8, " << MaxRaysPerProbe << "], clamped.";

			m_parameters.raysPerProbe = std::clamp(m_parameters.raysPerProbe, 8U, MaxRaysPerProbe);
		}

		if ( m_parameters.probeSpacing <= 0.01F )
		{
			TraceWarning{ClassId} << "The probe spacing must be positive, reset to the default.";

			m_parameters.probeSpacing = DefaultGraphicsRayTracingIrradianceProbesSpacing;
		}

		m_parameters.hysteresis = std::clamp(m_parameters.hysteresis, 0.0F, 0.999F);
	}

	bool
	IrradianceProbeVolume::initialize (Renderer & renderer) noexcept
	{
		m_device = renderer.device();

		this->readSettings(renderer);

		if ( !this->createResources(renderer) || !this->createDescriptorSets(renderer) || !this->createPipelines(renderer) )
		{
			this->destroy();

			return false;
		}

		TraceInfo{ClassId} <<
			"Irradiance probe volume ready: " << m_parameters.probeCountX << " x " << m_parameters.probeCountY << " x " << m_parameters.probeCountZ <<
			" probes, " << m_parameters.probeSpacing << " m apart, " << m_parameters.raysPerProbe << " rays per probe (" <<
			(this->probeCount() * m_parameters.raysPerProbe) << " rays per frame), " << ( m_parameters.enabled ? "enabled" : "DISABLED by settings" ) << ".";

		return true;
	}

	void
	IrradianceProbeVolume::destroy () noexcept
	{
		m_borderDistancePipeline.reset();
		m_borderIrradiancePipeline.reset();
		m_blendDistancePipeline.reset();
		m_blendIrradiancePipeline.reset();
		m_tracePipeline.reset();
		m_updatePipelineLayout.reset();
		m_tracePipelineLayout.reset();
		m_descriptorSets.clear();
		m_descriptorSetLayout.reset();
		m_descriptorPool.reset();
		m_parameterBuffers.clear();
		m_rayBuffer.reset();
		m_sampler.reset();
		m_distanceView.reset();
		m_distanceImage.reset();
		m_irradianceView.reset();
		m_irradianceImage.reset();
		m_device.reset();
		m_atlasesInitialized = false;
		m_hasCameraCell = false;
	}

	bool
	IrradianceProbeVolume::createResources (Renderer & renderer) noexcept
	{
		const auto createAtlas = [this] (uint32_t texels, VkFormat format, const char * name, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & view) {
			const uint32_t tile = texels + 2;
			const VkExtent3D extent{m_parameters.probeCountX * tile, m_parameters.probeCountZ * tile, 1};

			image = std::make_shared< Image >(m_device, VK_IMAGE_TYPE_2D, format, extent, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, 1, m_parameters.probeCountY);
			image->setIdentifier(ClassId, name, "Image");

			if ( !image->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the " << name << " atlas !";

				return false;
			}

			view = std::make_shared< ImageView >(image, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, m_parameters.probeCountY});
			view->setIdentifier(ClassId, name, "ImageView");

			if ( !view->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the " << name << " atlas view !";

				return false;
			}

			return true;
		};

		if ( !createAtlas(IrradianceTexels, VK_FORMAT_R16G16B16A16_SFLOAT, "IrradianceAtlas", m_irradianceImage, m_irradianceView) )
		{
			return false;
		}

		if ( !createAtlas(DistanceTexels, VK_FORMAT_R16G16_SFLOAT, "DistanceAtlas", m_distanceImage, m_distanceView) )
		{
			return false;
		}

		/* Linear, clamped: the tile borders carry the octahedral wrap, the clamp only guards the
		 * atlas edges. No mip chain. */
		m_sampler = renderer.getSampler("IrradianceProbeAtlas", [] (Settings & /*settings*/, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 0.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the atlas sampler !";

			return false;
		}

		/* The ray buffer: (radiance, distance) per (probe, ray), device-local, GPU written and read. */
		const VkDeviceSize rayBufferBytes = static_cast< VkDeviceSize >(this->probeCount()) * m_parameters.raysPerProbe * sizeof(float) * 4;

		m_rayBuffer = std::make_unique< ShaderStorageBufferObject >(m_device, rayBufferBytes, false);
		m_rayBuffer->setIdentifier(ClassId, "RayBuffer", "ShaderStorageBufferObject");

		if ( !m_rayBuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the ray buffer !";

			return false;
		}

		/* One parameters block per frame in flight: the scroll and the ray rotation change every frame. */
		const auto frameCount = renderer.framesInFlight();

		m_parameterBuffers.reserve(frameCount);

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex )
		{
			auto buffer = std::make_unique< UniformBufferObject >(m_device, sizeof(ParametersUBO));
			buffer->setIdentifier(ClassId, "Parameters-F" + std::to_string(frameIndex), "UniformBufferObject");

			if ( !buffer->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the parameters buffer of frame " << frameIndex << " !";

				return false;
			}

			m_parameterBuffers.emplace_back(std::move(buffer));
		}

		return true;
	}

	bool
	IrradianceProbeVolume::createDescriptorSets (Renderer & renderer) noexcept
	{
		constexpr VkShaderStageFlags consumerStages = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		m_descriptorSetLayout = std::make_shared< DescriptorSetLayout >(m_device, "IrradianceProbeVolume");
		m_descriptorSetLayout->setIdentifier(ClassId, "Main", "DescriptorSetLayout");

		if ( !m_descriptorSetLayout->declareStorageBuffer(RayBufferBinding, VK_SHADER_STAGE_COMPUTE_BIT) ||
			!m_descriptorSetLayout->declareStorageImage(IrradianceStorageBinding, VK_SHADER_STAGE_COMPUTE_BIT) ||
			!m_descriptorSetLayout->declareStorageImage(DistanceStorageBinding, VK_SHADER_STAGE_COMPUTE_BIT) ||
			!m_descriptorSetLayout->declareUniformBuffer(ParametersBinding, consumerStages) ||
			!m_descriptorSetLayout->declareCombinedImageSampler(IrradianceSamplerBinding, consumerStages) ||
			!m_descriptorSetLayout->declareCombinedImageSampler(DistanceSamplerBinding, consumerStages) )
		{
			TraceError{ClassId} << "Unable to declare the descriptor set layout bindings !";

			return false;
		}

		if ( !m_descriptorSetLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout !";

			return false;
		}

		/* Own pool: the renderer's main pool declares no storage image capacity. ⚠️ FREE_DESCRIPTOR_SET_BIT
		 * is mandatory, Vulkan::DescriptorSet frees its set individually (see src/Vulkan/AGENTS.md). */
		const auto frameCount = renderer.framesInFlight();

		const std::vector< VkDescriptorPoolSize > poolSizes{
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = frameCount},
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 2 * frameCount},
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = frameCount},
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 2 * frameCount}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(m_device, poolSizes, frameCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
		m_descriptorPool->setIdentifier(ClassId, "Main", "DescriptorPool");

		if ( !m_descriptorPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor pool !";

			return false;
		}

		VkDescriptorBufferInfo rayBufferInfo{};
		rayBufferInfo.buffer = m_rayBuffer->handle();
		rayBufferInfo.offset = 0;
		rayBufferInfo.range = m_rayBuffer->bytes();

		m_descriptorSets.reserve(frameCount);

		for ( uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, "Main-F" + std::to_string(frameIndex), "DescriptorSet");

			if ( !descriptorSet->create() )
			{
				TraceError{ClassId} << "Unable to allocate the descriptor set of frame " << frameIndex << " !";

				return false;
			}

			/* The atlases live in GENERAL for their whole life: the explicit-layout overloads, not the
			 * one reading the image's current layout (UNDEFINED at creation — measured as
			 * VUID-VkWriteDescriptorSet-descriptorType-04150 on every set). */
			if ( !descriptorSet->writeStorageBuffer(RayBufferBinding, rayBufferInfo) ||
				!descriptorSet->writeStorageImage(IrradianceStorageBinding, *m_irradianceView) ||
				!descriptorSet->writeStorageImage(DistanceStorageBinding, *m_distanceView) ||
				!descriptorSet->writeUniformBufferObject(ParametersBinding, *m_parameterBuffers[frameIndex]) ||
				!descriptorSet->writeCombinedImageSampler(IrradianceSamplerBinding, *m_irradianceView, *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeCombinedImageSampler(DistanceSamplerBinding, *m_distanceView, *m_sampler, VK_IMAGE_LAYOUT_GENERAL) )
			{
				TraceError{ClassId} << "Unable to write the descriptor set of frame " << frameIndex << " !";

				return false;
			}

			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		return true;
	}

	bool
	IrradianceProbeVolume::createPipelines (Renderer & renderer) noexcept
	{
		const auto rtLayout = renderer.rtDescriptorSetLayout();
		const auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( rtLayout == nullptr || bindlessLayout == nullptr )
		{
			TraceError{ClassId} << "The RT and bindless descriptor set layouts are required !";

			return false;
		}

		/* Trace: set 0 = RT data, set 1 = bindless textures, set 2 = the volume. */
		m_tracePipelineLayout = std::make_shared< PipelineLayout >(
			m_device, "IrradianceProbeTracePipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{rtLayout, bindlessLayout, m_descriptorSetLayout},
			StaticVector< VkPushConstantRange, 4 >{}
		);

		if ( !m_tracePipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the trace pipeline layout !";

			return false;
		}

		/* Blend and borders: set 0 = the volume. */
		m_updatePipelineLayout = std::make_shared< PipelineLayout >(
			m_device, "IrradianceProbeUpdatePipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_descriptorSetLayout},
			StaticVector< VkPushConstantRange, 4 >{}
		);

		if ( !m_updatePipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the update pipeline layout !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();

		const auto buildPipeline = [&] (std::unique_ptr< ComputePipeline > & pipeline, const std::shared_ptr< PipelineLayout > & layout, const char * name, const std::string & source) {
			const auto shaderModule = shaderManager.getShaderModuleFromSourceCode(m_device, name, ShaderType::ComputeShader, source);

			if ( shaderModule == nullptr )
			{
				TraceError{ClassId} << "Failed to compile the compute shader '" << name << "' !";

				return false;
			}

			pipeline = std::make_unique< ComputePipeline >(layout);
			pipeline->setShaderModule(shaderModule->handle());

			if ( !pipeline->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the compute pipeline '" << name << "' !";

				pipeline.reset();

				return false;
			}

			return true;
		};

		return
			buildPipeline(m_blendIrradiancePipeline, m_updatePipelineLayout, "IrradianceProbe_BlendIrradiance_CS", ProbeBlendIrradianceComputeShader) &&
			buildPipeline(m_blendDistancePipeline, m_updatePipelineLayout, "IrradianceProbe_BlendDistance_CS", ProbeBlendDistanceComputeShader) &&
			buildPipeline(m_borderIrradiancePipeline, m_updatePipelineLayout, "IrradianceProbe_BorderIrradiance_CS", probeBorderComputeShader(IrradianceTexels, IrradianceBorderImageDeclaration)) &&
			buildPipeline(m_borderDistancePipeline, m_updatePipelineLayout, "IrradianceProbe_BorderDistance_CS", probeBorderComputeShader(DistanceTexels, DistanceBorderImageDeclaration)) &&
			buildPipeline(m_tracePipeline, m_tracePipelineLayout, "IrradianceProbe_Trace_CS", ProbeTraceComputeShaderHead);
	}

	const DescriptorSet *
	IrradianceProbeVolume::descriptorSet (uint32_t frameIndex) const noexcept
	{
		if ( frameIndex >= m_descriptorSets.size() )
		{
			return nullptr;
		}

		return m_descriptorSets[frameIndex].get();
	}

	void
	IrradianceProbeVolume::updateParameters (uint32_t frameIndex, const FrameInputs & inputs) noexcept
	{
		const std::array< int32_t, 3 > counts{
			static_cast< int32_t >(m_parameters.probeCountX),
			static_cast< int32_t >(m_parameters.probeCountY),
			static_cast< int32_t >(m_parameters.probeCountZ)
		};

		/* The camera's grid cell anchors the volume: probes sit on integer multiples of the spacing,
		 * so they do not move while the camera wanders inside a cell. */
		std::array< int32_t, 3 > cell{};

		for ( size_t axis = 0; axis < 3; ++axis )
		{
			cell[axis] = static_cast< int32_t >(std::floor(inputs.cameraPosition[axis] / m_parameters.probeSpacing));
		}

		bool resetAll = !m_hasCameraCell;
		m_resetPlanes = {-1, -1, -1};

		if ( m_hasCameraCell )
		{
			for ( size_t axis = 0; axis < 3; ++axis )
			{
				const auto delta = cell[axis] - m_cameraCell[axis];

				if ( delta == 0 )
				{
					continue;
				}

				/* The volume scrolls by whole cells: the probes that leave one side are the ones that
				 * appear on the other, in the same storage slot — only THAT plane restarts from scratch.
				 * A jump of more than one cell in a frame (a teleport) restarts the whole volume. */
				if ( std::abs(delta) > 1 )
				{
					resetAll = true;

					continue;
				}

				m_scroll[axis] = positiveModulo(m_scroll[axis] + delta, counts[axis]);

				const auto arrivingGridIndex = delta > 0 ? counts[axis] - 1 : 0;

				m_resetPlanes[axis] = positiveModulo(arrivingGridIndex + m_scroll[axis], counts[axis]);
			}
		}

		if ( resetAll )
		{
			m_scroll = {0, 0, 0};
			m_resetPlanes = {-1, -1, -1};
		}

		m_cameraCell = cell;
		m_hasCameraCell = true;
		m_resetAll = resetAll;

		/* A uniformly random rotation (Shoemake, Graphics Gems III) applied to the whole ray set:
		 * the probes see a new set of directions every frame and the hysteresis integrates them. */
		std::uniform_real_distribution< float > unit{0.0F, 1.0F};
		const float u1 = unit(m_random);
		const float u2 = unit(m_random);
		const float u3 = unit(m_random);
		constexpr float TwoPi = 6.283185307179586F;
		const float sqrt1 = std::sqrt(1.0F - u1);
		const float sqrt2 = std::sqrt(u1);
		const float qx = sqrt1 * std::sin(TwoPi * u2);
		const float qy = sqrt1 * std::cos(TwoPi * u2);
		const float qz = sqrt2 * std::sin(TwoPi * u3);
		const float qw = sqrt2 * std::cos(TwoPi * u3);

		/* Centred on the camera cell horizontally; vertically the camera sits at CameraHeightFraction
		 * of the height (a camera stands near the floor of a room whose ceiling is far above it). */
		const auto cellsBelowCamera = static_cast< int32_t >(std::floor(static_cast< float >(counts[1] - 1) * m_parameters.cameraHeightFraction));

		ParametersUBO block{};
		block.originSpacing = {
			static_cast< float >(cell[0] - counts[0] / 2) * m_parameters.probeSpacing,
			static_cast< float >(cell[1] - cellsBelowCamera) * m_parameters.probeSpacing,
			static_cast< float >(cell[2] - counts[2] / 2) * m_parameters.probeSpacing,
			m_parameters.probeSpacing
		};
		block.probeCounts = {counts[0], counts[1], counts[2], static_cast< int32_t >(m_parameters.raysPerProbe)};
		block.scrollReset = {m_scroll[0], m_scroll[1], m_scroll[2], resetAll ? 1 : 0};
		block.resetPlanes = {m_resetPlanes[0], m_resetPlanes[1], m_resetPlanes[2], static_cast< int32_t >(m_frameCounter)};
		block.biasHysteresis = {
			m_parameters.normalBias,
			m_parameters.viewBias,
			m_parameters.hysteresis,
			m_parameters.probeSpacing * MaxRayDistanceCellDiagonals * 1.7320508F
		};
		block.skyAmbient = {inputs.skyLuminance, m_parameters.enabled ? 1.0F : 0.0F, static_cast< float >(inputs.lightCount), m_parameters.bounceFeedback};
		block.ambientColor = {inputs.ambient.red(), inputs.ambient.green(), inputs.ambient.blue(), m_parameters.indirectIntensity};
		/* Rotation matrix of the quaternion, stored as COLUMNS (GLSL mat3 constructor order). */
		block.rotation0 = {1.0F - 2.0F * (qy * qy + qz * qz), 2.0F * (qx * qy + qz * qw), 2.0F * (qx * qz - qy * qw), 0.0F};
		block.rotation1 = {2.0F * (qx * qy - qz * qw), 1.0F - 2.0F * (qx * qx + qz * qz), 2.0F * (qy * qz + qx * qw), 0.0F};
		block.rotation2 = {2.0F * (qx * qz + qy * qw), 2.0F * (qy * qz - qx * qw), 1.0F - 2.0F * (qx * qx + qy * qy), 0.0F};

		if ( frameIndex < m_parameterBuffers.size() )
		{
			const auto & buffer = *m_parameterBuffers[frameIndex];
			auto * pointer = buffer.mapMemoryAs< uint8_t >(0, VK_WHOLE_SIZE);

			if ( pointer != nullptr )
			{
				std::memcpy(pointer, &block, sizeof(ParametersUBO));

				buffer.unmapMemory();
			}
			else
			{
				TraceError{ClassId} << "Unable to map the parameters buffer of frame " << frameIndex << " !";
			}
		}

		m_frameCounter++;
	}

	void
	IrradianceProbeVolume::recordAtlasInitialization (const CommandBuffer & commandBuffer) noexcept
	{
		/* UNDEFINED -> GENERAL, then a clear: the atlases live in GENERAL for their whole life
		 * (storage writes by the update, sampled reads by the consumers, no layout ping-pong). */
		for ( const auto & image : {m_irradianceImage, m_distanceImage} )
		{
			const Sync::ImageMemoryBarrier toGeneral{*image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL};
			commandBuffer.pipelineBarrier(toGeneral, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			commandBuffer.clearColor(*image, VK_IMAGE_LAYOUT_GENERAL);

			const Sync::ImageMemoryBarrier cleared{*image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL};
			commandBuffer.pipelineBarrier(cleared, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		m_atlasesInitialized = true;
	}

	void
	IrradianceProbeVolume::recordUpdate (const CommandBuffer & commandBuffer, uint32_t frameIndex, const DescriptorSet & rtDescriptorSet, const DescriptorSet & bindlessDescriptorSet, const FrameInputs & inputs) noexcept
	{
		if ( !this->usable() || frameIndex >= m_descriptorSets.size() )
		{
			return;
		}

		if ( !m_atlasesInitialized )
		{
			this->recordAtlasInitialization(commandBuffer);
		}

		/* The parameters are written even when disabled: a consumer's query reads the flag there. */
		this->updateParameters(frameIndex, inputs);

		if ( !m_parameters.enabled )
		{
			return;
		}

		const auto & volumeSet = *m_descriptorSets[frameIndex];
		const auto probeTotal = this->probeCount();

		/* Previous consumers (fragment, compute) and the previous update are done with the atlases and
		 * the ray buffer before this update writes them. A pipeline barrier orders against every
		 * earlier submission on the queue, which covers the previous frames' readers. */
		{
			const Sync::MemoryBarrier barrier{VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT};
			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		/* 1. Trace: one invocation per (probe, ray), reads the atlases of the previous frame for the bounce. */
		commandBuffer.bind(*m_tracePipeline);
		commandBuffer.bind(rtDescriptorSet, *m_tracePipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		commandBuffer.bind(bindlessDescriptorSet, *m_tracePipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 1);
		commandBuffer.bind(volumeSet, *m_tracePipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 2);
		commandBuffer.dispatch((probeTotal * m_parameters.raysPerProbe + TraceWorkgroupSize - 1) / TraceWorkgroupSize, 1, 1);

		/* Ray buffer written -> read by the blends. */
		{
			const Sync::MemoryBarrier barrier{VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT};
			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		/* 2-3. Blend: one workgroup per probe. */
		commandBuffer.bind(*m_blendIrradiancePipeline);
		commandBuffer.bind(volumeSet, *m_updatePipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		commandBuffer.dispatch(probeTotal, 1, 1);

		commandBuffer.bind(*m_blendDistancePipeline);
		commandBuffer.dispatch(probeTotal, 1, 1);

		/* Interior texels written -> read by the border copies. */
		{
			const Sync::MemoryBarrier barrier{VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT};
			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		/* 4. Borders: one workgroup per probe. */
		commandBuffer.bind(*m_borderIrradiancePipeline);
		commandBuffer.dispatch(probeTotal, 1, 1);

		commandBuffer.bind(*m_borderDistancePipeline);
		commandBuffer.dispatch(probeTotal, 1, 1);

		/* Atlases complete -> sampled by the consumers of this frame (fragment effects, compute). */
		{
			const Sync::MemoryBarrier barrier{VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT};
			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}
	}
}
