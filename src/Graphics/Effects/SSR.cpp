/*
 * src/Graphics/Effects/SSR.cpp
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
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

/* Defining the resource owner of this translation unit. */
/* NOLINTBEGIN(cert-err58-cpp) : We need static strings. */
static constexpr auto TracerTag{"SSREffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	static constexpr auto FullscreenVertexShader = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

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

	/* Read view-space normal and roughness from MRT (roughness stored in alpha). */
	vec4 normalData = texture(normalTex, vUV);
	vec3 rawN = normalData.rgb;
	float roughness = normalData.a;

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

		/* Read roughness to modulate cubemap fallback. */
		float roughness = texture(normalTex, vUV).a;

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

	/* Composite pass: blends the blurred reflected color with the scene. */
	static constexpr auto SSRCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D ssrTex;

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

	/* ssrData.rgb = blurred reflected color, ssrData.a = blurred confidence. */
	float confidence = ssrData.a;

	if (confidence > 0.001)
	{
		color.rgb = mix(color.rgb, ssrData.rgb / max(confidence, 0.001), confidence * intensity);
	}

	outColor = color;
}
)GLSL";

	[[nodiscard]]
	std::shared_ptr< Vulkan::GraphicsPipeline >
	createFullscreenPipeline (
		Graphics::Renderer & renderer,
		const std::string & name,
		const std::shared_ptr< Vulkan::ShaderModule > & vertexModule,
		const std::shared_ptr< Vulkan::ShaderModule > & fragmentModule,
		const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout,
		const Graphics::IntermediateRenderTarget & target
	) noexcept
	{
		auto pipeline = std::make_shared< Vulkan::GraphicsPipeline >(renderer.device());
		pipeline->setIdentifier(TracerTag, name, "GraphicsPipeline");

		Libs::StaticVector< std::shared_ptr< Vulkan::ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		if ( !pipeline->configureShaderStages(shaderModules) )
		{
			return nullptr;
		}

		if ( !pipeline->configureEmptyVertexInputState() )
		{
			return nullptr;
		}

		if ( !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
		{
			return nullptr;
		}

		Libs::StaticVector< VkDynamicState, 16 > dynamicStates;
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

		if ( !pipeline->configureDynamicStates(dynamicStates) )
		{
			return nullptr;
		}

		if ( !pipeline->configureViewportState(target.width(), target.height()) )
		{
			return nullptr;
		}

		VkPipelineRasterizationStateCreateInfo rasterization{};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.rasterizerDiscardEnable = VK_FALSE;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.depthBiasEnable = VK_FALSE;
		rasterization.lineWidth = 1.0F;

		if ( !pipeline->configureRasterizationState(rasterization) )
		{
			return nullptr;
		}

		if ( !pipeline->configureMultisampleState(1) )
		{
			return nullptr;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		if ( !pipeline->configureDepthStencilState(depthStencil) )
		{
			return nullptr;
		}

		Libs::StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;
		attachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		});

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.logicOpEnable = VK_FALSE;

		if ( !pipeline->configureColorBlendState(attachments, colorBlend) )
		{
			return nullptr;
		}

		const auto renderPass = target.framebuffer().renderPass();

		if ( !pipeline->finalize(renderPass, pipelineLayout, false, false) )
		{
			return nullptr;
		}

		return pipeline;
	}
}

namespace EmEn::Graphics::Effects
{
	using namespace Vulkan;

	bool
	SSR::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: hitUV.xy + confidence.z). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Trace") )
		{
			TraceError{TracerTag} << "Failed to create SSR trace target !";

			return false;
		}

		/* Resolve target (half-res, RGBA16F: reflected color RGB + confidence A). */
		if ( !m_resolveTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Resolve") )
		{
			TraceError{TracerTag} << "Failed to create SSR resolve target !";

			return false;
		}

		/* Blur targets (half-res, RGBA16F). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create SSR blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create SSR blur V target !";

			return false;
		}

		/* Composite target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSR_Output") )
		{
			TraceError{TracerTag} << "Failed to create SSR output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (depth + normals). */
		auto traceInputLayout = layoutManager.getDescriptorSetLayout("SSRTraceInput");

		if ( traceInputLayout == nullptr )
		{
			traceInputLayout = layoutManager.prepareNewDescriptorSetLayout("SSRTraceInput");
			traceInputLayout->setIdentifier(ClassId, "SSRTraceInput", "DescriptorSetLayout");
			traceInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			traceInputLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(traceInputLayout) )
			{
				return false;
			}
		}

		/* Single input (SSR trace result for blur). */
		auto singleLayout = layoutManager.getDescriptorSetLayout("SSRSingleInput");

		if ( singleLayout == nullptr )
		{
			singleLayout = layoutManager.prepareNewDescriptorSetLayout("SSRSingleInput");
			singleLayout->setIdentifier(ClassId, "SSRSingleInput", "DescriptorSetLayout");
			singleLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(singleLayout) )
			{
				return false;
			}
		}

		/* Resolve input (color + trace + depth + normals + env cubemap). */
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

		/* Dual input (color + blurred SSR for composite). */
		auto compositeLayout = layoutManager.getDescriptorSetLayout("SSRCompositeInput");

		if ( compositeLayout == nullptr )
		{
			compositeLayout = layoutManager.prepareNewDescriptorSetLayout("SSRCompositeInput");
			compositeLayout->setIdentifier(ClassId, "SSRCompositeInput", "DescriptorSetLayout");
			compositeLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			compositeLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(compositeLayout) )
			{
				return false;
			}
		}

		/* ---- Pipeline layouts ---- */
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(traceInputLayout);
			m_traceLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ResolvePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(resolveInputLayout);
			m_resolveLayout = layoutManager.getPipelineLayout(sets, ranges);
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
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(compositeLayout);
			m_compositeLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_traceLayout == nullptr || m_resolveLayout == nullptr || m_blurLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = shaderManager.getShaderModuleFromSourceCode(
			device, "SSR_VS", Saphir::ShaderType::VertexShader, FullscreenVertexShader
		);
		auto traceFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSR_Trace_FS", Saphir::ShaderType::FragmentShader, SSRTraceFragmentShader
		);
		auto resolveFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSR_Resolve_FS", Saphir::ShaderType::FragmentShader, SSRResolveFragmentShader
		);
		auto blurFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSR_Blur_FS", Saphir::ShaderType::FragmentShader, SSRBlurFragmentShader
		);
		auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSR_Composite_FS", Saphir::ShaderType::FragmentShader, SSRCompositeFragmentShader
		);

		if ( vertexModule == nullptr || traceFragment == nullptr || resolveFragment == nullptr || blurFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile SSR shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = createFullscreenPipeline(renderer, "SSR_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_resolvePipeline = createFullscreenPipeline(renderer, "SSR_Resolve", vertexModule, resolveFragment, m_resolveLayout, m_resolveTarget);
		m_blurPipeline = createFullscreenPipeline(renderer, "SSR_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_compositePipeline = createFullscreenPipeline(renderer, "SSR_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_resolvePipeline == nullptr || m_blurPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();
		const auto frameCount = renderer.framesInFlight();

		/* Trace: reads depth + normals (updated per-frame). */
		m_tracePerFrame.clear();
		m_tracePerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, traceInputLayout);
			ds->setIdentifier(ClassId, "Trace_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			m_tracePerFrame.emplace_back(std::move(ds));
		}

		/* Resolve: reads color (binding 0, per-frame), trace (binding 1, fixed),
		 * depth (binding 2, per-frame), normals (binding 3, per-frame),
		 * environment cubemap (binding 4, fixed). */
		{
			const auto & cubemap = m_environmentCubemap
				? m_environmentCubemap
				: renderer.getDefaultTextureCubemap();

			m_resolvePerFrame.clear();
			m_resolvePerFrame.reserve(frameCount);

			for ( uint32_t f = 0; f < frameCount; ++f )
			{
				auto ds = std::make_unique< DescriptorSet >(pool, resolveInputLayout);
				ds->setIdentifier(ClassId, "Resolve_DescSet-F" + std::to_string(f), "DescriptorSet");

				if ( !ds->create() )
				{
					return false;
				}

				/* Binding 1: trace result (same target every frame). */
				if ( !ds->writeCombinedImageSampler(1, m_traceTarget) )
				{
					return false;
				}

				/* Binding 4: environment cubemap (fixed). */
				if ( cubemap != nullptr )
				{
					if ( !ds->writeCombinedImageSampler(4, *cubemap) )
					{
						return false;
					}
				}

				m_resolvePerFrame.emplace_back(std::move(ds));
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
		m_compositePerFrame.clear();
		m_compositePerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, compositeLayout);
			ds->setIdentifier(ClassId, "Composite_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			/* Binding 1: blurred SSR (same for all frames). */
			if ( !ds->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}

			m_compositePerFrame.emplace_back(std::move(ds));
		}

		m_renderer = &renderer;

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

		m_renderer = nullptr;

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

	bool
	SSR::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	const TextureInterface &
	SSR::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		const TextureInterface * inputNormals,
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

		/* Update color descriptor for composite pass (this frame's copy). */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* ---- Pass 1: Trace ---- */
		{
			m_traceTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_tracePipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_traceTarget.width()),
				.height = static_cast< float >(m_traceTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_traceTarget.width(), m_traceTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

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

			vkCmdPushConstants(
				commandBuffer.handle(), m_traceLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(TracePushConstants), &pc
			);

			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_traceTarget.endRenderPass(commandBuffer);
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

			/* Compute inverse view matrix for cubemap fallback. */
			const auto & viewMat = m_renderer->mainRenderTarget()->viewMatrices().viewMatrix(false, 0);
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

			m_resolveTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_resolvePipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_resolveTarget.width()),
				.height = static_cast< float >(m_resolveTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_resolveTarget.width(), m_resolveTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			vkCmdPushConstants(
				commandBuffer.handle(), m_resolveLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(ResolvePushConstants), &resolvePC
			);

			commandBuffer.bind(*m_resolvePerFrame[frameIndex], *m_resolveLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_resolveTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 3: Blur Horizontal (on resolved reflected colors) ---- */
		{
			m_blurHTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_blurPipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_blurHTarget.width()),
				.height = static_cast< float >(m_blurHTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_blurHTarget.width(), m_blurHTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_blurLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(BlurPushConstants), &blurH
			);

			commandBuffer.bind(*m_blurHDescSet, *m_blurLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_blurHTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 4: Blur Vertical ---- */
		{
			m_blurVTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_blurPipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_blurVTarget.width()),
				.height = static_cast< float >(m_blurVTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_blurVTarget.width(), m_blurVTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_blurLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(BlurPushConstants), &blurV
			);

			commandBuffer.bind(*m_blurVDescSet, *m_blurLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_blurVTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 5: Composite ---- */
		{
			m_outputTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_compositePipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_outputTarget.width()),
				.height = static_cast< float >(m_outputTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_outputTarget.width(), m_outputTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			const CompositePushConstants comp{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_compositeLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(CompositePushConstants), &comp
			);

			commandBuffer.bind(*m_compositePerFrame[frameIndex], *m_compositeLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_outputTarget.endRenderPass(commandBuffer);
		}

		return m_outputTarget;
	}
}
