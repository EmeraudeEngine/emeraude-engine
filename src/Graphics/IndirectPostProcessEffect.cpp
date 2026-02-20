/*
 * src/Graphics/IndirectPostProcessEffect.cpp
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

#include "IndirectPostProcessEffect.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "IntermediateRenderTarget.hpp"
#include "Renderer.hpp"
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
static constexpr auto TracerTag{"IndirectPostProcessEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace EmEn::Graphics
{
	using namespace Vulkan;

	/* ---- Default resize ---- */

	bool
	IndirectPostProcessEffect::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	/* ---- Shared fullscreen vertex shader ---- */

	std::shared_ptr< ShaderModule >
	IndirectPostProcessEffect::getFullscreenVertexShader (Renderer & renderer) noexcept
	{
		return renderer.shaderManager().getShaderModuleFromSourceCode(
			renderer.device(), "FullscreenPostProcessVS", Saphir::ShaderType::VertexShader, FullscreenVertexShaderSource
		);
	}

	/* ---- Shared fullscreen pipeline creation ---- */

	std::shared_ptr< GraphicsPipeline >
	IndirectPostProcessEffect::createFullscreenPipeline (
		Renderer & renderer,
		const char * tracerTag,
		const std::string & name,
		const std::shared_ptr< ShaderModule > & vertexModule,
		const std::shared_ptr< ShaderModule > & fragmentModule,
		const std::shared_ptr< PipelineLayout > & pipelineLayout,
		const IntermediateRenderTarget & target
	) noexcept
	{
		auto pipeline = std::make_shared< GraphicsPipeline >(renderer.device());
		pipeline->setIdentifier(tracerTag, name, "GraphicsPipeline");

		Libs::StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
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

	/* ---- Shared fullscreen pass recording ---- */

	void
	IndirectPostProcessEffect::recordFullscreenPass (
		const CommandBuffer & commandBuffer,
		IntermediateRenderTarget & target,
		const GraphicsPipeline & pipeline,
		const PipelineLayout & pipelineLayout,
		const DescriptorSet & descriptorSet,
		const void * pushConstants,
		uint32_t pushConstantsSize
	) noexcept
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

	/* ---- Shared descriptor set layout helpers ---- */

	std::shared_ptr< DescriptorSetLayout >
	IndirectPostProcessEffect::getSingleInputLayout (Renderer & renderer) noexcept
	{
		static constexpr auto LayoutId = "PostProcess_SingleSampler";

		auto & layoutManager = renderer.layoutManager();
		auto layout = layoutManager.getDescriptorSetLayout(LayoutId);

		if ( layout == nullptr )
		{
			layout = layoutManager.prepareNewDescriptorSetLayout(LayoutId);
			layout->setIdentifier(TracerTag, LayoutId, "DescriptorSetLayout");
			layout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(layout) )
			{
				return nullptr;
			}
		}

		return layout;
	}

	std::shared_ptr< DescriptorSetLayout >
	IndirectPostProcessEffect::getDualInputLayout (Renderer & renderer) noexcept
	{
		static constexpr auto LayoutId = "PostProcess_DualSampler";

		auto & layoutManager = renderer.layoutManager();
		auto layout = layoutManager.getDescriptorSetLayout(LayoutId);

		if ( layout == nullptr )
		{
			layout = layoutManager.prepareNewDescriptorSetLayout(LayoutId);
			layout->setIdentifier(TracerTag, LayoutId, "DescriptorSetLayout");
			layout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			layout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(layout) )
			{
				return nullptr;
			}
		}

		return layout;
	}

	/* ---- Shared per-frame descriptor set allocation ---- */

	std::vector< std::unique_ptr< DescriptorSet > >
	IndirectPostProcessEffect::createPerFrameDescriptorSets (
		Renderer & renderer,
		const std::shared_ptr< DescriptorSetLayout > & layout,
		const char * classId,
		const std::string & baseName
	) noexcept
	{
		const auto & pool = renderer.descriptorPool();
		const auto frameCount = renderer.framesInFlight();

		std::vector< std::unique_ptr< DescriptorSet > > result;
		result.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, layout);
			ds->setIdentifier(classId, baseName + "-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				TraceError{TracerTag} << "Failed to create descriptor set '" << baseName << "' for frame " << f << " !";

				return {};
			}

			result.emplace_back(std::move(ds));
		}

		return result;
	}
}
