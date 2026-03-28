/*
 * src/Graphics/Effects/Framebuffer/SSR.cpp
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

#include "SSR.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
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

	static constexpr auto SSRTraceFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outHit;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	float maxDistance;
	float stride;
	float thickness;
	float fadeScreenEdge;
	uint maxSteps;
	uint binarySteps;
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

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(fadeScreenEdge), uv)
	          * smoothstep(vec2(0.0), vec2(fadeScreenEdge), vec2(1.0) - uv);
	return fade.x * fade.y;
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outHit = vec4(0.0);
		return;
	}

	vec3 viewPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal and packed roughness+metalness from MRT.
	 * Alpha encoding: alpha = roughness + metalness * 2.0
	 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0; */
	vec4 normalData = texture(normalTex, vUV);
	vec3 rawN = normalData.rgb;
	float packedRM = normalData.a;
	float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;

	if (dot(rawN, rawN) < 0.0001)
	{
		outHit = vec4(0.0);
		return;
	}

	/* Skip expensive ray march for very rough surfaces (no visible reflection). */
	if (roughness > 0.5)
	{
		outHit = vec4(0.0);
		return;
	}

	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Compute reflection direction in reconstruction space. */
	vec3 viewDir = normalize(viewPos);
	vec3 reflDir = reflect(viewDir, normal);

	/* Skip reflections pointing towards the camera. */
	if (reflDir.z < 0.0)
	{
		outHit = vec4(0.0);
		return;
	}

	/* Offset the ray origin along the normal to prevent self-intersection.
	 * Use a depth-proportional bias so distant surfaces get a larger offset. */
	float bias = max(stride * 3.0, viewPos.z * 0.002);
	vec3 rayOrigin = viewPos + normal * bias;

	/* Adaptive stride: scale step size proportional to depth so that
	 * distant reflections cover more ground per step. */
	float adaptiveStride = stride * max(1.0, viewPos.z * 0.1);

	/* Linear ray march in reconstruction space. */
	vec3 rayPos = rayOrigin;
	vec2 hitUV = vec2(0.0);
	bool hit = false;

	for (uint i = 0u; i < maxSteps; ++i)
	{
		rayPos += reflDir * adaptiveStride;

		/* Check max distance. */
		float travelDist = length(rayPos - viewPos);

		if (travelDist > maxDistance)
		{
			break;
		}

		/* Project to screen space. */
		vec2 sampleUV = projectToUV(rayPos);

		/* Out of screen bounds. */
		if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
		{
			break;
		}

		/* Compare depths.  Use a depth-proportional thickness so thin
		 * features far from the camera are not missed. */
		float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
		float diff = rayPos.z - sampleDepth;
		float adaptiveThickness = thickness * max(1.0, sampleDepth * 0.05);

		if (diff > 0.0 && diff < adaptiveThickness)
		{
			hitUV = sampleUV;
			hit = true;
			break;
		}
	}

	/* Binary refinement at hit point for sub-step precision. */
	if (hit)
	{
		vec3 refinePos = rayPos;
		float refineStep = adaptiveStride * 0.5;

		for (uint b = 0u; b < binarySteps; ++b)
		{
			vec2 sampleUV = projectToUV(refinePos);
			float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
			float diff = refinePos.z - sampleDepth;

			if (diff > 0.0)
			{
				refinePos -= reflDir * refineStep;
			}
			else
			{
				refinePos += reflDir * refineStep;
			}

			refineStep *= 0.5;
		}

		hitUV = projectToUV(refinePos);
	}

	/* Compute confidence from multiple fade factors. */
	float confidence = 0.0;

	if (hit)
	{
		/* Distance fade. */
		float rayDist = length(rayPos - viewPos);
		float distFade = 1.0 - clamp(rayDist / maxDistance, 0.0, 1.0);

		/* Screen edge fade. */
		float edgeFade = screenEdgeFade(hitUV);

		/* Facing fade: reflections nearly parallel to the view direction are weak. */
		float facingFade = 1.0 - pow(max(0.0, dot(viewDir, reflDir)), 5.0);

		/* Roughness fade: smooth surfaces reflect, rough surfaces don't. */
		float roughnessFade = 1.0 - smoothstep(0.0, 0.4, roughness);

		confidence = distFade * edgeFade * facingFade * roughnessFade;
	}

	outHit = vec4(hitUV, confidence, 0.0);
}
)GLSL";

	static constexpr auto SSRBlurFragmentShader = R"GLSL(
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

	vec4 result = vec4(0.0);
	result += texture(inputTex, vUV - 2.0 * dir * texelSize) * 0.06136;
	result += texture(inputTex, vUV - 1.0 * dir * texelSize) * 0.24477;
	result += texture(inputTex, vUV) * 0.38774;
	result += texture(inputTex, vUV + 1.0 * dir * texelSize) * 0.24477;
	result += texture(inputTex, vUV + 2.0 * dir * texelSize) * 0.06136;

	outBlur = result;
}
)GLSL";

	/* Resolve pass: reads the trace hit data and the scene color,
	 * outputs the reflected color weighted by confidence.
	 * This converts (hitUV, confidence) into (reflectedColor * confidence, confidence)
	 * so that the subsequent blur operates on colors, not UV coordinates.
	 * When confidence is zero (SSR miss), falls back to sampling the environment cubemap. */
	static constexpr auto SSRResolveFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolve;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D traceTex;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D normalTex;
layout(set = 0, binding = 4) uniform samplerCube envCubemap;

layout(push_constant) uniform PushConstants
{
	vec4 invViewCol0;
	vec4 invViewCol1;
	vec4 invViewCol2;
	float texelSizeX, texelSizeY;
	float nearPlane, farPlane;
	float tanHalfFovY, aspectRatio;
	float envFallbackIntensity;
	float intensity;
};

float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

void main()
{
	vec4 traceData = texture(traceTex, vUV);
	float confidence = traceData.z;

	if (confidence > 0.001)
	{
		/* SSR hit: sample reflected color at hitUV. */
		vec3 reflColor = texture(colorTex, traceData.xy).rgb;
		outResolve = vec4(reflColor, confidence);
	}
	else if (envFallbackIntensity > 0.0)
	{
		/* No SSR hit: cubemap fallback. */
		float depth = texture(depthTex, vUV).r;

		if (depth >= 1.0)
		{
			outResolve = vec4(0.0);
			return;
		}

		/* Read packed roughness+metalness to modulate cubemap fallback.
		 * Decode: metalness = (alpha >= 2.0) ? 1.0 : 0.0; roughness = alpha - metalness * 2.0; */
		float packedRM = texture(normalTex, vUV).a;
		float roughness = packedRM >= 2.0 ? packedRM - 2.0 : packedRM;

		/* Reconstruct view-space position (standard: Z negative = into screen). */
		float linearZ = linearizeDepth(depth);
		vec2 ndc = vUV * 2.0 - 1.0;
		float t = abs(tanHalfFovY);
		vec3 viewPos = vec3(ndc.x * t * aspectRatio * linearZ,
		                    ndc.y * t * linearZ, -linearZ);

		/* Read view-space normal from MRT. */
		vec3 rawN = texture(normalTex, vUV).rgb;

		if (dot(rawN, rawN) < 0.001)
		{
			outResolve = vec4(0.0);
			return;
		}

		vec3 normal = normalize(rawN);

		/* Reflection in view space, then transform to world space for cubemap lookup. */
		vec3 reflDir = reflect(normalize(viewPos), normal);
		mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
		vec3 worldReflDir = invViewRot * reflDir;

		/* Reduce cubemap fallback for rough surfaces. */
		float roughnessFallback = envFallbackIntensity * (1.0 - smoothstep(0.0, 0.4, roughness));

		vec3 envColor = texture(envCubemap, worldReflDir).rgb;
		outResolve = vec4(envColor, roughnessFallback);
	}
	else
	{
		outResolve = vec4(0.0);
	}
}
)GLSL";

	/* Composite pass: blends the blurred reflected color with the scene,
	 * modulated by the per-pixel reflectivity from the material properties G-buffer. */
	static constexpr auto SSRCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D ssrTex;
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
	vec4 ssrData = texture(ssrTex, vUV);

	/* Decode reflectivity from the material properties G-buffer (R channel, high nibble). */
	vec4 mp = texture(materialPropsTex, vUV);
	uint rPacked = uint(mp.r * 255.0);
	float reflectivity = float(rPacked >> 4u) / 15.0;

	/* ssrData.rgb = blurred reflected color, ssrData.a = blurred confidence. */
	float confidence = ssrData.a;

	if (confidence > 0.001 && reflectivity > 0.0)
	{
		color.rgb = mix(color.rgb, ssrData.rgb / max(confidence, 0.001), confidence * intensity * reflectivity);
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
	SSR::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: hitUV.xy + confidence.z). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Trace") )
		{
			TraceError{ClassId} << "Failed to create SSR trace target !";

			return false;
		}

		/* Resolve target (half-res, RGBA16F: reflected color RGB + confidence A). */
		if ( !m_resolveTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Resolve") )
		{
			TraceError{ClassId} << "Failed to create SSR resolve target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurH") )
		{
			TraceError{ClassId} << "Failed to create SSR blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurV") )
		{
			TraceError{ClassId} << "Failed to create SSR blur V target !";

			return false;
		}

		/* Composite target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Output") )
		{
			TraceError{ClassId} << "Failed to create SSR output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (depth + normals): 2 combined image samplers at bindings 0,1. */
		auto traceInputLayout = this->getInputLayout(2);

		/* Single input (SSR trace result for blur): 1 combined image sampler at binding 0. */
		auto singleLayout = this->getInputLayout(1);

		/* Resolve input (color + trace + depth + normals + env cubemap): 5 bindings, custom. */
		auto resolveInputLayout = layoutManager.getDescriptorSetLayout("SSRResolveInput");

		if ( resolveInputLayout == nullptr )
		{
			resolveInputLayout = layoutManager.prepareNewDescriptorSetLayout("SSRResolveInput");
			resolveInputLayout->setIdentifier(ClassId, "SSRResolveInput", "DescriptorSetLayout");
			resolveInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(3, VK_SHADER_STAGE_FRAGMENT_BIT);
			resolveInputLayout->declareCombinedImageSampler(4, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(resolveInputLayout) )
			{
				return false;
			}
		}

		/* Composite input (color + blurred SSR + material properties): 3 combined image samplers at bindings 0,1,2. */
		auto compositeLayout = this->getInputLayout(3);

		if ( traceInputLayout == nullptr || singleLayout == nullptr || resolveInputLayout == nullptr || compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(traceInputLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(resolveInputLayout);

			m_resolveLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ResolvePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(singleLayout);

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

		if ( m_traceLayout == nullptr || m_resolveLayout == nullptr || m_blurLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Trace_FS", ShaderType::FragmentShader, SSRTraceFragmentShader);
		const auto resolveFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Resolve_FS", ShaderType::FragmentShader, SSRResolveFragmentShader);
		const auto blurFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Blur_FS", ShaderType::FragmentShader, SSRBlurFragmentShader);
		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSR_Composite_FS", ShaderType::FragmentShader, SSRCompositeFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr || resolveFragment == nullptr || blurFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSR shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "SSR_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_resolvePipeline = this->createFullscreenPipeline(ClassId, "SSR_Resolve", vertexModule, resolveFragment, m_resolveLayout, m_resolveTarget);
		m_blurPipeline = this->createFullscreenPipeline(ClassId, "SSR_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "SSR_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_resolvePipeline == nullptr || m_blurPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();

		/* Trace: reads depth + normals (updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Resolve: reads color (binding 0, per-frame), trace (binding 1, fixed),
		 * depth (binding 2, per-frame), normals (binding 3, per-frame),
		 * environment cubemap (binding 4, fixed). */
		{
			const auto & cubemap = m_fallbackEnvCubemap
				? m_fallbackEnvCubemap
				: renderer.getDefaultTextureCubemap();

			m_resolvePerFrame = this->createPerFrameDescriptorSets(resolveInputLayout, ClassId, "Resolve_DescSet");

			if ( m_resolvePerFrame.empty() )
			{
				return false;
			}

			for ( const auto & descriptorSet : m_resolvePerFrame )
			{
				/* Binding 1: trace result (same target every frame). */
				if ( !descriptorSet->writeCombinedImageSampler(1, m_traceTarget) )
				{
					return false;
				}

				/* Binding 4: environment cubemap (fixed). */
				if ( cubemap != nullptr )
				{
					if ( !descriptorSet->writeCombinedImageSampler(4, *cubemap) )
					{
						return false;
					}
				}
			}
		}

		/* Blur H: reads resolve result. */
		m_blurHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurHDescSet->setIdentifier(ClassId, "BlurH_DescSet", "DescriptorSet");

		if ( !m_blurHDescSet->create() )
		{
			return false;
		}

		if ( !m_blurHDescSet->writeCombinedImageSampler(0, m_resolveTarget) )
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

		/* Composite: reads color (updated per-frame) + blurred SSR (fixed). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(compositeLayout, ClassId, "Composite_DescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			/* Binding 1: blurred SSR (same for all frames). */
			if ( !descriptorSet->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	SSR::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_resolvePerFrame.clear();
		m_tracePerFrame.clear();
		m_blurVDescSet.reset();
		m_blurHDescSet.reset();
		
		m_compositePipeline.reset();
		m_blurPipeline.reset();
		m_resolvePipeline.reset();
		m_tracePipeline.reset();
		m_compositeLayout.reset();
		m_blurLayout.reset();
		m_resolveLayout.reset();
		m_traceLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_resolveTarget.destroy();
		m_traceTarget.destroy();
	}

	const TextureInterface &
	SSR::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, const TextureInterface * inputNormals, const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
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

		/* Update color descriptor for composite pass (this frame's copy). */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for composite pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* ---- Pass 1: Trace ---- */
		{
			const TracePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_traceTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_traceTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.maxDistance = m_parameters.maxDistance,
				.stride = m_parameters.stride,
				.thickness = m_parameters.thickness,
				.fadeScreenEdge = m_parameters.fadeScreenEdge,
				.maxSteps = m_parameters.maxSteps,
				.binarySteps = m_parameters.binarySteps
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

		/* ---- Pass 2: Resolve (sample reflected color at hitUV, cubemap fallback on miss) ---- */
		{
			/* Update per-frame descriptors: color (binding 0), depth (binding 2), normals (binding 3). */
			static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

			if ( inputDepth != nullptr )
			{
				static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(2, *inputDepth));
			}

			if ( inputNormals != nullptr )
			{
				static_cast< void >(m_resolvePerFrame[frameIndex]->writeCombinedImageSampler(3, *inputNormals));
			}

			/* Compute inverse view matrix for cubemap fallback.
			 * Use readStateIndex to match the view matrix that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMat = this->renderer().mainRenderTarget()->viewMatrices().viewMatrix(readStateIndex, false, 0);
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			const ResolvePushConstants resolvePC{
				.invViewCol0 = {inv[0], inv[1], inv[2], 0.0F},
				.invViewCol1 = {inv[4], inv[5], inv[6], 0.0F},
				.invViewCol2 = {inv[8], inv[9], inv[10], 0.0F},
				.texelSizeX = 1.0F / static_cast< float >(m_resolveTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_resolveTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.envFallbackIntensity = m_parameters.envFallbackIntensity,
				.intensity = m_parameters.intensity
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_resolveTarget,
				*m_resolvePipeline,
				*m_resolveLayout,
				*m_resolvePerFrame[frameIndex],
				&resolvePC,
				sizeof(ResolvePushConstants)
			);
		}

		/* ---- Pass 3: Blur Horizontal (on resolved reflected colors) ---- */
		{
			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurHTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurHDescSet,
				&blurH,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 4: Blur Vertical ---- */
		{
			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_blurVTarget,
				*m_blurPipeline,
				*m_blurLayout,
				*m_blurVDescSet,
				&blurV,
				sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 5: Composite ---- */
		{
			const CompositePushConstants comp{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_compositePipeline,
				*m_compositeLayout,
				*m_compositePerFrame[frameIndex],
				&comp,
				sizeof(CompositePushConstants)
			);
		}

		return m_outputTarget;
	}
}
