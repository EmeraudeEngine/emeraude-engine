/*
 * src/Graphics/SelectionOutline.cpp
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

#include "SelectionOutline.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>

/* Local inclusions. */
#include "Graphics/GrabPass.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SelectionDepthTarget.hpp"
#include "Saphir/Types.hpp"
#include "Scenes/Scene.hpp"
#include "Settings.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;

	namespace
	{
		/** @brief Push constants of the composite (48 bytes, the GLSL block below). */
		struct OutlinePushConstants
		{
			float colorR;
			float colorG;
			float colorB;
			float hiddenOpacity;
			float texelX;
			float texelY;
			float width;
			float nearPlane;
			float farPlane;
			/* 1 when binding 1 holds the scene depth, 0 when a stand-in is bound (every part drawn full). */
			float sceneDepthEnabled;
			float padding0;
			float padding1;
		};

		static_assert(sizeof(OutlinePushConstants) == 48, "OutlinePushConstants must be 48 bytes.");

		/** @brief The shared fullscreen triangle (IndirectPostProcessEffect's, same name: one module). */
		constexpr auto FullscreenVertexShader = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

		/* Each pixel OUTSIDE the selection looks for the nearest selection pixel within the width (a disk): found, it is
		 * outline — soft over its last half pixel —, and the nearest VISIBLE one decides the opacity: a selection pixel is
		 * visible when its depth is the scene's front-most (1 % + 1 cm of linear tolerance: the scene depth is jittered,
		 * the selection's is not). Inside the selection nothing is drawn: an outline, not a tint. */
		constexpr auto CompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D selectionDepthTex;
layout(set = 0, binding = 1) uniform sampler2D sceneDepthTex;

layout(push_constant) uniform PushConstants
{
	vec4 colorHidden;
	vec2 texel;
	float width;
	float nearPlane;
	float farPlane;
	float sceneDepthEnabled;
};

float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;

	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
	if ( textureLod(selectionDepthTex, vUV, 0.0).r < 1.0 )
	{
		discard;
	}

	const int radius = int(ceil(width));
	const float reach = width + 0.5;
	float nearest = 1.0e9;
	float nearestVisible = 1.0e9;

	for ( int dy = -radius; dy <= radius; ++dy )
	{
		for ( int dx = -radius; dx <= radius; ++dx )
		{
			const float distance = length(vec2(dx, dy));

			if ( distance > reach )
			{
				continue;
			}

			const vec2 uv = vUV + vec2(dx, dy) * texel;
			const float selectionDepth = textureLod(selectionDepthTex, uv, 0.0).r;

			if ( selectionDepth >= 1.0 )
			{
				continue;
			}

			nearest = min(nearest, distance);

			bool visible = true;

			if ( sceneDepthEnabled > 0.5 )
			{
				const float selectionLinear = linearizeDepth(selectionDepth);
				const float sceneLinear = linearizeDepth(textureLod(sceneDepthTex, uv, 0.0).r);

				visible = selectionLinear <= sceneLinear * 1.01 + 0.01;
			}

			if ( visible )
			{
				nearestVisible = min(nearestVisible, distance);
			}
		}
	}

	if ( nearest > reach )
	{
		discard;
	}

	const float edge = clamp(reach - nearest, 0.0, 1.0);
	const float opacity = nearestVisible <= reach ? 1.0 : colorHidden.a;

	outColor = vec4(colorHidden.rgb, edge * opacity);
}
)GLSL";

		/**
		 * @brief Decodes one sRGB channel to linear (IEC 61966-2-1).
		 * @param value The encoded value.
		 * @return float
		 */
		[[nodiscard]]
		float
		sRGBToLinear (float value) noexcept
		{
			return value <= 0.04045F ? value / 12.92F : std::pow((value + 0.055F) / 1.055F, 2.4F);
		}
	}

	SelectionOutline::SelectionOutline (Renderer & renderer) noexcept
		: m_renderer{renderer}
	{

	}

	SelectionOutline::~SelectionOutline ()
	{
		this->destroy();
	}

	void
	SelectionOutline::setWidth (float pixels) noexcept
	{
		m_width = std::clamp(pixels, 1.0F, MaxWidth);
	}

	void
	SelectionOutline::setHiddenOpacity (float opacity) noexcept
	{
		m_hiddenOpacity = std::clamp(opacity, 0.0F, 1.0F);
	}

	bool
	SelectionOutline::createSharedResources () noexcept
	{
		/* Nearest, clamp-to-edge: depths are compared per pixel, never interpolated across a silhouette. */
		m_sampler = m_renderer.getSampler("SelectionOutline", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 1.0F;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the sampler !");

			return false;
		}

		auto & layoutManager = m_renderer.layoutManager();

		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( m_descriptorSetLayout == nullptr )
		{
			auto newLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			newLayout->setIdentifier(ClassId, "Composite", "DescriptorSetLayout");
			newLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			newLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(newLayout) )
			{
				Tracer::error(ClassId, "Unable to create the descriptor set layout !");

				return false;
			}

			m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

			if ( m_descriptorSetLayout == nullptr )
			{
				return false;
			}
		}

		/* One set per frame in flight: rewritten every frame, never while a frame in flight reads it. */
		for ( uint32_t frameIndex = 0; frameIndex < m_renderer.framesInFlight(); ++frameIndex )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_renderer.descriptorPool(), m_descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, "Composite", "DescriptorSet");

			if ( !descriptorSet->create() )
			{
				Tracer::error(ClassId, "Unable to create a descriptor set !");

				return false;
			}

			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
		sets.emplace_back(m_descriptorSetLayout);

		m_pipelineLayout = layoutManager.getPipelineLayout(sets, {
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(OutlinePushConstants)
			}
		});

		if ( m_pipelineLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to create the pipeline layout !");

			return false;
		}

		m_sharedResourcesCreated = true;

		return true;
	}

	bool
	SelectionOutline::createPipeline (const Framebuffer & framebuffer, uint32_t width, uint32_t height) noexcept
	{
		const auto & renderPass = framebuffer.renderPass();

		auto & shaderManager = m_renderer.shaderManager();

		const auto vertexModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "FullscreenPostProcessVS", Saphir::ShaderType::VertexShader, FullscreenVertexShader);
		const auto fragmentModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "SelectionOutlineFS", Saphir::ShaderType::FragmentShader, CompositeFragmentShader);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			Tracer::error(ClassId, "Unable to compile the composite shaders !");

			return false;
		}

		auto pipeline = std::make_shared< GraphicsPipeline >(m_renderer.device());
		pipeline->setIdentifier(ClassId, "Composite", "GraphicsPipeline");

		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		if ( !pipeline->configureShaderStages(shaderModules) || !pipeline->configureEmptyVertexInputState() || !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
		{
			return false;
		}

		StaticVector< VkDynamicState, 16 > dynamicStates;
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

		if ( !pipeline->configureDynamicStates(dynamicStates) || !pipeline->configureViewportState(width, height) )
		{
			return false;
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
			return false;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		if ( !pipeline->configureDepthStencilState(depthStencil) )
		{
			return false;
		}

		/* Alpha blended over the final image, every colour attachment of the composite pass alike. */
		StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;

		for ( uint32_t index = 0; index < std::max(1U, renderPass->colorAttachmentCount(0)); ++index )
		{
			attachments.emplace_back(VkPipelineColorBlendAttachmentState{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			});
		}

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.logicOpEnable = VK_FALSE;

		if ( !pipeline->configureColorBlendState(attachments, colorBlend) || !pipeline->finalize(renderPass, m_pipelineLayout, false, false) )
		{
			Tracer::error(ClassId, "Unable to create the composite pipeline !");

			return false;
		}

		if ( m_pipeline != nullptr )
		{
			m_renderer.deferredDestructor().retireObject(std::move(m_pipeline));
		}

		m_pipeline = std::move(pipeline);
		m_pipelineRenderPass = renderPass->handle();

		return true;
	}

	bool
	SelectionOutline::recordDepth (const CommandBuffer & commandBuffer, Scenes::Scene & scene, uint32_t width, uint32_t height, ViewMatricesInterface & mainViewMatrices) noexcept
	{
		if ( scene.highlightedEntity() == nullptr || width == 0 || height == 0 )
		{
			return false;
		}

		/* The target follows the scene extent: a resized one is retired, in-flight frames may still sample it. */
		if ( m_depthTarget != nullptr && (m_depthTarget->extent().width != width || m_depthTarget->extent().height != height) )
		{
			m_renderer.deferredDestructor().retireAction([target = std::move(m_depthTarget)] {
				target->destroyRenderTarget();
			});

			m_depthTarget.reset();
		}

		if ( m_depthTarget == nullptr )
		{
			auto target = std::make_shared< SelectionDepthTarget >(width, height, mainViewMatrices.farPlane());

			if ( !target->createRenderTarget(m_renderer) )
			{
				Tracer::error(ClassId, "Unable to create the selection depth target !");

				return false;
			}

			target->setSourceViewMatrices(mainViewMatrices);

			m_depthTarget = std::move(target);
		}

		constexpr std::array< VkClearValue, 1 > clearValues{
			VkClearValue{.depthStencil = {.depth = 1.0F, .stencil = 0}}
		};

		commandBuffer.beginRenderPass(*m_depthTarget->framebuffer(), m_depthTarget->renderArea(), clearValues, VK_SUBPASS_CONTENTS_INLINE);

		const bool drawn = scene.renderSelectionDepth(m_depthTarget, commandBuffer);

		commandBuffer.endRenderPass();

		/* Explicit write -> read barrier before the composite samples it (render pass dependencies alone were measured
		 * insufficient on MoltenVK between encoders: IndirectPostProcessEffect::recordFullscreenPass()). */
		const Sync::ImageMemoryBarrier barrier{
			*m_depthTarget->depthImage(),
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_DEPTH_BIT
		};

		commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		return drawn;
	}

	void
	SelectionOutline::recordComposite (const CommandBuffer & commandBuffer, const Framebuffer & framebuffer, uint32_t width, uint32_t height, const GrabPass * grabPass, const ViewMatricesInterface & mainViewMatrices, bool sRGBTarget) noexcept
	{
		if ( m_depthTarget == nullptr || width == 0 || height == 0 )
		{
			return;
		}

		if ( !m_sharedResourcesCreated && !this->createSharedResources() )
		{
			return;
		}

		if ( (m_pipeline == nullptr || m_pipelineRenderPass != framebuffer.renderPass()->handle()) && !this->createPipeline(framebuffer, width, height) )
		{
			return;
		}

		const auto & descriptorSet = *m_descriptorSets[m_renderer.currentFrameIndex() % m_descriptorSets.size()];
		const bool hasSceneDepth = grabPass != nullptr && grabPass->hasDepth();

		static_cast< void >(descriptorSet.writeCombinedImageSampler(0, *m_depthTarget->depthImageView(), *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

		/* Without a scene depth, the selection depth stands in (never read: sceneDepthEnabled = 0). */
		if ( hasSceneDepth )
		{
			static_cast< void >(descriptorSet.writeCombinedImageSampler(1, *grabPass->depthImageView(), *grabPass->depthSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}
		else
		{
			static_cast< void >(descriptorSet.writeCombinedImageSampler(1, *m_depthTarget->depthImageView(), *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		/* The colour is authored as displayed (sRGB): an sRGB target encodes on write, so it is handed linear. */
		const auto encode = [sRGBTarget] (float value) noexcept {
			return sRGBTarget ? sRGBToLinear(value) : value;
		};

		const OutlinePushConstants pushConstants{
			.colorR = encode(m_color.red()),
			.colorG = encode(m_color.green()),
			.colorB = encode(m_color.blue()),
			.hiddenOpacity = m_hiddenOpacity,
			.texelX = 1.0F / static_cast< float >(width),
			.texelY = 1.0F / static_cast< float >(height),
			.width = m_width,
			.nearPlane = mainViewMatrices.nearPlane(),
			.farPlane = mainViewMatrices.farPlane(),
			.sceneDepthEnabled = hasSceneDepth ? 1.0F : 0.0F,
			.padding0 = 0.0F,
			.padding1 = 0.0F
		};

		commandBuffer.bind(*m_pipeline);

		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(width),
			.height = static_cast< float >(height),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};
		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

		const VkRect2D scissor{
			.offset = {0, 0},
			.extent = {width, height}
		};
		vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

		vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

		commandBuffer.bind(descriptorSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		commandBuffer.draw(3, 1);
	}

	void
	SelectionOutline::destroy () noexcept
	{
		m_pipeline.reset();
		m_pipelineRenderPass = VK_NULL_HANDLE;
		m_pipelineLayout.reset();
		m_descriptorSets.clear();
		m_descriptorSetLayout.reset();
		m_sampler.reset();

		if ( m_depthTarget != nullptr )
		{
			m_depthTarget->destroyRenderTarget();
			m_depthTarget.reset();
		}

		m_sharedResourcesCreated = false;
	}
}
