/*
 * src/Graphics/Effects/Framebuffer/SSGI.cpp
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

#include "SSGI.hpp"

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

	/* SSGI trace pass: one-bounce diffuse indirect lighting via screen-space ray marching.
	 * For each pixel, casts cosine-weighted hemisphere rays through the depth buffer.
	 * On hit, samples the scene color at the hit UV to produce indirect radiance
	 * (color bleeding). This is the screen-space approximation of RTGI.
	 *
	 * Descriptor set 0 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: scene color texture (HDR, for bounce color sampling)
	 */
	static constexpr auto SSGITraceFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outIndirect;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform sampler2D colorTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	float maxDistance;
	float thickness;
	uint sampleCount;
	uint stepCount;
};

/* Linearize depth from [0,1] range (Vulkan [0,1] depth convention). */
float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

/* Reconstruct view-space position from UV and depth. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = abs(tanHalfFovY);
	return vec3(ndc * vec2(t * aspectRatio, t) * linearZ, linearZ);
}

/* Project view-space position back to screen UV. */
vec2 projectToUV (vec3 viewPos)
{
	float t = abs(tanHalfFovY);
	vec2 ndc = viewPos.xy / (viewPos.z * vec2(t * aspectRatio, t));
	return ndc * 0.5 + 0.5;
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

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(0.05), uv)
	          * smoothstep(vec2(0.0), vec2(0.05), vec2(1.0) - uv);
	return fade.x * fade.y;
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outIndirect = vec4(0.0);
		return;
	}

	vec3 centerPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal from MRT normal buffer. */
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outIndirect = vec4(0.0);
		return;
	}

	/* Convert to reconstruction space (Z negated: linearDepth is positive,
	 * view-space Z is negative for objects in front of the camera). */
	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Per-pixel random rotation to break banding. */
	vec2 noiseVec = vec2(hash(vUV), hash(vUV * 2.37));

	/* Build a tangent-space basis around the view-space normal. */
	vec3 tangent = normalize(vec3(noiseVec.x, noiseVec.y, 0.0) - normal * dot(vec3(noiseVec.x, noiseVec.y, 0.0), normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	/* Compute stride length from max distance and step count. */
	float strideLen = maxDistance / float(stepCount);

	/* Adaptive stride: scale with depth so distant pixels cover more ground. */
	float adaptiveStride = strideLen * max(1.0, centerPos.z * 0.1);

	/* Accumulate indirect radiance. */
	vec3 indirectLight = vec3(0.0);

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, normal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Ray march through the depth buffer. */
		bool hit = false;
		vec2 hitUV = vec2(0.0);
		float hitDist = 0.0;

		for (uint s = 1u; s <= stepCount; ++s)
		{
			vec3 rayPos = centerPos + sampleDir * adaptiveStride * float(s);

			/* Project to screen space. */
			vec2 sampleUV = projectToUV(rayPos);

			/* Out of screen bounds. */
			if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
			{
				break;
			}

			/* Compare depth at the projected position. */
			float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
			float diff = rayPos.z - sampleDepth;

			/* Adaptive thickness based on distance (distant surfaces need larger threshold). */
			float adaptiveThick = thickness * max(1.0, sampleDepth * 0.05);

			if (diff > 0.0 && diff < adaptiveThick)
			{
				hitUV = sampleUV;
				hitDist = length(rayPos - centerPos);
				hit = true;
				break;
			}
		}

		if (hit)
		{
			/* Sample scene color at the hit point (the indirect bounce). */
			vec3 hitColor = texture(colorTex, hitUV).rgb;

			/* Distance attenuation: closer bounces contribute more. */
			float distFade = 1.0 - clamp(hitDist / maxDistance, 0.0, 1.0);

			/* Screen edge fade at hit point to avoid artifacts at screen borders. */
			float edgeFade = screenEdgeFade(hitUV);

			/* Lambert BRDF energy conservation: divide by PI.
			 * Cosine weighting is implicit in the hemisphere distribution. */
			indirectLight += (hitColor / 3.14159265) * distFade * edgeFade;
		}
	}

	/* Normalize by sample count. Intensity is applied in the apply pass. */
	indirectLight = indirectLight / float(sampleCount);

	outIndirect = vec4(indirectLight, 1.0);
}
)GLSL";

	/* Bilateral blur shader — depth/normal-aware separable filter.
	 * Preserves edges by weighting samples based on depth and normal similarity.
	 * Identical to the RTGI bilateral blur.
	 * Binding 0: GI input, Binding 1: depth, Binding 2: normals. */
	static constexpr auto SSGIBlurFragmentShader = R"GLSL(
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

	/* Center pixel reference values. */
	vec4 centerGI = texture(inputTex, vUV);
	float centerDepth = texture(depthTex, vUV).r;
	vec3 centerNormal = texture(normalTex, vUV).rgb;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outBlur = centerGI;
		return;
	}

	/* Bilateral weighted accumulation.
	 * Each sample's weight combines:
	 * 1. Spatial Gaussian (distance from center)
	 * 2. Depth similarity (penalize large depth discontinuities)
	 * 3. Normal similarity (penalize different surface orientations) */
	vec4 result = vec4(0.0);
	float totalWeight = 0.0;

	float spatialSigma = float(blurRadius) * 0.5;
	float invSpatialSigma2 = 1.0 / (2.0 * spatialSigma * spatialSigma);
	float invDepthSigma2 = 1.0 / (2.0 * depthSigma * depthSigma);

	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		vec2 sampleUV = vUV + dir * texelSize * float(i);
		vec4 sampleGI = texture(inputTex, sampleUV);
		float sampleDepth = texture(depthTex, sampleUV).r;
		vec3 sampleNormal = texture(normalTex, sampleUV).rgb;

		/* Spatial weight: Gaussian falloff. */
		float spatialW = exp(-float(i * i) * invSpatialSigma2);

		/* Depth weight: penalize depth discontinuities. */
		float depthDiff = abs(centerDepth - sampleDepth);
		float depthW = exp(-depthDiff * depthDiff * invDepthSigma2);

		/* Normal weight: dot product similarity. */
		float normalDot = max(dot(centerNormal, sampleNormal), 0.0);
		float normalW = pow(normalDot, 1.0 / max(normalSigma, 0.001));

		float w = spatialW * depthW * normalW;
		result += sampleGI * w;
		totalWeight += w;
	}

	outBlur = (totalWeight > 0.0) ? result / totalWeight : centerGI;
}
)GLSL";

	/* Apply pass: additive blend of indirect light onto the scene,
	 * modulated by the material properties G-buffer (emissive surfaces
	 * should not receive GI — they emit their own light).
	 * Identical to the RTGI apply pass. */
	static constexpr auto SSGIApplyFragmentShader = R"GLSL(
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
	using namespace Libs;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	SSGI::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: indirect radiance RGB). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_Trace") )
		{
			TraceError{ClassId} << "Failed to create SSGI trace target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_BlurH") )
		{
			TraceError{ClassId} << "Failed to create SSGI blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_BlurV") )
		{
			TraceError{ClassId} << "Failed to create SSGI blur V target !";

			return false;
		}

		/* Apply target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_Output") )
		{
			TraceError{ClassId} << "Failed to create SSGI output target !";

			return false;
		}

		/* ---- Descriptor set layouts (shared) ---- */
		auto tripleLayout = this->getInputLayout(3);

		if ( tripleLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(tripleLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(tripleLayout);

			m_blurLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(tripleLayout);

			m_applyLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ApplyPushConstants)}
			});
		}

		if ( m_traceLayout == nullptr || m_blurLayout == nullptr || m_applyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSGI_Trace_FS", ShaderType::FragmentShader, SSGITraceFragmentShader);
		const auto blurFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSGI_Blur_FS", ShaderType::FragmentShader, SSGIBlurFragmentShader);
		const auto applyFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSGI_Apply_FS", ShaderType::FragmentShader, SSGIApplyFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr || blurFragment == nullptr || applyFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSGI shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "SSGI_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_blurPipeline = this->createFullscreenPipeline(ClassId, "SSGI_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_applyPipeline = this->createFullscreenPipeline(ClassId, "SSGI_Apply", vertexModule, applyFragment, m_applyLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_blurPipeline == nullptr || m_applyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Trace: reads depth + normals + scene color (all updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Blur H: reads trace result (fixed) + depth + normals (per-frame). */
		m_blurHPerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "BlurH_DescSet");

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

		/* Blur V: reads blur H result (fixed) + depth + normals (per-frame). */
		m_blurVPerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "BlurV_DescSet");

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

		/* Apply: reads color (per-frame) + blurred GI (fixed) + material properties (per-frame). */
		m_applyPerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "Apply_DescSet");

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

		return true;
	}

	void
	SSGI::destroy () noexcept
	{
		m_applyPerFrame.clear();
		m_blurVPerFrame.clear();
		m_blurHPerFrame.clear();
		m_tracePerFrame.clear();

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
	SSGI::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, const TextureInterface * inputNormals, const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals + scene color descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, inputColor));

		/* Update color descriptor for apply pass. */
		static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for apply pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* ---- Pass 1: Screen-Space GI Trace ---- */
		{
			const TracePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_traceTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_traceTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.maxDistance = m_parameters.maxDistance,
				.thickness = m_parameters.thickness,
				.sampleCount = m_parameters.sampleCount,
				.stepCount = m_parameters.stepCount
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_traceTarget,
				*m_tracePipeline,
				*m_traceLayout,
				*m_tracePerFrame[frameIndex],
				&pc,
				sizeof(TracePushConstants)
			);
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

		/* ---- Pass 4: Apply GI to scene color ---- */
		{
			const ApplyPushConstants apply{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_applyPipeline,
				*m_applyLayout,
				*m_applyPerFrame[frameIndex],
				&apply,
				sizeof(ApplyPushConstants)
			);
		}

		return m_outputTarget;
	}
}