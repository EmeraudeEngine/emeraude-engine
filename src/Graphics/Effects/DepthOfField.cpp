/*
 * src/Graphics/Effects/DepthOfField.cpp
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

#include "DepthOfField.hpp"

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
static constexpr auto TracerTag{"DepthOfFieldEffect"};
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

	static constexpr auto CoCFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
	float nearPlane;
	float farPlane;
	float focusDistance;
	float aperture;
	float focalLength;
	float cocScale;
	float texelSizeX;
	float texelSizeY;
	uint autoFocus;
};

float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

float computeAutoFocusDistance ()
{
	float totalWeight = 0.0;
	float totalDepth = 0.0;
	vec2 center = vec2(0.5);
	vec2 texel = vec2(texelSizeX, texelSizeY);

	/* 5x5 Gaussian-weighted sampling around screen center.
	 * Multiplier spreads samples across ~40 half-res texels (~80 full-res pixels). */
	for (int y = -2; y <= 2; ++y)
	{
		for (int x = -2; x <= 2; ++x)
		{
			vec2 uv = center + vec2(float(x), float(y)) * texel * 8.0;
			float d = texture(depthTex, uv).r;

			if (d >= 1.0)
				continue;

			float linearZ = linearizeDepth(d);
			float w = exp(-float(x * x + y * y) / 4.5);

			totalDepth += linearZ * w;
			totalWeight += w;
		}
	}

	return (totalWeight > 0.0) ? totalDepth / totalWeight : farPlane;
}

void main()
{
	vec4 color = texture(colorTex, vUV);
	float depth = texture(depthTex, vUV).r;
	float linearZ = linearizeDepth(depth);

	/* Determine focus distance: auto-focus samples center of screen, or use manual value. */
	float fd = (autoFocus != 0u) ? computeAutoFocusDistance() : focusDistance;

	/* Circle of confusion calculation (thin lens model). */
	/* CoC = |aperture * focalLength * (focusDistance - linearZ) / (linearZ * (focusDistance - focalLength))| */
	float focalLengthM = focalLength * 0.001;  /* mm to meters */
	float coc = abs(aperture * focalLengthM * (fd - linearZ) /
		(linearZ * (fd - focalLengthM)));

	/* Normalize CoC to [0, 1] range. */
	coc = clamp(coc * cocScale, 0.0, 1.0);

	/* Store color in RGB and CoC in alpha. */
	outColor = vec4(color.rgb, coc);
}
)GLSL";

	static constexpr auto DoFBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

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

	/* CoC-weighted Gaussian blur. */
	vec4 center = texture(inputTex, vUV);
	float centerCoC = center.a;

	vec4 result = vec4(0.0);
	float totalWeight = 0.0;

	/* 9-tap Gaussian with CoC-weighted spread. */
	const float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

	for (int i = -4; i <= 4; ++i)
	{
		vec2 offset = dir * texelSize * float(i) * centerCoC * 4.0;
		vec4 samp = texture(inputTex, vUV + offset);
		float w = weights[abs(i)];

		/* Only blur with samples that have similar or larger CoC (prevent sharp bleeding into blurry). */
		w *= step(centerCoC - 0.1, samp.a);

		result += samp * w;
		totalWeight += w;
	}

	outColor = result / max(totalWeight, 0.001);
}
)GLSL";

	static constexpr auto DoFCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D originalTex;
layout(set = 0, binding = 1) uniform sampler2D blurredTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float padding1;
	float padding2;
};

void main()
{
	vec4 original = texture(originalTex, vUV);
	vec4 blurred = texture(blurredTex, vUV);

	/* Blend between sharp and blurred based on CoC stored in blurred alpha. */
	float coc = blurred.a;
	outColor = vec4(mix(original.rgb, blurred.rgb, coc), original.a);
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
	DepthOfField::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* CoC target stores color + CoC in alpha (half-res, RGBA16F for precision). */
		if ( !m_cocTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_CoC") )
		{
			TraceError{TracerTag} << "Failed to create DoF CoC target !";

			return false;
		}

		/* Blur targets (half-res). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create DoF blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create DoF blur V target !";

			return false;
		}

		/* Output at full resolution. */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_Output") )
		{
			TraceError{TracerTag} << "Failed to create DoF output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		auto singleLayout = layoutManager.getDescriptorSetLayout("DoFSingleInput");

		if ( singleLayout == nullptr )
		{
			singleLayout = layoutManager.prepareNewDescriptorSetLayout("DoFSingleInput");
			singleLayout->setIdentifier(ClassId, "DoFSingleInput", "DescriptorSetLayout");
			singleLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(singleLayout) )
			{
				return false;
			}
		}

		auto dualLayout = layoutManager.getDescriptorSetLayout("DoFDualInput");

		if ( dualLayout == nullptr )
		{
			dualLayout = layoutManager.prepareNewDescriptorSetLayout("DoFDualInput");
			dualLayout->setIdentifier(ClassId, "DoFDualInput", "DescriptorSetLayout");
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
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CoCPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualLayout);
			m_cocLayout = layoutManager.getPipelineLayout(sets, ranges);
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
			sets.emplace_back(dualLayout);
			m_compositeLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_cocLayout == nullptr || m_blurLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = shaderManager.getShaderModuleFromSourceCode(
			device, "DoF_VS", Saphir::ShaderType::VertexShader, FullscreenVertexShader
		);
		auto cocFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "DoF_CoC_FS", Saphir::ShaderType::FragmentShader, CoCFragmentShader
		);
		auto blurFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "DoF_Blur_FS", Saphir::ShaderType::FragmentShader, DoFBlurFragmentShader
		);
		auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "DoF_Composite_FS", Saphir::ShaderType::FragmentShader, DoFCompositeFragmentShader
		);

		if ( vertexModule == nullptr || cocFragment == nullptr || blurFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile DoF shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_cocPipeline = createFullscreenPipeline(renderer, "DoF_CoC", vertexModule, cocFragment, m_cocLayout, m_cocTarget);
		m_blurPipeline = createFullscreenPipeline(renderer, "DoF_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_compositePipeline = createFullscreenPipeline(renderer, "DoF_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_cocPipeline == nullptr || m_blurPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();

		const auto frameCount = renderer.framesInFlight();

		/* CoC: reads color + depth (updated per-frame, needs per-frame copies). */
		m_cocPerFrame.clear();
		m_cocPerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, dualLayout);
			ds->setIdentifier(ClassId, "CoC_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			m_cocPerFrame.emplace_back(std::move(ds));
		}

		/* Blur H: reads CoC result. */
		m_blurHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurHDescSet->setIdentifier(ClassId, "BlurH_DescSet", "DescriptorSet");

		if ( !m_blurHDescSet->create() )
		{
			return false;
		}

		if ( !m_blurHDescSet->writeCombinedImageSampler(0, m_cocTarget) )
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

		/* Composite: reads original color + blurred result (binding 0 updated per-frame). */
		m_compositePerFrame.clear();
		m_compositePerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, dualLayout);
			ds->setIdentifier(ClassId, "Composite_DescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			/* Binding 1: blurred result (same for all frames). */
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
	DepthOfField::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_cocPerFrame.clear();
		m_compositeDescSet.reset();
		m_blurVDescSet.reset();
		m_blurHDescSet.reset();
		m_cocDescSet.reset();

		m_compositePipeline.reset();
		m_blurPipeline.reset();
		m_cocPipeline.reset();
		m_compositeLayout.reset();
		m_blurLayout.reset();
		m_cocLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_cocTarget.destroy();
	}

	bool
	DepthOfField::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	const TextureInterface &
	DepthOfField::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		[[maybe_unused]] const TextureInterface * inputNormals,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		/* Update per-frame descriptor sets to avoid conflicts with pending command buffers. */
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* Update CoC descriptor: binding 0 = color, binding 1 = depth. */
		static_cast< void >(m_cocPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_cocPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		/* Update composite descriptor: binding 0 = original color. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* ---- Pass 1: CoC Computation ---- */
		{
			m_cocTarget.beginRenderPass(commandBuffer);
			commandBuffer.bind(*m_cocPipeline);

			const VkViewport viewport{
				.x = 0.0F, .y = 0.0F,
				.width = static_cast< float >(m_cocTarget.width()),
				.height = static_cast< float >(m_cocTarget.height()),
				.minDepth = 0.0F, .maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_cocTarget.width(), m_cocTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			const CoCPushConstants pc{
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.focusDistance = m_parameters.focusDistance,
				.aperture = m_parameters.aperture,
				.focalLength = m_parameters.focalLength,
				.cocScale = m_parameters.cocScale,
				.texelSizeX = 1.0F / static_cast< float >(m_cocTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_cocTarget.height()),
				.autoFocus = m_parameters.autoFocus ? 1U : 0U
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_cocLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(CoCPushConstants), &pc
			);

			commandBuffer.bind(*m_cocPerFrame[frameIndex], *m_cocLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_cocTarget.endRenderPass(commandBuffer);
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

		/* ---- Pass 4: Composite ---- */
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

			const CompositePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
				.padding1 = 0.0F,
				.padding2 = 0.0F
			};

			vkCmdPushConstants(
				commandBuffer.handle(), m_compositeLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(CompositePushConstants), &pc
			);

			commandBuffer.bind(*m_compositePerFrame[frameIndex], *m_compositeLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			m_outputTarget.endRenderPass(commandBuffer);
		}

		return m_outputTarget;
	}
}
