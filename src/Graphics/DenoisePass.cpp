/*
 * src/Graphics/DenoisePass.cpp
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

#include "DenoisePass.hpp"

/* STL inclusions. */
#include <array>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Graphics/IntermediateRenderTarget.hpp"
#include "Graphics/Renderer.hpp"
#include "Hash/FNV1a.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/UniformBufferObject.hpp"

static constexpr auto TracerTag{"DenoisePass"};

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	/* ---- Lifecycle ---- */

	DenoisePass::DenoisePass (Renderer & renderer) noexcept
		: IndirectPostProcessEffect{renderer}
	{

	}

	DenoisePass::~DenoisePass () = default;

	void
	DenoisePass::destroy () noexcept
	{
		for ( auto & [signature, variant] : m_variants )
		{
			variant.framebufferH.reset();
			variant.framebufferV.reset();
			variant.descriptorSetsHPerFrame.clear();
			variant.descriptorSetsVPerFrame.clear();
			variant.uniformBuffersPerFrame.clear();
			variant.pipeline.reset();
			variant.pipelineLayout.reset();
			variant.renderPass.reset();
		}

		m_variants.clear();
	}

	/* ---- Shader generation ---- */

	std::string
	DenoisePass::buildFragmentShaderSource (const std::vector< GroupEntry > & contributions) noexcept
	{
		bool needsDepth = false;
		bool needsNormals = false;

		for ( const auto & contribution : contributions )
		{
			needsDepth |= contribution.needsDepth;
			needsNormals |= contribution.needsNormals;
		}

		std::stringstream source;

		source <<
			"#version 450\n\n"
			"layout(location = 0) in vec2 vUV;\n";

		for ( size_t index = 0; index < contributions.size(); ++index )
		{
			source << "layout(location = " << index << ") out vec4 " << contributions[index].prefix << "Out;\n";
		}

		uint32_t binding = 0;

		if ( needsDepth )
		{
			source << "layout(set = 0, binding = " << binding++ << ") uniform sampler2D emDepth;\n";
		}

		if ( needsNormals )
		{
			source << "layout(set = 0, binding = " << binding++ << ") uniform sampler2D emNormals;\n";
		}

		for ( const auto & contribution : contributions )
		{
			source << "layout(set = 0, binding = " << binding++ << ") uniform sampler2D " << contribution.prefix << "Src;\n";
		}

		/* Per-frame scalars: one vec4 per contribution (std140-safe). */
		source << "\nlayout(set = 0, binding = " << binding << ") uniform DenoiseDynamics\n{\n";

		for ( const auto & contribution : contributions )
		{
			source << "\tvec4 " << contribution.prefix << "Dynamics0;\n";
		}

		source << "} emDyn;\n";

		source <<
			"\nlayout(push_constant) uniform PushConstants\n"
			"{\n"
			"\tfloat directionX;\n"
			"\tfloat directionY;\n"
			"\tfloat padding0;\n"
			"\tfloat padding1;\n"
			"};\n\n"
			"void main()\n"
			"{\n"
			"\tvec2 emDenoiseDir = vec2(directionX, directionY);\n\n";

		for ( const auto & contribution : contributions )
		{
			source <<
				"\t/* --- " << contribution.prefix << " --- */\n" << contribution.code << '\n' <<
				'\t' << contribution.prefix << "Out = " << contribution.prefix << "Result;\n\n";
		}

		source << "}\n";

		return source.str();
	}

	size_t
	DenoisePass::computeSignature (const std::vector< GroupEntry > & contributions) noexcept
	{
		const auto hashCombine = [] (size_t & seed, size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t hash = Hash::FNV1a(ClassId);

		for ( const auto & contribution : contributions )
		{
			hashCombine(hash, Hash::FNV1a(contribution.prefix));
			hashCombine(hash, static_cast< size_t >(contribution.targetH != nullptr ? contribution.targetH->format() : VK_FORMAT_UNDEFINED));
			hashCombine(hash, static_cast< size_t >(contribution.needsDepth) | (static_cast< size_t >(contribution.needsNormals) << 1U));
		}

		return hash;
	}

	/* ---- Variant management ---- */

	DenoisePass::Variant *
	DenoisePass::getOrCreateVariant (size_t signature, const std::vector< GroupEntry > & contributions) noexcept
	{
		const auto existing = m_variants.find(signature);

		if ( existing != m_variants.end() )
		{
			return existing->second.pipeline != nullptr ? &existing->second : nullptr;
		}

		auto & renderer = this->renderer();
		auto & variant = m_variants[signature];

		bool needsDepth = false;
		bool needsNormals = false;

		for ( const auto & contribution : contributions )
		{
			needsDepth |= contribution.needsDepth;
			needsNormals |= contribution.needsNormals;
		}

		variant.samplerCount = static_cast< uint32_t >(contributions.size()) + static_cast< uint32_t >(needsDepth) + static_cast< uint32_t >(needsNormals);
		variant.width = contributions.front().targetH->width();
		variant.height = contributions.front().targetH->height();

		/* Render pass: one color attachment per contribution, same conventions as the
		 * IntermediateRenderTarget single-attachment pass — DONT_CARE load, STORE, ends
		 * SHADER_READ_ONLY, and FULL (non-by-region) external dependencies: consumers
		 * sample these targets NON-LOCALLY (the V pass reads an H neighbourhood, the
		 * combine upsamples), so the whole write must complete before any read. */
		{
			auto renderPass = std::make_shared< RenderPass >(renderer.device());
			renderPass->setIdentifier(ClassId, "SharedDenoise", "RenderPass");

			RenderSubPass subPass;

			for ( size_t index = 0; index < contributions.size(); ++index )
			{
				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = contributions[index].targetH->format(),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				});

				subPass.addColorAttachment(static_cast< uint32_t >(index), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			renderPass->addSubPass(subPass);

			renderPass->addSubPassDependency(VkSubpassDependency{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = 0
			});

			renderPass->addSubPassDependency(VkSubpassDependency{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dependencyFlags = 0
			});

			if ( !renderPass->createOnHardware() )
			{
				TraceError{TracerTag} << "Failed to create the shared denoise render pass !";

				return nullptr;
			}

			variant.renderPass = std::move(renderPass);
		}

		/* Shader module (cached by signature-derived name). */
		std::stringstream shaderName;
		shaderName << "DenoiseFS";

		for ( const auto & contribution : contributions )
		{
			shaderName << '_' << contribution.prefix;
		}

		auto vertexModule = this->getFullscreenVertexShader();
		auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(renderer.device(), shaderName.str(), ShaderType::FragmentShader, buildFragmentShaderSource(contributions));

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the shared denoise shader '" << shaderName.str() << "' !";

			return nullptr;
		}

		/* Descriptor set layout (samplers + dynamics UBO) and pipeline layout. */
		auto descriptorSetLayout = this->getInputLayout(variant.samplerCount, 1);

		if ( descriptorSetLayout == nullptr )
		{
			return nullptr;
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(descriptorSetLayout);

			variant.pipelineLayout = renderer.layoutManager().getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(DirectionPushConstants)
				}
			});
		}

		if ( variant.pipelineLayout == nullptr )
		{
			return nullptr;
		}

		/* Graphics pipeline: same fullscreen configuration as
		 * IndirectPostProcessEffect::createFullscreenPipeline(), with one blend
		 * attachment per render target. */
		{
			auto pipeline = std::make_shared< GraphicsPipeline >(renderer.device());
			pipeline->setIdentifier(ClassId, shaderName.str(), "GraphicsPipeline");

			StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
			shaderModules.emplace_back(vertexModule);
			shaderModules.emplace_back(fragmentModule);

			if ( !pipeline->configureShaderStages(shaderModules) || !pipeline->configureEmptyVertexInputState() || !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
			{
				return nullptr;
			}

			StaticVector< VkDynamicState, 16 > dynamicStates;
			dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

			if ( !pipeline->configureDynamicStates(dynamicStates) || !pipeline->configureViewportState(variant.width, variant.height) )
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

			if ( !pipeline->configureRasterizationState(rasterization) || !pipeline->configureMultisampleState(1) )
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

			StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;

			for ( size_t index = 0; index < contributions.size(); ++index )
			{
				attachments.emplace_back(VkPipelineColorBlendAttachmentState{
					.blendEnable = VK_FALSE,
					.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
				});
			}

			VkPipelineColorBlendStateCreateInfo colorBlend{};
			colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlend.logicOpEnable = VK_FALSE;

			if ( !pipeline->configureColorBlendState(attachments, colorBlend) )
			{
				return nullptr;
			}

			if ( !pipeline->finalize(variant.renderPass, variant.pipelineLayout, false, false) )
			{
				return nullptr;
			}

			variant.pipeline = std::move(pipeline);
		}

		/* Per-frame descriptor sets (H and V read different sources) + dynamics UBOs. */
		variant.descriptorSetsHPerFrame = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, shaderName.str() + "HDescSet");
		variant.descriptorSetsVPerFrame = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, shaderName.str() + "VDescSet");

		if ( variant.descriptorSetsHPerFrame.empty() || variant.descriptorSetsVPerFrame.empty() )
		{
			return nullptr;
		}

		const VkDeviceSize uboSize = static_cast< VkDeviceSize >(contributions.size()) * 4 * sizeof(float);

		variant.uniformBuffersPerFrame = this->createPerFrameUniformBuffers(uboSize, ClassId, shaderName.str() + "Dynamics");

		if ( variant.uniformBuffersPerFrame.empty() )
		{
			return nullptr;
		}

		for ( size_t frameIndex = 0; frameIndex < variant.uniformBuffersPerFrame.size(); ++frameIndex )
		{
			if ( !variant.descriptorSetsHPerFrame[frameIndex]->writeUniformBufferObject(variant.samplerCount, *variant.uniformBuffersPerFrame[frameIndex]) ||
				 !variant.descriptorSetsVPerFrame[frameIndex]->writeUniformBufferObject(variant.samplerCount, *variant.uniformBuffersPerFrame[frameIndex]) )
			{
				TraceError{TracerTag} << "Failed to bind the shared denoise dynamics UBO !";

				return nullptr;
			}
		}

		return &variant;
	}

	bool
	DenoisePass::updateFramebuffers (Variant & variant, const std::vector< GroupEntry > & contributions) noexcept
	{
		std::vector< VkImageView > viewsH;
		std::vector< VkImageView > viewsV;
		viewsH.reserve(contributions.size());
		viewsV.reserve(contributions.size());

		for ( const auto & contribution : contributions )
		{
			viewsH.emplace_back(contribution.targetH->imageView()->handle());
			viewsV.emplace_back(contribution.targetV->imageView()->handle());
		}

		if ( variant.framebufferH != nullptr && viewsH == variant.attachmentViewsH && viewsV == variant.attachmentViewsV )
		{
			return true;
		}

		/* (Re)build both framebuffers — first use, or an effect recreated its targets. */
		const VkExtent2D extent{variant.width, variant.height};

		auto framebufferH = std::make_unique< Framebuffer >(variant.renderPass, extent);
		framebufferH->setIdentifier(ClassId, "SharedDenoiseH", "Framebuffer");

		auto framebufferV = std::make_unique< Framebuffer >(variant.renderPass, extent);
		framebufferV->setIdentifier(ClassId, "SharedDenoiseV", "Framebuffer");

		for ( size_t index = 0; index < contributions.size(); ++index )
		{
			framebufferH->addAttachment(viewsH[index]);
			framebufferV->addAttachment(viewsV[index]);
		}

		if ( !framebufferH->createOnHardware() || !framebufferV->createOnHardware() )
		{
			TraceError{TracerTag} << "Failed to create the shared denoise framebuffers !";

			return false;
		}

		/* Retire the previous framebuffers through the deferred destructor (in-flight safety). */
		if ( variant.framebufferH != nullptr )
		{
			this->renderer().deferredDestructor().retireAction([oldH = std::shared_ptr< Framebuffer >{std::move(variant.framebufferH)}, oldV = std::shared_ptr< Framebuffer >{std::move(variant.framebufferV)}] () {});
		}

		variant.framebufferH = std::move(framebufferH);
		variant.framebufferV = std::move(framebufferV);
		variant.attachmentViewsH = std::move(viewsH);
		variant.attachmentViewsV = std::move(viewsV);

		return true;
	}

	/* ---- Recording ---- */

	bool
	DenoisePass::record (const CommandBuffer & commandBuffer, const std::vector< GroupEntry > & contributions, const FrameContext & context) noexcept
	{
		if ( contributions.empty() )
		{
			return true;
		}

		/* The whole group must share one extent (formats may differ). */
		for ( const auto & contribution : contributions )
		{
			if ( contribution.source == nullptr || contribution.targetH == nullptr || contribution.targetV == nullptr ||
				 contribution.targetH->width() != contributions.front().targetH->width() ||
				 contribution.targetH->height() != contributions.front().targetH->height() )
			{
				TraceError{TracerTag} << "Inconsistent shared denoise group (null or mismatched-extent contribution from '" << (contribution.prefix != nullptr ? contribution.prefix : "?") << "') !";

				return false;
			}
		}

		auto * variant = this->getOrCreateVariant(computeSignature(contributions), contributions);

		if ( variant == nullptr || !this->updateFramebuffers(*variant, contributions) )
		{
			return false;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Bind the guides + sources for both passes and upload the dynamics. */
		{
			auto & setH = *variant->descriptorSetsHPerFrame[frameIndex];
			auto & setV = *variant->descriptorSetsVPerFrame[frameIndex];

			bool needsDepth = false;
			bool needsNormals = false;

			for ( const auto & contribution : contributions )
			{
				needsDepth |= contribution.needsDepth;
				needsNormals |= contribution.needsNormals;
			}

			uint32_t binding = 0;

			if ( needsDepth )
			{
				if ( context.depth == nullptr )
				{
					TraceError{TracerTag} << "The shared denoise group requires the missing depth guide !";

					return false;
				}

				static_cast< void >(setH.writeCombinedImageSampler(binding, *context.depth));
				static_cast< void >(setV.writeCombinedImageSampler(binding, *context.depth));
				++binding;
			}

			if ( needsNormals )
			{
				if ( context.normals == nullptr )
				{
					TraceError{TracerTag} << "The shared denoise group requires the missing normals guide !";

					return false;
				}

				static_cast< void >(setH.writeCombinedImageSampler(binding, *context.normals));
				static_cast< void >(setV.writeCombinedImageSampler(binding, *context.normals));
				++binding;
			}

			for ( const auto & contribution : contributions )
			{
				/* H reads the trace source; V reads the H result. */
				static_cast< void >(setH.writeCombinedImageSampler(binding, *contribution.source));
				static_cast< void >(setV.writeCombinedImageSampler(binding, *contribution.targetH));
				++binding;
			}

			std::vector< float > dynamicsData(contributions.size() * 4, 0.0F);

			for ( size_t index = 0; index < contributions.size(); ++index )
			{
				const auto & vector = contributions[index].dynamics;

				dynamicsData[(index * 4) + 0] = vector.x();
				dynamicsData[(index * 4) + 1] = vector.y();
				dynamicsData[(index * 4) + 2] = vector.z();
				dynamicsData[(index * 4) + 3] = vector.w();
			}

			if ( !updateUniformBufferData(*variant->uniformBuffersPerFrame[frameIndex], dynamicsData.data(), dynamicsData.size() * sizeof(float)) )
			{
				TraceError{TracerTag} << "Failed to upload the shared denoise dynamics !";

				return false;
			}
		}

		/* Record the two passes. */
		const VkRect2D renderArea{
			.offset = {0, 0},
			.extent = {variant->width, variant->height}
		};

		/* Every attachment loads DONT_CARE: no clear value is consumed. */
		const std::array< VkClearValue, 0 > clearValues{};

		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(variant->width),
			.height = static_cast< float >(variant->height),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};

		const std::array< DirectionPushConstants, 2 > directions{
			DirectionPushConstants{1.0F, 0.0F, 0.0F, 0.0F},
			DirectionPushConstants{0.0F, 1.0F, 0.0F, 0.0F}
		};

		for ( size_t pass = 0; pass < 2; ++pass )
		{
			const auto & framebuffer = pass == 0 ? *variant->framebufferH : *variant->framebufferV;
			const auto & descriptorSet = pass == 0 ? *variant->descriptorSetsHPerFrame[frameIndex] : *variant->descriptorSetsVPerFrame[frameIndex];

			commandBuffer.beginRenderPass(framebuffer, renderArea, clearValues, VK_SUBPASS_CONTENTS_INLINE);
			commandBuffer.bind(*variant->pipeline);

			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {variant->width, variant->height}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			vkCmdPushConstants(
				commandBuffer.handle(),
				variant->pipelineLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(DirectionPushConstants),
				&directions[pass]
			);

			commandBuffer.bind(descriptorSet, *variant->pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			commandBuffer.draw(3, 1);
			commandBuffer.endRenderPass();
		}

		return true;
	}
}
