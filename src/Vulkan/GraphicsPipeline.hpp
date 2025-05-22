/*
 * src/Vulkan/GraphicsPipeline.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Graphics/Types.hpp"
#include "RenderPass.hpp"
#include "PipelineLayout.hpp"
#include "ShaderModule.hpp"

/* Forward declarations */
namespace EmEn::Graphics
{
	namespace Material
	{
		class Interface;
	}

	namespace RenderTarget
	{
		class Abstract;
	}

	namespace RenderableInstance
	{
		class Abstract;
	}

	class RasterizationOptions;
	class VertexBufferFormat;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The graphics pipeline class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This Vulkan Object needs a device.
	 */
	class GraphicsPipeline final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanGraphicsPipeline"};

			/**
			 * @brief Constructs a graphics pipeline.
			 * @param device A reference to a device smart pointer.
			 * @param createFlags The createInfo flags. Default none.
			 */
			explicit
			GraphicsPipeline (const std::shared_ptr< Device > & device, VkPipelineCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{device}
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

			/**
			 * @brief Constructs a graphics pipeline with a createInfo.
			 * @param device A reference to a device smart pointer.
			 * @param createInfo A reference to a createInfo.
			 */
			GraphicsPipeline (const std::shared_ptr< Device > & device, const VkGraphicsPipelineCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			GraphicsPipeline (const GraphicsPipeline & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			GraphicsPipeline (GraphicsPipeline && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return GraphicsPipeline &
			 */
			GraphicsPipeline & operator= (const GraphicsPipeline & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return GraphicsPipeline &
			 */
			GraphicsPipeline & operator= (GraphicsPipeline && copy) noexcept = delete;

			/**
			 * @brief Destructs the graphics pipeline.
			 */
			~GraphicsPipeline () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Configures the shader stages of the pipeline.
			 * @param shaderModules A reference to a shader module smart pointer list.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureShaderStages (const std::vector< std::shared_ptr< Vulkan::ShaderModule > > & shaderModules) noexcept;

			/**
			 * @brief Generates vertex input state into the graphics pipeline createInfo.
			 * @param vertexBufferFormat A reference to a vertex buffer format.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureVertexInputState (const Graphics::VertexBufferFormat & vertexBufferFormat, VkPipelineVertexInputStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates input assembly state into the graphics pipeline createInfo.
			 * @param vertexBufferFormat A reference to a vertex buffer format.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureInputAssemblyState (const Graphics::VertexBufferFormat & vertexBufferFormat, VkPipelineInputAssemblyStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates tesselation state into the graphics pipeline createInfo.
			 * @param flags Flags value for this stage. Default 0.
			 * @param patchControlPoints
			 * @return bool
			 */
			[[nodiscard]]
			bool configureTessellationState (uint32_t patchControlPoints, VkPipelineTessellationStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates viewport state into the graphics pipeline createInfo.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureViewportState (uint32_t width, uint32_t height, VkPipelineViewportStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates rasterization state into the graphics pipeline createInfo.
			 * @param renderPassType The render pass type.
			 * @param options A pointer to rasterization options. Default none.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureRasterizationState (Graphics::RenderPassType renderPassType, const Graphics::RasterizationOptions * options = nullptr, VkPipelineRasterizationStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates rasterization state into the graphics pipeline createInfo.
			 * @param createInfo A reference to a VkPipelineRasterizationStateCreateInfo
			 * @return bool
			 */
			[[nodiscard]]
			bool configureRasterizationState (VkPipelineRasterizationStateCreateInfo createInfo) noexcept;

			/**
			 * @brief Generates multisample state into the graphics pipeline createInfo.
			 * @param renderTarget A reference to the render target.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureMultisampleState (const Graphics::RenderTarget::Abstract & renderTarget, VkPipelineMultisampleStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Generates depth stencil state into the graphics pipeline createInfo.
			 * @param renderPassType The render pass type.
			 * @param renderableInstance A reference to the renderable instance.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureDepthStencilState (Graphics::RenderPassType renderPassType, const Graphics::RenderableInstance::Abstract & renderableInstance, VkPipelineDepthStencilStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Sets a custom depth/stencil state;
			 * @param createInfo A reference to a VkPipelineDepthStencilStateCreateInfo
			 * @return bool
			 */
			[[nodiscard]]
			bool configureDepthStencilState (const VkPipelineDepthStencilStateCreateInfo & createInfo) noexcept;

			/**
			 * @brief Generates color blend state into the graphics pipeline createInfo for simple alpha blending.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureColorBlendStateForAlphaBlending () noexcept;

			/**
			 * @brief Generates color blend state into the graphics pipeline createInfo.
			 * @param renderPassType The render pass type.
			 * @param material A reference to the material.
			 * @param blendColor A reference to a color for blend operation. Default Black.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureColorBlendState (Graphics::RenderPassType renderPassType, const Graphics::Material::Interface & material, const Libs::PixelFactory::Color< float > & blendColor = Libs::PixelFactory::Black, VkPipelineColorBlendStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Sets a custom color blend state;
			 * @param attachments A reference to an attachment blend operation list.
			 * @param createInfo A reference to a VkPipelineDepthStencilStateCreateInfo
			 * @return bool
			 */
			[[nodiscard]]
			bool configureColorBlendState (const std::vector< VkPipelineColorBlendAttachmentState > & attachments, const VkPipelineColorBlendStateCreateInfo & createInfo) noexcept;

			/**
			 * @brief Generates dynamic state into the graphics pipeline createInfo.
			 * @param dynamicStates A vector of dynamic states desired.
			 * @param flags Flags value for this stage. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool configureDynamicStates (const std::vector< VkDynamicState > & dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0) noexcept;

			/**
			 * @brief Finalizes the configuration of the graphics pipeline.
			 * @note This should be done by the renderer.
			 * @param renderPass A reference to a render pass smart pointer.
			 * @param pipelineLayout A reference to a pipeline layout smart pointer.
			 * @param useTesselation Declares tesselation was enabled.
			 * @param isDynamicStateEnabled
			 * @return bool
			 */
			[[nodiscard]]
			bool finalize (const std::shared_ptr< const RenderPass > & renderPass, const std::shared_ptr< PipelineLayout > & pipelineLayout, bool useTesselation, bool isDynamicStateEnabled) noexcept;

			/**
			 * @brief Recreates the graphics pipeline.
			 * @todo Check a better version here !
			 * @param renderTarget A reference to a render target to query the viewport.
			 * @param width The width of the viewport.
			 * @param height The height of the viewport.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateOnHardware (const Graphics::RenderTarget::Abstract & renderTarget, uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Recreates the graphics pipeline for a specific renderable instance.
			 * @todo Check this version !
			 * @param renderTarget A reference to a render target to query the viewport.
			 * @param renderableInstance A reference to the renderable instance to query options.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateOnHardware (const Graphics::RenderTarget::Abstract & renderTarget, const Graphics::RenderableInstance::Abstract & renderableInstance) noexcept;

			/**
			 * @brief Returns the pipeline vulkan handle.
			 * @return VkPipeline
			 */
			[[nodiscard]]
			VkPipeline
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the list of color blend attachments.
			 * @return const std::vector< VkPipelineColorBlendAttachmentState > &
			 */
			[[nodiscard]]
			const std::vector< VkPipelineColorBlendAttachmentState > &
			colorBlendAttachments () const noexcept
			{
				return m_colorBlendAttachments;
			}

			/**
			 * @brief Returns a hash for this graphics pipeline according to constructor params.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getHash () noexcept;

		private:

			/**
			 * @brief Returns whether a dynamic state has been enabled.
			 * @param state The dynamic state to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool hasDynamicState (VkDynamicState state) const noexcept;

			/**
			 * @brief Configures a default color blend state.
			 * @return void
			 */
			void defaultColorBlendState () noexcept;

			/* FIXME: Remove this !!!! */
			static size_t s_fakeHash;

			VkPipeline m_handle{VK_NULL_HANDLE};
			VkGraphicsPipelineCreateInfo m_createInfo{};
			std::vector< VkPipelineShaderStageCreateInfo > m_shaderStages;
			VkPipelineVertexInputStateCreateInfo m_vertexInputState{};
			VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState{};
			VkPipelineTessellationStateCreateInfo m_tessellationState{};
			std::vector< VkViewport > m_viewports;
			std::vector< VkRect2D > m_scissors;
			VkPipelineViewportStateCreateInfo m_viewportState{};
			VkPipelineRasterizationStateCreateInfo m_rasterizationState{};
			VkPipelineMultisampleStateCreateInfo m_multisampleState{};
			VkPipelineDepthStencilStateCreateInfo m_depthStencilState{};
			std::vector< VkPipelineColorBlendAttachmentState > m_colorBlendAttachments;
			VkPipelineColorBlendStateCreateInfo m_colorBlendState{};
			std::vector< VkDynamicState > m_dynamicStates;
			VkPipelineDynamicStateCreateInfo m_dynamicState{};
	};
}
