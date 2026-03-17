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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "RTGI.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

namespace
{
	using namespace EmEn;

	/* RTGI trace pass: one-bounce diffuse indirect lighting.
	 * For each pixel, casts hemisphere rays against the TLAS. On hit,
	 * samples the surface albedo (bindless texture or scalar) and computes
	 * direct lighting at the hit point. The result is indirect radiance
	 * that produces color bleeding effects.
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
	 *
	 * Descriptor set 2 (bindless textures — from BindlessTextureManager):
	 *   binding 1: sampler2D[] (2D texture array)
	 */
	static constexpr auto RTGITraceFragmentShader = R"GLSL(
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

/* Input textures (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;

/* Bindless textures (set 2). Binding 1 = 2D texture array. */
layout(set = 2, binding = 1) uniform sampler2D textures2D[];

layout(push_constant) uniform PushConstants
{
	mat4 invViewProj;
	vec3 invViewCol0; float viewPosX;
	vec3 invViewCol1; float viewPosY;
	vec3 invViewCol2; float viewPosZ;
	float maxDistance;
	float intensity;
	float bias;
	uint sampleCount;
};

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

MeshAccessor getMeshAccessor (uint instanceIndex, uint primitiveIndex)
{
	MeshAccessor m;

	uvec4 meta0 = meshSSBO.meshEntries[instanceIndex * 2u];
	uvec4 meta1 = meshSSBO.meshEntries[instanceIndex * 2u + 1u];

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

/* Hash function for pseudo-random sampling. */
float hash (vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

/* Generate a cosine-weighted hemisphere sample direction. */
vec3 hemispherePoint (uint i, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

/* Compute direct lighting at hit point (Lambert diffuse over all scene lights).
 * lightCount is derived from push constants but stored in the SSBO header.
 * We pass it via the last push constant field (sampleCount shares the uint slot). */
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
		totalLight += lightColor * NdotL * attenuation;
	}

	return totalLight;
}

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

	/* Transform view-space normal to world space. */
	mat3 invViewRot = mat3(invViewCol0, invViewCol1, invViewCol2);
	vec3 worldNormal = normalize(invViewRot * normalize(rawN));

	/* Build a tangent frame (TBN) around the world normal for hemisphere sampling. */
	vec3 up = abs(worldNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, worldNormal));
	vec3 bitangent = cross(worldNormal, tangent);
	mat3 TBN = mat3(tangent, bitangent, worldNormal);

	/* Per-pixel random rotation to break banding. */
	vec2 noiseVec = vec2(hash(vUV), hash(vUV * 2.37));

	/* Adaptive bias: scale with camera distance AND grazing angle.
	 * Distance: pixel footprint grows → needs larger offset.
	 * NdotV: at grazing angles, rays easily clip the surface → needs extra offset. */
	vec3 viewDir = normalize(worldPos - vec3(viewPosX, viewPosY, viewPosZ));
	float cameraDist = length(worldPos - vec3(viewPosX, viewPosY, viewPosZ));
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
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, worldNormal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Trace a diffuse bounce ray. */
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(
			rayQuery, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF,
			rayOrigin, adaptiveBias, sampleDir, maxDistance
		);

		while (rayQueryProceedEXT(rayQuery)) {}

		if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
		{
			float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
			uint instanceIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
			uint primitiveIndex = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
			vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);

			/* Unpack mesh data. */
			MeshAccessor mesh = getMeshAccessor(instanceIndex, primitiveIndex);

			/* Look up material. */
			uint materialIndex = meshSSBO.meshEntries[instanceIndex * 2u + 1u].w;
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

			/* The indirect radiance is the hit surface's albedo lit by direct light.
			 * This is the one-bounce diffuse GI contribution.
			 * Cosine-weighted by the hemisphere sampling (implicit in the distribution).
			 * Distance attenuation: closer bounces contribute more. */
			float distFade = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);

			/* Lambert BRDF energy conservation: divide by PI. */
			indirectLight += (albedo / 3.14159265) * lighting * distFade;
		}
	}

	/* Normalize by sample count only. Intensity is applied in the apply pass. */
	indirectLight = indirectLight / float(sampleCount);

	outIndirect = vec4(indirectLight, 1.0);
}
)GLSL";

	/* Blur shader — separable Gaussian, same as RTR/RTAO. */
	static constexpr auto RTGIBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outBlur;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float directionX;
	float directionY;
};

void main()
{
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec2 dir = vec2(directionX, directionY);

	/* 9-tap Gaussian blur: [1, 2, 3, 4, 5, 4, 3, 2, 1] / 25 */
	vec4 result = vec4(0.0);
	float total = 0.0;

	for (int i = -4; i <= 4; i++)
	{
		float w = 5.0 - abs(float(i));
		result += texture(inputTex, vUV + dir * texelSize * float(i)) * w;
		total += w;
	}

	outBlur = result / total;
}
)GLSL";

	/* Apply pass: additive blend of indirect light onto the scene,
	 * modulated by the material properties G-buffer (emissive surfaces
	 * should not receive GI — they emit their own light). */
	static constexpr auto RTGIApplyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D giTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float intensity;
	float padding1;
	float padding2;
	float padding3;
};

void main()
{
	vec4 color = texture(colorTex, vUV);
	vec3 gi = texture(giTex, vUV).rgb;

	/* Decode emissive mask from material properties G-buffer.
	 * B channel low nibble = emissiveMask (0 = not emissive, 15 = fully emissive).
	 * Emissive surfaces should not receive GI — they emit their own light. */
	vec4 mp = texture(materialPropsTex, vUV);
	uint bPacked = uint(mp.b * 255.0);
	float emissiveMask = float(bPacked & 0xFu) / 15.0;
	gi *= (1.0 - emissiveMask);

	/* Additive blend: indirect light adds to the scene. */
	color.rgb += gi * intensity;

	outColor = color;
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Vulkan;

	bool
	RTGI::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		/* Pixel doubling: half-res for performance (default), full-res for quality. */
		const auto pixelDoubling = renderer.primaryServices().settings().getOrSetDefault< bool >(
			GraphicsRayTracingGIPixelDoublingKey, DefaultGraphicsRayTracingGIPixelDoubling
		);
		const auto halfW = pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? ((height > 1) ? height / 2 : 1U) : height;

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

		/* Apply target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "RTGI_Output") )
		{
			TraceError{ClassId} << "Failed to create RTGI output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals — 2 combined image samplers. */
		auto traceInputLayout = getInputLayout(renderer, 2);

		/* Single input (blur): 1 combined image sampler. */
		auto singleLayout = getInputLayout(renderer, 1);

		/* Apply input (color + blurred GI + material properties): 3 combined image samplers. */
		auto applyLayout = getInputLayout(renderer, 3);

		if ( traceInputLayout == nullptr || singleLayout == nullptr || applyLayout == nullptr )
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
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);
			m_traceLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleLayout);
			m_blurLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ApplyPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(applyLayout);
			m_applyLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_traceLayout == nullptr || m_blurLayout == nullptr || m_applyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = getFullscreenVertexShader(renderer);
		auto traceFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTGI_Trace_FS", Saphir::ShaderType::FragmentShader, RTGITraceFragmentShader
		);
		auto blurFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTGI_Blur_FS", Saphir::ShaderType::FragmentShader, RTGIBlurFragmentShader
		);
		auto applyFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTGI_Apply_FS", Saphir::ShaderType::FragmentShader, RTGIApplyFragmentShader
		);

		if ( vertexModule == nullptr || traceFragment == nullptr || blurFragment == nullptr || applyFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTGI shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = createFullscreenPipeline(renderer, ClassId, "RTGI_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_blurPipeline = createFullscreenPipeline(renderer, ClassId, "RTGI_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_applyPipeline = createFullscreenPipeline(renderer, ClassId, "RTGI_Apply", vertexModule, applyFragment, m_applyLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_blurPipeline == nullptr || m_applyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();

		/* Trace: set 1 reads depth + normals (updated per-frame). */
		m_tracePerFrame = createPerFrameDescriptorSets(renderer, traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Blur H: reads trace result. */
		m_blurHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurHDescSet->setIdentifier(ClassId, "BlurH_DescSet", "DescriptorSet");

		if ( !m_blurHDescSet->create() )
		{
			return false;
		}

		if ( !m_blurHDescSet->writeCombinedImageSampler(0, m_traceTarget) )
		{
			return false;
		}

		/* Blur V: reads blur H result. */
		m_blurVDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurVDescSet->setIdentifier(ClassId, "BlurV_DescSet", "DescriptorSet");

		if ( !m_blurVDescSet->create() )
		{
			return false;
		}

		if ( !m_blurVDescSet->writeCombinedImageSampler(0, m_blurHTarget) )
		{
			return false;
		}

		/* Apply: reads color (per-frame) + blurred GI (fixed). */
		m_applyPerFrame = createPerFrameDescriptorSets(renderer, applyLayout, ClassId, "Apply_DescSet");

		if ( m_applyPerFrame.empty() )
		{
			return false;
		}

		for ( auto & ds : m_applyPerFrame )
		{
			if ( !ds->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		m_renderer = &renderer;

		return true;
	}

	void
	RTGI::destroy () noexcept
	{
		m_applyPerFrame.clear();
		m_tracePerFrame.clear();
		m_blurVDescSet.reset();
		m_blurHDescSet.reset();

		m_renderer = nullptr;

		m_applyPipeline.reset();
		m_blurPipeline.reset();
		m_tracePipeline.reset();
		m_applyLayout.reset();
		m_blurLayout.reset();
		m_traceLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();
	}

	const TextureInterface &
	RTGI::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		const TextureInterface * inputNormals,
		const TextureInterface * inputMaterialProperties,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* Update depth + normals descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Update color descriptor for apply pass. */
		static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for apply pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* ---- Pass 1: Ray Trace GI ---- */
		{
			/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
			const auto readStateIndex = m_renderer->currentReadStateIndex();
			const auto & viewMatrices = m_renderer->mainRenderTarget()->viewMatrices();
			const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
			const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
			const auto invViewProj = (projMat * viewMat).inverse();
			const auto * ivp = invViewProj.data();

			/* Inverse view rotation for normal transformation (view → world). */
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

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
				.bias = m_parameters.bias,
				.sampleCount = m_parameters.sampleCount
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
			const auto * rtDescSet = m_renderer->rtDescriptorSet();

			if ( rtDescSet != nullptr )
			{
				commandBuffer.bind(*rtDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			}

			/* Bind set 1: Input textures (depth + normals). */
			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

			/* Bind set 2: Bindless textures. */
			const auto * bindlessDescSet = m_renderer->bindlessTextureManager().descriptorSet();

			if ( bindlessDescSet != nullptr )
			{
				commandBuffer.bind(*bindlessDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
			}

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 2: Blur Horizontal ---- */
		{
			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F
			};

			recordFullscreenPass(
				commandBuffer, m_blurHTarget, *m_blurPipeline, *m_blurLayout,
				*m_blurHDescSet, &blurH, sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 3: Blur Vertical ---- */
		{
			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F
			};

			recordFullscreenPass(
				commandBuffer, m_blurVTarget, *m_blurPipeline, *m_blurLayout,
				*m_blurVDescSet, &blurV, sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 4: Apply GI to scene color ---- */
		{
			const ApplyPushConstants apply{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			recordFullscreenPass(
				commandBuffer, m_outputTarget, *m_applyPipeline, *m_applyLayout,
				*m_applyPerFrame[frameIndex], &apply, sizeof(ApplyPushConstants)
			);
		}

		return m_outputTarget;
	}
}
