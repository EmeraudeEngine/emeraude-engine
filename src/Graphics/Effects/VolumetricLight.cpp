/*
 * src/Graphics/Effects/VolumetricLight.cpp
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

#include "VolumetricLight.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <string>

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
static constexpr auto TracerTag{"VolumetricLightEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto FullscreenVertexShader = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

	static constexpr auto OcclusionFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
	float lightScreenX;
	float lightScreenY;
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float lightColorR;
	float lightColorG;
	float lightColorB;
	float lightIntensity;
	float density;
	float decay;
	float exposure;
	float depthThreshold;
	uint numSamples;
	float lightOnScreen;
};

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky pixels emit light, geometry blocks it. */
	float isLit = (depth >= depthThreshold) ? 1.0 : 0.0;

	vec3 lightColor = vec3(lightColorR, lightColorG, lightColorB);
	outColor = vec4(lightColor * lightIntensity * isLit, isLit);
}
)GLSL";

	static constexpr auto RadialBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D occlusionTex;

layout(push_constant) uniform PushConstants
{
	float lightScreenX;
	float lightScreenY;
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float lightColorR;
	float lightColorG;
	float lightColorB;
	float lightIntensity;
	float density;
	float decay;
	float exposure;
	float depthThreshold;
	uint numSamples;
	float lightOnScreen;
};

void main()
{
	vec2 lightPos = vec2(lightScreenX, lightScreenY);
	vec2 deltaUV = (vUV - lightPos);
	deltaUV *= (1.0 / float(numSamples)) * density;

	vec2 sampleUV = vUV;
	vec3 color = vec3(0.0);
	float illuminationDecay = 1.0;

	for (uint i = 0u; i < numSamples; ++i)
	{
		sampleUV -= deltaUV;
		vec3 sampleColor = texture(occlusionTex, sampleUV).rgb;
		sampleColor *= illuminationDecay;
		color += sampleColor;
		illuminationDecay *= decay;
	}

	color *= exposure / float(numSamples);
	color *= lightOnScreen;

	outColor = vec4(color, 1.0);
}
)GLSL";

	static constexpr auto CompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D shaftsTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float padding1;
	float padding2;
};

void main()
{
	vec3 scene = texture(sceneTex, vUV).rgb;
	vec3 shafts = texture(shaftsTex, vUV).rgb;
	outColor = vec4(scene + shafts, 1.0);
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

	/* ---- Lifecycle ---- */

	bool
	VolumetricLight::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Create occlusion target (half-res). */
		if ( !m_occlusionTarget.create(renderer, halfW, halfH, format, "VL_Occlusion") )
		{
			TraceError{TracerTag} << "Failed to create occlusion target !";

			return false;
		}

		/* Create radial blur target (half-res). */
		if ( !m_radialTarget.create(renderer, halfW, halfH, format, "VL_Radial") )
		{
			TraceError{TracerTag} << "Failed to create radial blur target !";

			return false;
		}

		/* Create output target (full-res). */
		if ( !m_outputTarget.create(renderer, width, height, format, "VL_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Single input layout (1 combined image sampler). */
		auto singleInputLayout = layoutManager.getDescriptorSetLayout("VLSingleInput");

		if ( singleInputLayout == nullptr )
		{
			singleInputLayout = layoutManager.prepareNewDescriptorSetLayout("VLSingleInput");
			singleInputLayout->setIdentifier(ClassId, "VLSingleInput", "DescriptorSetLayout");
			singleInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(singleInputLayout) )
			{
				return false;
			}
		}

		/* Dual input layout (2 combined image samplers). */
		auto dualInputLayout = layoutManager.getDescriptorSetLayout("VLDualInput");

		if ( dualInputLayout == nullptr )
		{
			dualInputLayout = layoutManager.prepareNewDescriptorSetLayout("VLDualInput");
			dualInputLayout->setIdentifier(ClassId, "VLDualInput", "DescriptorSetLayout");
			dualInputLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			dualInputLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(dualInputLayout) )
			{
				return false;
			}
		}

		/* ---- Pipeline layouts ---- */
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);
			m_occlusionLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);
			m_radialLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);
			m_compositeLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_occlusionLayout == nullptr || m_radialLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = shaderManager.getShaderModuleFromSourceCode(
			device, "VL_VS", Saphir::ShaderType::VertexShader, FullscreenVertexShader
		);

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		auto occlusionFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "VL_Occlusion_FS", Saphir::ShaderType::FragmentShader, OcclusionFragmentShader
		);

		if ( occlusionFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile occlusion shader !";

			return false;
		}

		auto radialFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "VL_Radial_FS", Saphir::ShaderType::FragmentShader, RadialBlurFragmentShader
		);

		if ( radialFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile radial blur shader !";

			return false;
		}

		auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "VL_Composite_FS", Saphir::ShaderType::FragmentShader, CompositeFragmentShader
		);

		if ( compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile composite shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_occlusionPipeline = createFullscreenPipeline(
			renderer, "VL_Occlusion", vertexModule, occlusionFragment, m_occlusionLayout, m_occlusionTarget
		);
		m_radialPipeline = createFullscreenPipeline(
			renderer, "VL_Radial", vertexModule, radialFragment, m_radialLayout, m_radialTarget
		);
		m_compositePipeline = createFullscreenPipeline(
			renderer, "VL_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget
		);

		if ( m_occlusionPipeline == nullptr || m_radialPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();
		const auto frameCount = renderer.framesInFlight();

		/* Occlusion: reads depth (updated per-frame). */
		m_occlusionPerFrame.clear();
		m_occlusionPerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, singleInputLayout);
			ds->setIdentifier(ClassId, "Occlusion_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			m_occlusionPerFrame.emplace_back(std::move(ds));
		}

		/* Radial blur: reads occlusion target (fixed). */
		m_radialDescSet = std::make_unique< DescriptorSet >(pool, singleInputLayout);
		m_radialDescSet->setIdentifier(ClassId, "Radial_DescSet", "DescriptorSet");

		if ( !m_radialDescSet->create() )
		{
			return false;
		}

		if ( !m_radialDescSet->writeCombinedImageSampler(0, m_occlusionTarget) )
		{
			return false;
		}

		/* Composite: reads scene color (updated per-frame, binding 0) + radial result (fixed, binding 1). */
		m_compositePerFrame.clear();
		m_compositePerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, dualInputLayout);
			ds->setIdentifier(ClassId, "Composite_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			/* Binding 1: radial blur result (same for all frames). */
			if ( !ds->writeCombinedImageSampler(1, m_radialTarget) )
			{
				return false;
			}

			m_compositePerFrame.emplace_back(std::move(ds));
		}

		m_renderer = &renderer;

		return true;
	}

	void
	VolumetricLight::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_radialDescSet.reset();
		m_occlusionPerFrame.clear();

		m_renderer = nullptr;

		m_compositePipeline.reset();
		m_radialPipeline.reset();
		m_occlusionPipeline.reset();
		m_compositeLayout.reset();
		m_radialLayout.reset();
		m_occlusionLayout.reset();

		m_outputTarget.destroy();
		m_radialTarget.destroy();
		m_occlusionTarget.destroy();
	}

	bool
	VolumetricLight::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	/* ---- Fullscreen Pass Helper ---- */

	void
	VolumetricLight::recordFullscreenPass (
		const CommandBuffer & commandBuffer,
		IntermediateRenderTarget & target,
		const GraphicsPipeline & pipeline,
		const PipelineLayout & pipelineLayout,
		const DescriptorSet & descriptorSet,
		const void * pushConstants,
		uint32_t pushConstantsSize
	) const noexcept
	{
		target.beginRenderPass(commandBuffer);

		commandBuffer.bind(pipeline);

		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(target.width()),
			.height = static_cast< float >(target.height()),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};
		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

		const VkRect2D scissor{
			.offset = {0, 0},
			.extent = {target.width(), target.height()}
		};
		vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout.handle(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			pushConstantsSize,
			pushConstants
		);

		commandBuffer.bind(descriptorSet, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		commandBuffer.draw(3, 1);

		target.endRenderPass(commandBuffer);
	}

	/* ---- Execute ---- */

	const TextureInterface &
	VolumetricLight::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		[[maybe_unused]] const TextureInterface * inputNormals,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* 1. Project light direction to screen space. */
		const auto & viewMat = m_renderer->mainRenderTarget()->viewMatrices().viewMatrix(false, 0);
		const auto & projMat = m_renderer->mainRenderTarget()->viewMatrices().projectionMatrix();
		const auto & camPos = m_renderer->mainRenderTarget()->viewMatrices().position();

		/* Light source direction (opposite of emission direction). */
		const auto lightSourceX = -m_lightDirX;
		const auto lightSourceY = -m_lightDirY;
		const auto lightSourceZ = -m_lightDirZ;
		const auto invLen = 1.0F / std::sqrt(lightSourceX * lightSourceX + lightSourceY * lightSourceY + lightSourceZ * lightSourceZ + 1e-8F);
		const auto normX = lightSourceX * invLen;
		const auto normY = lightSourceY * invLen;
		const auto normZ = lightSourceZ * invLen;

		/* Project a far point along the light source direction. */
		const auto farPointX = camPos[0] + normX * 10000.0F;
		const auto farPointY = camPos[1] + normY * 10000.0F;
		const auto farPointZ = camPos[2] + normZ * 10000.0F;

		/* Transform to view space (Matrix<4> * Vector<4>). */
		const Libs::Math::Vector< 4, float > worldPos{farPointX, farPointY, farPointZ, 1.0F};
		const auto viewPos = viewMat * worldPos;
		const auto clipPos = projMat * viewPos;

		auto lightOnScreen = (clipPos[3] > 0.0F) ? 1.0F : 0.0F;
		auto screenX = 0.5F;
		auto screenY = 0.5F;

		if ( clipPos[3] > 0.001F )
		{
			screenX = (clipPos[0] / clipPos[3]) * 0.5F + 0.5F;
			screenY = (clipPos[1] / clipPos[3]) * 0.5F + 0.5F;
		}

		/* Fade based on distance from screen center. */
		const auto dx = screenX - 0.5F;
		const auto dy = screenY - 0.5F;
		const auto distFromCenter = std::sqrt(dx * dx + dy * dy);
		lightOnScreen *= std::max(0.0F, std::min(1.0F, 1.0F - distFromCenter * 0.5F));

		/* 2. Update per-frame occlusion descriptor with depth texture. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_occlusionPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		/* 3. Update per-frame composite descriptor with scene color. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Build scatter push constants (shared by occlusion and radial passes). */
		const ScatterPushConstants scatterPC{
			.lightScreenX = screenX,
			.lightScreenY = screenY,
			.texelSizeX = 1.0F / static_cast< float >(m_occlusionTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_occlusionTarget.height()),
			.nearPlane = constants.nearPlane,
			.farPlane = constants.farPlane,
			.lightColorR = m_lightColorR,
			.lightColorG = m_lightColorG,
			.lightColorB = m_lightColorB,
			.lightIntensity = m_lightIntensity,
			.density = m_parameters.density,
			.decay = m_parameters.decay,
			.exposure = m_parameters.exposure,
			.depthThreshold = m_parameters.depthThreshold,
			.numSamples = m_parameters.numSamples,
			.lightOnScreen = lightOnScreen
		};

		/* 4. Pass 1: Occlusion extraction. */
		this->recordFullscreenPass(
			commandBuffer,
			m_occlusionTarget,
			*m_occlusionPipeline,
			*m_occlusionLayout,
			*m_occlusionPerFrame[frameIndex],
			&scatterPC,
			sizeof(ScatterPushConstants)
		);

		/* 5. Pass 2: Radial blur. */
		this->recordFullscreenPass(
			commandBuffer,
			m_radialTarget,
			*m_radialPipeline,
			*m_radialLayout,
			*m_radialDescSet,
			&scatterPC,
			sizeof(ScatterPushConstants)
		);

		/* 6. Pass 3: Composite (additive blend scene + light shafts). */
		const CompositePushConstants compositePC{
			.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
			.padding1 = 0.0F,
			.padding2 = 0.0F
		};

		this->recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_compositePipeline,
			*m_compositeLayout,
			*m_compositePerFrame[frameIndex],
			&compositePC,
			sizeof(CompositePushConstants)
		);

		return m_outputTarget;
	}
}
