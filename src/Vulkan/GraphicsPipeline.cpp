/*
 * src/Vulkan/GraphicsPipeline.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "GraphicsPipeline.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Graphics/VertexBufferFormat.hpp"
#include "Graphics/RasterizationOptions.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Device.hpp"
#include "PipelineLayout.hpp"
#include "ShaderModule.hpp"
#include "RenderPass.hpp"
#include "Framebuffer.hpp"
#include "Utility.hpp"
#include "Core.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Saphir;

	size_t GraphicsPipeline::s_fakeHash{0};

	GraphicsPipeline::GraphicsPipeline (const std::shared_ptr< Device > & device, VkPipelineCreateFlags createFlags) noexcept
		: AbstractDeviceDependentObject(device)
	{
		m_createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = createFlags;
		m_createInfo.stageCount = 0;
		m_createInfo.pStages = nullptr;
		m_createInfo.pVertexInputState = nullptr;
		m_createInfo.pInputAssemblyState = nullptr;
		m_createInfo.pTessellationState = nullptr;
		m_createInfo.pViewportState = nullptr;
		m_createInfo.pRasterizationState = nullptr;
		m_createInfo.pMultisampleState = nullptr;
		m_createInfo.pDepthStencilState = nullptr;
		m_createInfo.pColorBlendState = nullptr;
		m_createInfo.pDynamicState = nullptr;
		m_createInfo.layout = VK_NULL_HANDLE;
		m_createInfo.renderPass = VK_NULL_HANDLE;
		m_createInfo.subpass = 0;
		m_createInfo.basePipelineHandle = VK_NULL_HANDLE;
		m_createInfo.basePipelineIndex = -1;
	}

	GraphicsPipeline::GraphicsPipeline (const std::shared_ptr< Device > & device, const VkGraphicsPipelineCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(device), m_createInfo(createInfo)
	{

	}

	GraphicsPipeline::~GraphicsPipeline ()
	{
		this->destroyFromHardware();
	}

	bool
	GraphicsPipeline::configureShaderStages (const std::vector< std::shared_ptr< Vulkan::ShaderModule > > & shaderModules) noexcept
	{
		for ( const auto & shaderModule : shaderModules )
		{
			m_shaderStages.emplace_back(shaderModule->pipelineShaderStageCreateInfo());
		}

		/* Adds to the pipeline createInfo. */
		m_createInfo.stageCount = static_cast< uint32_t >(m_shaderStages.size());
		m_createInfo.pStages = m_shaderStages.data();

		return true;
	}

	bool
	GraphicsPipeline::configureVertexInputState (const VertexBufferFormat & vertexBufferFormat, VkPipelineVertexInputStateCreateFlags flags) noexcept
	{
		const auto & bindings = vertexBufferFormat.bindings();
		const auto & attributes = vertexBufferFormat.attributes();

		if ( bindings.empty() || attributes.empty())
		{
			Tracer::error(ClassId, "There is no binding or vertex attribute !");

			return false;
		}

		m_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_vertexInputState.pNext = nullptr;
		m_vertexInputState.flags = flags;
		m_vertexInputState.vertexBindingDescriptionCount = static_cast< uint32_t >(bindings.size());
		m_vertexInputState.pVertexBindingDescriptions = bindings.data();
		m_vertexInputState.vertexAttributeDescriptionCount = static_cast< uint32_t >(attributes.size());
		m_vertexInputState.pVertexAttributeDescriptions = attributes.data();

		/* Adds to the pipeline createInfo. */
		m_createInfo.pVertexInputState = &m_vertexInputState;

		return true;
	}

	bool
	GraphicsPipeline::configureInputAssemblyState (const VertexBufferFormat & vertexBufferFormat, VkPipelineInputAssemblyStateCreateFlags flags) noexcept
	{
		const auto * binding = vertexBufferFormat.binding(0);

		if ( binding == nullptr )
		{
			Tracer::error(ClassId, "The first binding is a null pointer !");

			return false;
		}

		m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_inputAssemblyState.pNext = nullptr;
		m_inputAssemblyState.flags = flags;

		switch ( binding->topology() )
		{
			case Topology::PointList :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				break;

			case Topology::LineLine :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				break;

			case Topology::LineStrip :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				break;

			case Topology::TriangleList :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				break;

			case Topology::TriangleStrip :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				break;

			case Topology::TriangleFan :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
				break;

			case Topology::LineListWithAdjacency :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
				break;

			case Topology::LineStripWithAdjacency :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
				break;

			case Topology::TriangleListWithAdjacency :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
				break;

			case Topology::TriangleStripWithAdjacency :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
				break;

			case Topology::PatchList :
				m_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
				break;

			case Topology::CustomData :
			case Topology::Invalid :
				return false;
		}

		m_inputAssemblyState.primitiveRestartEnable = binding->requestPrimitiveRestart() ? VK_TRUE : VK_FALSE;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pInputAssemblyState = &m_inputAssemblyState;

		return true;
	}

	bool
	GraphicsPipeline::configureTessellationState (uint32_t patchControlPoints, VkPipelineTessellationStateCreateFlags flags) noexcept
	{
		if ( patchControlPoints == 0 )
		{
			Tracer::error(ClassId, "There is no patch control points for the tesselation state !");

			return false;
		}

		m_tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		m_tessellationState.pNext = nullptr;
		m_tessellationState.flags = flags;
		m_tessellationState.patchControlPoints = patchControlPoints;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pTessellationState = &m_tessellationState;

		return true;
	}

	bool
	GraphicsPipeline::configureViewportState (uint32_t width, uint32_t height, VkPipelineViewportStateCreateFlags flags) noexcept
	{
		if ( width <= 0 || height <= 0 )
		{
			Tracer::error(ClassId, "Invalid view dimensions !");

			return false;
		}

		if ( m_viewports.empty() )
		{
			m_viewports.resize(1);
		}

		m_viewports[0].x = 0.0F;
		m_viewports[0].y = 0.0F;
		m_viewports[0].width = static_cast< float >(width);
		m_viewports[0].height = static_cast< float >(height);
		m_viewports[0].minDepth = 0.0F;
		m_viewports[0].maxDepth = 1.0F;

		if ( m_scissors.empty() )
		{
			m_scissors.resize(1);
		}

		m_scissors[0].offset = {0, 0};
		m_scissors[0].extent.width = width;
		m_scissors[0].extent.height = height;

		m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_viewportState.pNext = nullptr;
		m_viewportState.flags = flags;
		m_viewportState.viewportCount = static_cast< uint32_t >(m_viewports.size());
		m_viewportState.pViewports = m_viewports.data();
		m_viewportState.scissorCount = static_cast< uint32_t >(m_scissors.size());
		m_viewportState.pScissors = m_scissors.data();

		/* Adds to the pipeline createInfo. */
		m_createInfo.pViewportState = &m_viewportState;

		return true;
	}

	bool
	GraphicsPipeline::configureRasterizationState (RenderPassType renderPassType, const RasterizationOptions * options, VkPipelineRasterizationStateCreateFlags flags) noexcept
	{
		m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_rasterizationState.pNext = nullptr;
		m_rasterizationState.flags = flags;
		m_rasterizationState.depthClampEnable = VK_FALSE;
		m_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		if ( options != nullptr )
		{
			switch ( options->polygonMode() )
			{
				case PolygonMode::Fill :
				case PolygonMode::Invalid :
					m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
					break;

				case PolygonMode::Line :
					m_rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
					break;

				case PolygonMode::Point :
					m_rasterizationState.polygonMode = VK_POLYGON_MODE_POINT;
					break;

				case PolygonMode::FillRectangle :
					m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL_RECTANGLE_NV;
					break;
			}

			switch ( options->cullingMode() )
			{
				case CullingMode::None :
					m_rasterizationState.cullMode = VK_CULL_MODE_NONE;
					break;

				case CullingMode::Front :
					m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
					break;

				case CullingMode::Back :
				case CullingMode::Invalid :
					m_rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
					break;

				case CullingMode::Both :
					m_rasterizationState.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
					break;
			}

			m_rasterizationState.frontFace = options->isTriangleClockwise() ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
		else
		{
			m_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
			m_rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
			m_rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}

		switch ( renderPassType )
		{
			case RenderPassType::DirectionalLightPass :
			case RenderPassType::DirectionalLightPassNoShadow :
			case RenderPassType::PointLightPass :
			case RenderPassType::PointLightPassNoShadow :
			case RenderPassType::SpotLightPass :
			case RenderPassType::SpotLightPassNoShadow :
				m_rasterizationState.depthBiasEnable = VK_TRUE;
				m_rasterizationState.depthBiasConstantFactor = -4.0F;
				m_rasterizationState.depthBiasClamp = 0.0F;
				m_rasterizationState.depthBiasSlopeFactor = 0.0F;
				break;

			default :
				m_rasterizationState.depthBiasEnable = VK_FALSE;
				m_rasterizationState.depthBiasConstantFactor = 0.0F;
				m_rasterizationState.depthBiasClamp = 0.0F;
				m_rasterizationState.depthBiasSlopeFactor = 0.0F;
				break;
		}

		m_rasterizationState.lineWidth = 1.0F;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pRasterizationState = &m_rasterizationState;

		return true;
	}

	bool
	GraphicsPipeline::configureRasterizationState (VkPipelineRasterizationStateCreateInfo createInfo) noexcept
	{
		m_rasterizationState = createInfo;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pRasterizationState = &m_rasterizationState;

		return true;
	}

	bool
	GraphicsPipeline::configureMultisampleState (const RenderTarget::Abstract & renderTarget, VkPipelineMultisampleStateCreateFlags flags) noexcept
	{
		const auto samples = renderTarget.precisions().samples();

		if ( samples < 1 )
		{
			TraceWarning{ClassId} << "No samples is requested from the render target '" << renderTarget.id() << "' !";

			return false;
		}

		m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_multisampleState.pNext = nullptr;
		m_multisampleState.flags = flags;
		m_multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		m_multisampleState.sampleShadingEnable = VK_FALSE;
		m_multisampleState.minSampleShading = 1.0F;
		m_multisampleState.pSampleMask = nullptr;
		m_multisampleState.alphaToCoverageEnable = VK_FALSE;
		m_multisampleState.alphaToOneEnable = VK_FALSE;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pMultisampleState = &m_multisampleState;

		return true;
	}

	bool
	GraphicsPipeline::configureDepthStencilState (RenderPassType renderPassType, const RenderableInstance::Abstract & renderableInstance, VkPipelineDepthStencilStateCreateFlags flags) noexcept
	{
		// FIXME: Check the render pass type to influence depth test (light code)
		// TODO: Disable depth testing/writing on light code.

		m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_depthStencilState.pNext = nullptr;
		m_depthStencilState.flags = flags;

		switch ( renderPassType )
		{
			case RenderPassType::DirectionalLightPass :
			case RenderPassType::DirectionalLightPassNoShadow :
			case RenderPassType::PointLightPass :
			case RenderPassType::PointLightPassNoShadow :
			case RenderPassType::SpotLightPass :
			case RenderPassType::SpotLightPassNoShadow :
				m_depthStencilState.depthTestEnable = VK_TRUE;
				m_depthStencilState.depthWriteEnable = VK_FALSE;
				m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
				break;

			default:
				m_depthStencilState.depthTestEnable = renderableInstance.isDepthTestDisabled() ? VK_FALSE : VK_TRUE;
				m_depthStencilState.depthWriteEnable = renderableInstance.isDepthWriteDisabled() ? VK_FALSE : VK_TRUE;
				m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
				break;
		}

		m_depthStencilState.stencilTestEnable = VK_FALSE;
		/*

		m_depthStencilState.front.failOp = VK_STENCIL_OP_ZERO;
		m_depthStencilState.front.passOp = VK_STENCIL_OP_KEEP;
		m_depthStencilState.front.depthFailOp = VK_STENCIL_OP_ZERO;
		m_depthStencilState.front.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_depthStencilState.front.compareMask = 0;
		m_depthStencilState.front.writeMask = 0;
		m_depthStencilState.front.reference = 0;

		m_depthStencilState.back.failOp = VK_STENCIL_OP_ZERO;
		m_depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
		m_depthStencilState.back.depthFailOp = VK_STENCIL_OP_ZERO;
		m_depthStencilState.back.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		m_depthStencilState.back.compareMask = 0;
		m_depthStencilState.back.writeMask = 0;
		m_depthStencilState.back.reference = 0;

		*/
		m_depthStencilState.minDepthBounds = 0.0F;
		m_depthStencilState.maxDepthBounds = 1.0F;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pDepthStencilState = &m_depthStencilState;

		return true;
	}

	bool
	GraphicsPipeline::configureDepthStencilState (const VkPipelineDepthStencilStateCreateInfo & createInfo) noexcept
	{
		m_depthStencilState = createInfo;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pDepthStencilState = &m_depthStencilState;

		return true;
	}

	void
	GraphicsPipeline::defaultColorBlendState () noexcept
	{
		m_colorBlendAttachments.clear();

		m_colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,

			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,

			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,

			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		});

		m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_colorBlendState.pNext = nullptr;
		m_colorBlendState.flags = 0;
		m_colorBlendState.logicOpEnable = VK_FALSE;
		m_colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		m_colorBlendState.attachmentCount = static_cast< uint32_t >(m_colorBlendAttachments.size());
		m_colorBlendState.pAttachments = m_colorBlendAttachments.data();
		m_colorBlendState.blendConstants[0] = 0.0F;
		m_colorBlendState.blendConstants[1] = 0.0F;
		m_colorBlendState.blendConstants[2] = 0.0F;
		m_colorBlendState.blendConstants[3] = 1.0F;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pColorBlendState = &m_colorBlendState;
	}

	bool
	GraphicsPipeline::configureColorBlendStateForAlphaBlending () noexcept
	{
		m_colorBlendAttachments.clear();

		m_colorBlendAttachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_TRUE,

			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,

			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,

			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		});

		m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_colorBlendState.pNext = nullptr;
		m_colorBlendState.flags = 0;
		m_colorBlendState.logicOpEnable = VK_FALSE;
		m_colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		m_colorBlendState.attachmentCount = static_cast< uint32_t >(m_colorBlendAttachments.size());
		m_colorBlendState.pAttachments = m_colorBlendAttachments.data();
		m_colorBlendState.blendConstants[0] = 0.0F;
		m_colorBlendState.blendConstants[1] = 0.0F;
		m_colorBlendState.blendConstants[2] = 0.0F;
		m_colorBlendState.blendConstants[3] = 1.0F;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pColorBlendState = &m_colorBlendState;

		return true;
	}

	bool
	GraphicsPipeline::configureColorBlendState (RenderPassType renderPassType, const Material::Interface & material, const PixelFactory::Color< float > & blendColor, VkPipelineColorBlendStateCreateFlags flags) noexcept
	{
		if ( m_colorBlendAttachments.empty() )
		{
			m_colorBlendAttachments.resize(1);

			switch ( renderPassType )
			{
				/* NOTE : This will perform a light color addition over the ambient pass. */
				case RenderPassType::DirectionalLightPass :
				case RenderPassType::DirectionalLightPassNoShadow :
				case RenderPassType::PointLightPass :
				case RenderPassType::PointLightPassNoShadow :
				case RenderPassType::SpotLightPass :
				case RenderPassType::SpotLightPassNoShadow :
					m_colorBlendAttachments[0].blendEnable = VK_TRUE;

					m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
					m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
					m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
					break;

				default:
					if ( material.isOpaque() )
					{
						m_colorBlendAttachments[0].blendEnable = VK_FALSE;

						m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
						m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
						m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;

						m_colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
						m_colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
						m_colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
					}
					else
					{
						m_colorBlendAttachments[0].blendEnable = VK_TRUE;

						switch ( material.blendingMode() )
						{
							case BlendingMode::Add :
								m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
								m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
								m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;

								m_colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
								m_colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
								m_colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
								break;

							case BlendingMode::Multiply :
								m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
								m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
								m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;

								m_colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
								m_colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
								m_colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
								break;

							case BlendingMode::Screen :
								m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
								m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
								m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;

								m_colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
								m_colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
								m_colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
								break;

							/* Simple alpha blending. */
							case BlendingMode::Normal :
							default:
								m_colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
								m_colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
								m_colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;

								m_colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
								m_colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
								m_colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
								break;
						}


					}
					break;
			}

			m_colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_colorBlendState.pNext = nullptr;
		m_colorBlendState.flags = flags;
		/* NOTE: The 'logicOp' feature is requested. */
		m_colorBlendState.logicOpEnable = VK_FALSE;
		m_colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		m_colorBlendState.attachmentCount = static_cast< uint32_t >(m_colorBlendAttachments.size());
		m_colorBlendState.pAttachments = m_colorBlendAttachments.data();
		m_colorBlendState.blendConstants[0] = blendColor.red();
		m_colorBlendState.blendConstants[1] = blendColor.green();
		m_colorBlendState.blendConstants[2] = blendColor.blue();
		m_colorBlendState.blendConstants[3] = blendColor.alpha();

		/* Adds to the pipeline createInfo. */
		m_createInfo.pColorBlendState = &m_colorBlendState;

		return true;
	}

	bool
	GraphicsPipeline::configureColorBlendState (const std::vector< VkPipelineColorBlendAttachmentState > & attachments, const VkPipelineColorBlendStateCreateInfo & createInfo) noexcept
	{
		m_colorBlendAttachments = attachments;
		m_colorBlendState = createInfo;

		/* Adds to the pipeline createInfo. */
		m_createInfo.pColorBlendState = &m_colorBlendState;

		return true;
	}

	bool
	GraphicsPipeline::configureDynamicStates (const std::vector< VkDynamicState > & dynamicStates, VkPipelineDynamicStateCreateFlags flags) noexcept
	{
		if ( dynamicStates.empty() )
		{
			return false;
		}

		/* Keep the states locally. */
		m_dynamicStates = dynamicStates;

		m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_dynamicState.pNext = nullptr;
		m_dynamicState.flags = flags;
		m_dynamicState.dynamicStateCount = static_cast< uint32_t >(m_dynamicStates.size());
		m_dynamicState.pDynamicStates = m_dynamicStates.data();

		/* Adds to the pipeline createInfo. */
		m_createInfo.pDynamicState = &m_dynamicState;

		return true;
	}

	bool
	GraphicsPipeline::hasDynamicState (VkDynamicState state) const noexcept
	{
		if ( m_dynamicStates.empty() )
		{
			return false;
		}

		return std::ranges::any_of(m_dynamicStates, [state] (const auto & item) {
			return item == state;
		});
	}

	bool
	GraphicsPipeline::finalize (const std::shared_ptr< const RenderPass > & renderPass, const std::shared_ptr< PipelineLayout > & pipelineLayout, bool useTesselation) noexcept
	{
		if ( renderPass == nullptr )
		{
			TraceError{ClassId} << "The render pass is a null pointer !";

			return false;
		}

		if ( m_createInfo.stageCount == 0 || m_createInfo.pStages == nullptr )
		{
			Tracer::error(ClassId, "There is no stage to build the graphics pipeline properly !");

			return false;
		}

		/* NOTE: It can be NULL if the pipeline is created with the VK_DYNAMIC_STATE_VERTEX_INPUT_EXT dynamic state set. */
		if ( m_createInfo.pVertexInputState == nullptr && !this->hasDynamicState(VK_DYNAMIC_STATE_VERTEX_INPUT_EXT) )
		{
			Tracer::error(ClassId, "There is no vertex input state to build the graphics pipeline properly !");

			return false;
		}

		/* NOTE: it can be NULL if the pipeline is created with both VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE,
		 * and VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY dynamic states set and dynamicPrimitiveTopologyUnrestricted is VK_TRUE.
		 * It is ignored if the pipeline includes a mesh shader stage (VK_EXT_mesh_shader). */
		if ( m_createInfo.pInputAssemblyState == nullptr && !this->hasDynamicState(VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE) && !this->hasDynamicState(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY) )
		{
			Tracer::error(ClassId, "There is no input assembly state to build the graphics pipeline properly !");

			return false;
		}

		/* NOTE: It can be NULL if the pipeline is created with the VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT dynamic state set.
		 * Or no tessellation shader present. */
		if ( useTesselation )
		{
			if ( m_createInfo.pTessellationState == nullptr && !this->hasDynamicState(VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT) )
			{
				Tracer::error(ClassId, "There is no tesselation state to build the graphics pipeline properly !");

				return false;
			}
		}

		/* NOTE: it can be NULL if the pipeline is created with both VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
		 * and VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT dynamic states set. */
		if ( m_createInfo.pViewportState == nullptr && !this->hasDynamicState(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT) && !this->hasDynamicState(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT) )
		{
			Tracer::error(ClassId, "There is no viewport state to build the graphics pipeline properly !");

			return false;
		}

		const auto dynamicStateExtended = Core::instance()->vulkanInstance().isDynamicStateExtensionEnabled();

		/* NOTE: If the VK_EXT_extended_dynamic_state3 extension is enabled, it can be NULL if the pipeline is created with all of
		 * VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT, VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE, VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
		 * VK_DYNAMIC_STATE_CULL_MODE, VK_DYNAMIC_STATE_FRONT_FACE, VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE, VK_DYNAMIC_STATE_DEPTH_BIAS,
		 * and VK_DYNAMIC_STATE_LINE_WIDTH dynamic states set. */
		if ( m_createInfo.pRasterizationState == nullptr )
		{
			if ( !dynamicStateExtended || (
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_POLYGON_MODE_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_CULL_MODE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_FRONT_FACE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH)
			))
			{
				Tracer::error(ClassId, "There is no rasterization state to build the graphics pipeline properly !");

				return false;
			}
		}

		/* NOTE: If the VK_EXT_extended_dynamic_state3 extension is enabled, it can be NULL if the pipeline is created with all of
		 * VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT, VK_DYNAMIC_STATE_SAMPLE_MASK_EXT, and VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT
		 * dynamic states set, and either alphaToOne is disabled on the device or VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT is set,
		 * in which case VkPipelineMultisampleStateCreateInfo::sampleShadingEnable is assumed to be VK_FALSE. */
		if ( m_createInfo.pMultisampleState == nullptr )
		{
			if ( !dynamicStateExtended || (
				!this->hasDynamicState(VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_SAMPLE_MASK_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_ALPHA_TO_ONE_ENABLE_EXT)
			))
			{
				Tracer::error(ClassId, "There is no multisample state to build the graphics pipeline properly !");

				return false;
			}
		}

		/* NOTE: If the VK_EXT_extended_dynamic_state3 extension is enabled, it can be NULL if the pipeline is created with all of
		 * VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
		 * VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE, VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE, VK_DYNAMIC_STATE_STENCIL_OP,
		 * and VK_DYNAMIC_STATE_DEPTH_BOUNDS dynamic states set. */
		if ( m_createInfo.pDepthStencilState == nullptr )
		{
			if ( !dynamicStateExtended || (
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_STENCIL_OP) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_DEPTH_BOUNDS)
			))
			{
				Tracer::error(ClassId, "There is no depth/stencil state to build the graphics pipeline properly !");

				return false;
			}
		}

		/* NOTE: If the VK_EXT_extended_dynamic_state3 extension is enabled, it can be NULL if the pipeline is created with all of
		 * VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT, VK_DYNAMIC_STATE_LOGIC_OP_EXT, VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT,
		 * VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT, VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT, and VK_DYNAMIC_STATE_BLEND_CONSTANTS
		 * dynamic states set. */
		if ( m_createInfo.pColorBlendState == nullptr )
		{
			if ( !dynamicStateExtended || (
				!this->hasDynamicState(VK_DYNAMIC_STATE_LOGIC_OP_ENABLE_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_LOGIC_OP_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_COLOR_WRITE_MASK_EXT) &&
				!this->hasDynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
			))
			{
				this->defaultColorBlendState();
			}
		}

		/* NOTE: This can be NULL, which means no state in the pipeline is considered dynamic. */
		//m_createInfo.pDynamicState = nullptr;

		m_createInfo.layout = pipelineLayout->handle();
		m_createInfo.renderPass = renderPass->handle();

		return this->createOnHardware();
	}

	bool
	GraphicsPipeline::recreateOnHardware (const RenderTarget::Abstract & renderTarget, uint32_t width, uint32_t height) noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this graphics pipeline !");

			return false;
		}

		if ( !renderTarget.isValid() )
		{
			TraceError{ClassId} << "The renderable target '" << renderTarget.id() << "' is not valid !";

			return false;
		}

		if ( !this->configureViewportState(width, height) )
		{
			Tracer::error(ClassId, "Unable to configure viewport state !");

			return false;
		}

		m_createInfo.renderPass = renderTarget.framebuffer()->renderPass()->handle();

		return this->createOnHardware();
	}

	bool
	GraphicsPipeline::recreateOnHardware (const RenderTarget::Abstract & renderTarget, const RenderableInstance::Abstract & /*renderableInstance*/) noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this graphics pipeline !");

			return false;
		}

		if ( !renderTarget.isValid() )
		{
			TraceError{ClassId} << "The renderable target '" << renderTarget.id() << "' is not valid !";

			return false;
		}

		const auto & extent = renderTarget.extent();

		if ( !this->configureViewportState(extent.width, extent.height) )
		{
			Tracer::error(ClassId, "Unable to configure viewport state !");

			return false;
		}

		/*if ( !this->configureDepthStencilState(renderableInstance) )
		{
			Tracer::error(ClassId, "Unable to configure depth/stencil state !");

			return false;
		}*/

		m_createInfo.renderPass = renderTarget.framebuffer()->renderPass()->handle();

		return this->createOnHardware();
	}

	bool
	GraphicsPipeline::createOnHardware () noexcept
	{
		const auto result = vkCreateGraphicsPipelines(
			this->device()->handle(),
			VK_NULL_HANDLE,
			1, &m_createInfo,
			nullptr,
			&m_handle
		);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a graphics pipeline : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	GraphicsPipeline::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the graphics pipeline " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != nullptr )
		{
			this->device()->waitIdle("Destroying a graphics pipeline");

			vkDestroyPipeline(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}

	size_t
	GraphicsPipeline::getHash () noexcept
	{
		/* FIXME: Weak and unstable hash method !! */
		return s_fakeHash++;
	}
}
