/*
 * src/Graphics/PathDebugOverlay.cpp
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

#include "PathDebugOverlay.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/PathGLSL.hpp"
#include "Scenes/SceneInstanceTransforms.hpp"
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
#include "Vulkan/ShaderStorageBufferObject.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	namespace
	{
		/** @brief Push constants of one path draw (64 bytes, the GLSL block below). */
		struct PathPushConstants
		{
			std::array< uint32_t, 4 > span;
			std::array< float, 4 > style;
			std::array< float, 4 > color;
			std::array< float, 4 > flags;
		};

		static_assert(sizeof(PathPushConstants) == 64, "PathPushConstants must be 64 bytes.");

		/** @brief The buffer header: view (16 floats), projection (16), eye (4), viewport (4) — then the {point, point} pairs. */
		constexpr size_t HeaderFloats{40};

		/** @brief The initial buffer size: the header and 1024 points. */
		constexpr VkDeviceSize InitialBytes{(HeaderFloats + 1024 * 8) * sizeof(float)};

		constexpr auto BlockDeclarations = R"GLSL(#version 450

layout(std430, set = 0, binding = 0) readonly buffer PathPoints
{
	mat4 view;
	mat4 projection;
	vec4 eye;
	vec4 viewport;
	vec4 pathPoints[];
} ubPathPoints;

layout(push_constant) uniform PushConstants
{
	uvec4 span;
	vec4 style;
	vec4 color;
	vec4 flags;
} pc;

)GLSL";

		constexpr auto VertexMain = R"GLSL(
layout(location = 0) out vec4 vCoordinates;

void main()
{
	vec4 coordinates;
	const vec3 world = pathCorner(mat4(1.0), pc.span, gl_VertexIndex, false, pc.style, ubPathPoints.eye.xyz, ubPathPoints.view, ubPathPoints.projection, ubPathPoints.viewport.xy, coordinates);

	vCoordinates = coordinates;
	gl_Position = ubPathPoints.projection * ubPathPoints.view * vec4(world, 1.0);
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

	PathDebugOverlay::PathDebugOverlay (Renderer & renderer) noexcept
		: m_renderer{renderer}
	{

	}

	PathDebugOverlay::~PathDebugOverlay ()
	{
		this->destroy();
	}

	bool
	PathDebugOverlay::createSharedResources () noexcept
	{
		auto & layoutManager = m_renderer.layoutManager();

		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( m_descriptorSetLayout == nullptr )
		{
			auto newLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			newLayout->setIdentifier(ClassId, "Points", "DescriptorSetLayout");
			newLayout->declareStorageBuffer(0, VK_SHADER_STAGE_VERTEX_BIT);

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

		/* One buffer and one set per frame in flight (engine Rule 1): written every frame, never while a frame in flight
		 * reads it. */
		for ( uint32_t frameIndex = 0; frameIndex < m_renderer.framesInFlight(); ++frameIndex )
		{
			auto buffer = std::make_unique< ShaderStorageBufferObject >(m_renderer.device(), InitialBytes);

			if ( !buffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create a point buffer !");

				return false;
			}

			auto descriptorSet = std::make_unique< DescriptorSet >(m_renderer.descriptorPool(), m_descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, "Points", "DescriptorSet");

			if ( !descriptorSet->create() || !descriptorSet->writeStorageBuffer(0, VkDescriptorBufferInfo{.buffer = buffer->handle(), .offset = 0, .range = VK_WHOLE_SIZE}) )
			{
				Tracer::error(ClassId, "Unable to create a descriptor set !");

				return false;
			}

			m_buffers.emplace_back(std::move(buffer));
			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
		sets.emplace_back(m_descriptorSetLayout);

		m_pipelineLayout = layoutManager.getPipelineLayout(sets, {
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(PathPushConstants)
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
	PathDebugOverlay::createPipeline (const Framebuffer & framebuffer, uint32_t width, uint32_t height) noexcept
	{
		const auto & renderPass = framebuffer.renderPass();

		auto & shaderManager = m_renderer.shaderManager();

		/* The scene program's ribbon, verbatim (Saphir::PathGLSL). */
		const std::string vertexSource = std::string{BlockDeclarations} + Saphir::PathGLSL::rawFunctions() + VertexMain;
		const std::string fragmentSource = std::string{BlockDeclarations} +
			"layout(location = 0) in vec4 vCoordinates;\n"
			"layout(location = 0) out vec4 outColor;\n\n"
			"void main()\n{\n" +
			Saphir::PathGLSL::roundDiscard("vCoordinates", "pc.style") +
			"	outColor = pc.color;\n"
			"}\n";

		const auto vertexModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "PathDebugOverlayVS", Saphir::ShaderType::VertexShader, vertexSource);
		const auto fragmentModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "PathDebugOverlayFS", Saphir::ShaderType::FragmentShader, fragmentSource);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			Tracer::error(ClassId, "Unable to compile the overlay shaders !");

			return false;
		}

		auto pipeline = std::make_shared< GraphicsPipeline >(m_renderer.device());
		pipeline->setIdentifier(ClassId, "Overlay", "GraphicsPipeline");

		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		/* No vertex input: every vertex is pulled from the point buffer by gl_VertexIndex. */
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

		/* The ribbon faces the eye from either side: no culling. */
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

		/* Always on top: no depth at all. */
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
			Tracer::error(ClassId, "Unable to create the overlay pipeline !");

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
	PathDebugOverlay::upload (uint32_t frameIndex, const std::vector< float > & data) noexcept
	{
		auto & buffer = m_buffers[frameIndex];
		const auto requiredBytes = static_cast< VkDeviceSize >(data.size() * sizeof(float));

		/* Grow the frame's buffer (power of two) and repoint its set: legal, the frame fence guarantees no in-flight use. */
		if ( buffer->bytes() < requiredBytes )
		{
			VkDeviceSize newBytes = buffer->bytes();

			while ( newBytes < requiredBytes )
			{
				newBytes *= 2;
			}

			auto newBuffer = std::make_unique< ShaderStorageBufferObject >(m_renderer.device(), newBytes);

			if ( !newBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to grow a point buffer !");

				return false;
			}

			m_renderer.deferredDestructor().retireObject(std::move(buffer));
			buffer = std::move(newBuffer);

			if ( !m_descriptorSets[frameIndex]->writeStorageBuffer(0, VkDescriptorBufferInfo{.buffer = buffer->handle(), .offset = 0, .range = VK_WHOLE_SIZE}) )
			{
				return false;
			}
		}

		auto * destination = buffer->mapMemoryAs< uint8_t >();

		if ( destination == nullptr )
		{
			return false;
		}

		std::memcpy(destination, data.data(), static_cast< size_t >(requiredBytes));
		buffer->unmapMemory();

		return true;
	}

	void
	PathDebugOverlay::record (const CommandBuffer & commandBuffer, const Framebuffer & framebuffer, uint32_t width, uint32_t height, const Scenes::SceneInstanceTransforms & instanceTransforms, const ViewMatricesInterface & mainViewMatrices, uint32_t readStateIndex, bool sRGBTarget) noexcept
	{
		const auto & paths = instanceTransforms.debugPaths();

		if ( paths.empty() || width == 0 || height == 0 )
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

		/* The header: the camera the points were staged for (unjittered: the final image carries no jitter), the eye, and
		 * THIS target's size in pixels (the pixel mode projects the points onto it). */
		const auto & projection = mainViewMatrices.unjitteredProjectionMatrix(readStateIndex);
		const auto & view = mainViewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & eye = mainViewMatrices.position(readStateIndex);

		const auto & points = instanceTransforms.debugPathPoints();

		m_staging.clear();
		m_staging.reserve(HeaderFloats + points.size() * 8);
		m_staging.insert(m_staging.end(), view.data(), view.data() + 16);
		m_staging.insert(m_staging.end(), projection.data(), projection.data() + 16);
		m_staging.insert(m_staging.end(), {eye[X], eye[Y], eye[Z], 1.0F, static_cast< float >(width), static_cast< float >(height), 0.0F, 0.0F});

		/* {current, previous} pairs, the layout pathPoint() reads: the overlay has no velocity, previous = current. */
		for ( const auto & point : points )
		{
			m_staging.insert(m_staging.end(), {point[X], point[Y], point[Z], point[W], point[X], point[Y], point[Z], point[W]});
		}

		const auto frameIndex = m_renderer.currentFrameIndex() % static_cast< uint32_t >(m_buffers.size());

		if ( !this->upload(frameIndex, m_staging) )
		{
			Tracer::error(ClassId, "Unable to upload the debug paths !");

			return;
		}

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

		commandBuffer.bind(*m_descriptorSets[frameIndex], *m_pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		/* The colour is authored as displayed (sRGB): an sRGB target encodes on write, so it is handed linear. */
		const auto encode = [sRGBTarget] (float value) noexcept {
			return sRGBTarget ? sRGBToLinear(value) : value;
		};

		for ( const auto & path : paths )
		{
			const PathPushConstants pushConstants{
				.span = {path.firstPoint, path.pointCount, 0U, 0U},
				.style = {path.style[X], path.style[Y], path.style[Z], path.style[W]},
				.color = {encode(path.color[X]), encode(path.color[Y]), encode(path.color[Z]), path.color[W]},
				.flags = {0.0F, 0.0F, 0.0F, 0.0F}
			};

			vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

			commandBuffer.draw((path.pointCount - 1) * Saphir::PathGLSL::VerticesPerSegment, 1);
		}
	}

	void
	PathDebugOverlay::destroy () noexcept
	{
		m_pipeline.reset();
		m_pipelineRenderPass = VK_NULL_HANDLE;
		m_pipelineLayout.reset();
		m_descriptorSets.clear();
		m_buffers.clear();
		m_descriptorSetLayout.reset();
		m_sharedResourcesCreated = false;
	}
}
