/*
 * src/Graphics/Effects/SSAO.cpp
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

#include "SSAO.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
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
static constexpr auto TracerTag{"SSAOEffect"};
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

	static constexpr auto SSAOComputeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out float outAO;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float radius;
	float intensity;
	float bias;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	uint sampleCount;
};

/* Linearize depth from [0,1] range. */
float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

/* Reconstruct view-space position from depth and UV.
 * Uses abs(tanHalfFovY) so a Y-flipped projection matrix (common in Vulkan) is handled. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = abs(tanHalfFovY);
	return vec3(ndc * vec2(t * aspectRatio, t) * linearZ, linearZ);
}

/* Hash function for pseudo-random sampling. */
float hash (vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

/* Generate a sample in a hemisphere using noise-based rotation. */
vec3 hemispherePoint (uint i, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outAO = 1.0;
		return;
	}

	vec3 centerPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal from MRT normal buffer and convert to reconstruction space.
	 * Reconstruction space matches view space for X and Y, but Z is negated
	 * (linearDepth is positive, view-space Z is negative for objects in front of the camera). */
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outAO = 1.0;
		return;
	}

	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Generate per-pixel random rotation. */
	vec2 noiseVec = vec2(hash(vUV), hash(vUV * 2.37));

	/* Build a tangent-space basis. */
	vec3 tangent = normalize(vec3(noiseVec.x, noiseVec.y, 0.0) - normal * dot(vec3(noiseVec.x, noiseVec.y, 0.0), normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	/* Accumulate occlusion. */
	float occlusion = 0.0;

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);
		vec3 samplePos = centerPos + sampleDir * radius;

		/* Project sample back to screen space. */
		float t = abs(tanHalfFovY);
		vec2 sampleUV = samplePos.xy / (samplePos.z * vec2(t * aspectRatio, t)) * 0.5 + 0.5;

		/* Sample depth at projected position. */
		float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);

		/* Range check and accumulate. */
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(centerPos.z - sampleDepth));
		occlusion += (sampleDepth <= samplePos.z - bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / float(sampleCount)) * intensity;
	outAO = clamp(occlusion, 0.0, 1.0);
}
)GLSL";

	static constexpr auto BlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out float outAO;

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

	float result = 0.0;
	result += texture(inputTex, vUV - 2.0 * dir * texelSize).r * 0.06136;
	result += texture(inputTex, vUV - 1.0 * dir * texelSize).r * 0.24477;
	result += texture(inputTex, vUV).r * 0.38774;
	result += texture(inputTex, vUV + 1.0 * dir * texelSize).r * 0.24477;
	result += texture(inputTex, vUV + 2.0 * dir * texelSize).r * 0.06136;

	outAO = result;
}
)GLSL";

	static constexpr auto ApplyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D aoTex;

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
	float ao = texture(aoTex, vUV).r;

	/* Mix towards full AO based on intensity. */
	ao = mix(1.0, ao, intensity);

	outColor = vec4(color.rgb * ao, color.a);
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
	SSAO::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* AO computation target (half-res, single channel). */
		if ( !m_aoTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_AO") )
		{
			TraceError{TracerTag} << "Failed to create SSAO AO target !";

			return false;
		}

		/* Blur targets (half-res, single channel). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create SSAO blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create SSAO blur V target !";

			return false;
		}

		/* Apply target (full-res, RGBA). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSAO_Output") )
		{
			TraceError{TracerTag} << "Failed to create SSAO output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* AO input (depth + normals). */
		auto aoInputLayout = layoutManager.getDescriptorSetLayout("SSAOAOInput");

		if ( aoInputLayout == nullptr )
		{
			aoInputLayout = layoutManager.prepareNewDescriptorSetLayout("SSAOAOInput");
			aoInputLayout->setIdentifier(ClassId, "SSAOAOInput", "DescriptorSetLayout");
			aoInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			aoInputLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(aoInputLayout) )
			{
				return false;
			}
		}

		/* Single input (AO for blur). */
		auto singleLayout = layoutManager.getDescriptorSetLayout("SSAOSingleInput");

		if ( singleLayout == nullptr )
		{
			singleLayout = layoutManager.prepareNewDescriptorSetLayout("SSAOSingleInput");
			singleLayout->setIdentifier(ClassId, "SSAOSingleInput", "DescriptorSetLayout");
			singleLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(singleLayout) )
			{
				return false;
			}
		}

		/* Dual input (color + AO for apply pass). */
		auto dualLayout = layoutManager.getDescriptorSetLayout("SSAODualInput");

		if ( dualLayout == nullptr )
		{
			dualLayout = layoutManager.prepareNewDescriptorSetLayout("SSAODualInput");
			dualLayout->setIdentifier(ClassId, "SSAODualInput", "DescriptorSetLayout");
			dualLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			dualLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(dualLayout) )
			{
				return false;
			}
		}

		/* ---- Pipeline layouts ---- */
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSAOPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(aoInputLayout);
			m_aoLayout = layoutManager.getPipelineLayout(sets, ranges);
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
			sets.emplace_back(dualLayout);
			m_applyLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_aoLayout == nullptr || m_blurLayout == nullptr || m_applyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = shaderManager.getShaderModuleFromSourceCode(
			device, "SSAO_VS", Saphir::ShaderType::VertexShader, FullscreenVertexShader
		);
		auto aoFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSAO_AO_FS", Saphir::ShaderType::FragmentShader, SSAOComputeFragmentShader
		);
		auto blurFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSAO_Blur_FS", Saphir::ShaderType::FragmentShader, BlurFragmentShader
		);
		auto applyFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "SSAO_Apply_FS", Saphir::ShaderType::FragmentShader, ApplyFragmentShader
		);

		if ( vertexModule == nullptr || aoFragment == nullptr || blurFragment == nullptr || applyFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile SSAO shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_aoPipeline = createFullscreenPipeline(renderer, "SSAO_AO", vertexModule, aoFragment, m_aoLayout, m_aoTarget);
		m_blurPipeline = createFullscreenPipeline(renderer, "SSAO_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_applyPipeline = createFullscreenPipeline(renderer, "SSAO_Apply", vertexModule, applyFragment, m_applyLayout, m_outputTarget);

		if ( m_aoPipeline == nullptr || m_blurPipeline == nullptr || m_applyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();
		const auto frameCount = renderer.framesInFlight();

		/* AO computation: reads depth + normals (updated per-frame). */
		m_aoPerFrame.clear();
		m_aoPerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, aoInputLayout);
			ds->setIdentifier(ClassId, "AO_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			m_aoPerFrame.emplace_back(std::move(ds));
		}

		/* Blur H: reads AO result. */
		m_blurHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurHDescSet->setIdentifier(ClassId, "BlurH_DescSet", "DescriptorSet");

		if ( !m_blurHDescSet->create() )
		{
			return false;
		}

		if ( !m_blurHDescSet->writeCombinedImageSampler(0, m_aoTarget) )
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

		/* Apply: reads color (updated per-frame) + blurred AO (fixed). */
		m_applyPerFrame.clear();
		m_applyPerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, dualLayout);
			ds->setIdentifier(ClassId, "Apply_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			/* Binding 1: blurred AO (same for all frames). */
			if ( !ds->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}

			m_applyPerFrame.emplace_back(std::move(ds));
		}

		m_renderer = &renderer;

		return true;
	}

	void
	SSAO::destroy () noexcept
	{
		m_applyPerFrame.clear();
		m_aoPerFrame.clear();
		m_blurVDescSet.reset();
		m_blurHDescSet.reset();

		m_renderer = nullptr;

		m_applyPipeline.reset();
		m_blurPipeline.reset();
		m_aoPipeline.reset();
		m_applyLayout.reset();
		m_blurLayout.reset();
		m_aoLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_aoTarget.destroy();
	}

	bool
	SSAO::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	const TextureInterface &
	SSAO::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		const TextureInterface * inputNormals,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* Update depth + normals descriptors for this frame's AO pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_aoPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_aoPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Update color descriptor for apply pass (this frame's copy). */
		static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* ---- Pass 1: AO Computation ---- */
		{
			m_aoTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_aoPipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_aoTarget.width()),
				.height = static_cast< float >(m_aoTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_aoTarget.width(), m_aoTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			const SSAOPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_aoTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_aoTarget.height()),
				.radius = m_parameters.radius,
				.intensity = m_parameters.intensity,
				.bias = m_parameters.bias,
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.sampleCount = m_parameters.sampleCount
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_aoLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(SSAOPushConstants), &pc
			);

			commandBuffer.bind(*m_aoPerFrame[frameIndex], *m_aoLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_aoTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 2: Horizontal Blur ---- */
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

			const BlurPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_blurLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(BlurPushConstants), &pc
			);

			commandBuffer.bind(*m_blurHDescSet, *m_blurLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_blurHTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 3: Vertical Blur ---- */
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

			const BlurPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_blurLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(BlurPushConstants), &pc
			);

			commandBuffer.bind(*m_blurVDescSet, *m_blurLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_blurVTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 4: Apply AO to Color ---- */
		{
			m_outputTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_applyPipeline);

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

			const ApplyPushConstants pc{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_applyLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(ApplyPushConstants), &pc
			);

			commandBuffer.bind(*m_applyPerFrame[frameIndex], *m_applyLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_outputTarget.endRenderPass(commandBuffer);
		}

		return m_outputTarget;
	}
}
