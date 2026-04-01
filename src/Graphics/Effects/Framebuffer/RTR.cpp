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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "RTR.hpp"

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
	 *             + normalByteOffset(u32) + materialIndex(u32) */
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
	float fadeScreenEdge;
	uint lightCount;
};

/* Material flag bits (must match GPURTMaterialData). */
const uint HasAlbedoTexture = 1u;

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

/* Compute direct lighting at hit point (simple Lambert diffuse). */
vec3 computeDirectLighting (vec3 hitPos, vec3 hitNormal)
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
			/* Directional light: direction is pre-computed, no attenuation. */
			L = normalize(-dirType.xyz);
		}
		else
		{
			/* Point or spot light: compute direction from position. */
			vec3 toLight = posRadius.xyz - hitPos;
			float dist = length(toLight);
			L = toLight / max(dist, 0.0001);

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

		/* Lambert diffuse. */
		float NdotL = max(dot(hitNormal, L), 0.0);
		totalLight += lightColor * NdotL * attenuation;
	}

	return totalLight;
}

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

	/* Skip very rough surfaces (no visible reflection). */
	if (roughness > 0.5)
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

	/* Offset ray origin along normal to prevent self-intersection. */
	vec3 rayOrigin = worldPos + worldNormal * 0.01;

	/* Trace reflection ray. */
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(
		rayQuery, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF,
		rayOrigin, 0.001, reflDir, maxDistance
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
		vec3 hitPos = rayOrigin + reflDir * hitT;

		/* Transform object-space normal to world space.
		 * VBO normals are already in engine convention (Y+ = down, "Vulkan world axis").
		 * Apply objectToWorld for rotated/scaled instances. */
		vec3 objectNormal = getHitNormal(mesh, barycentrics);
		mat4x3 objectToWorld = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
		vec3 hitNormal = normalize(mat3(objectToWorld) * objectNormal);

		/* Reject self-reflection: if hit normal is too similar to origin normal,
		 * this is a flat surface reflecting itself (e.g. floor → floor). */
		if (dot(hitNormal, worldNormal) > 0.9)
		{
			outReflection = vec4(0.0);
			return;
		}

		/* Compute direct lighting at hit point (Lambert diffuse over all scene lights). */
		vec3 lighting = computeDirectLighting(hitPos, hitNormal);
		lighting += vec3(0.15); /* Minimal ambient so shadowed faces aren't pure black. */

		vec3 litColor = albedo * lighting;

		/* Distance fade: reflection fades as hit gets further from the surface. */
		float distFade = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);

		/* Fresnel (Schlick): stronger reflections at grazing angles.
		 * F0 floor at 0.15 so dielectrics viewed head-on still show visible reflections. */
		float F0 = mix(0.15, 0.9, originMetalness);
		float NdotV = max(dot(worldNormal, -viewDir), 0.0);
		float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

		float confidence = distFade * fresnel;

		outReflection = vec4(litColor * confidence, confidence);
	}
	else
	{
		outReflection = vec4(0.0);
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

	float spatialSigma = float(blurRadius) * 0.5;
	float invSpatialSigma2 = 1.0 / (2.0 * spatialSigma * spatialSigma);
	float invDepthSigma2 = 1.0 / (2.0 * depthSigma * depthSigma);

	for (int i = -blurRadius; i <= blurRadius; i++)
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

	/* Composite shader — blends ray-traced reflections with the scene,
	 * modulated by the per-pixel reflectivity from the material properties G-buffer. */
	static constexpr auto RTRCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D rtrTex;
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
	vec4 rtrData = texture(rtrTex, vUV);

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
	using namespace Libs;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RTR::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: reflected color RGB + confidence A). */
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

		/* Trace input (set 1): depth + normals — 2 combined image samplers. */
		auto traceInputLayout = this->getInputLayout(2);

		/* Single input (blur): 1 combined image sampler. */
		auto blurInputLayout = this->getInputLayout(3);

		/* Composite input (color + blurred RTR + material properties): 3 combined image samplers. */
		auto compositeLayout = this->getInputLayout(3);

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
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(blurInputLayout);

			m_blurLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
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

		return true;
	}

	void
	RTR::destroy () noexcept
	{
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
	RTR::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, const TextureInterface * inputNormals, const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
	{
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

		/* Update color descriptor for composite pass. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for composite pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
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
				.lightCount = this->renderer().rtLightCount()
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
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer, m_outputTarget, *m_compositePipeline, *m_compositeLayout,
				*m_compositePerFrame[frameIndex], &comp, sizeof(CompositePushConstants)
			);
		}

		return m_outputTarget;
	}
}
