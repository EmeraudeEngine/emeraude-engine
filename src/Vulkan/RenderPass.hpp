/*
 * src/Vulkan/RenderPass.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The render subpass class to complete a render pass.
	 */
	class RenderSubPass final
	{
		public:

			/**
			 * @brief Construct a render subpass.
			 * @param pipelineBindPoint Set the type of pipeline being bound to this render subpass. Default VK_PIPELINE_BIND_POINT_GRAPHICS.
			 * @param flags Set flags of the subpass. Default none.
			 */
			explicit
			RenderSubPass (VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, VkSubpassDescriptionFlags flags = 0) noexcept
				: m_flags{flags},
				m_pipelineBindPoint{pipelineBindPoint}
			{

			}

			/**
			 * @brief Returns the description for the render pass.
			 * @return VkSubpassDescription
			 */
			[[nodiscard]]
			VkSubpassDescription generateSubPassDescription () const noexcept;

			/**
			 * @brief Adds an input attachment to the subpass.
			 * @param attachment The index of attachment.
			 * @param layout The image layout.
			 * @return void
			 */
			void
			addInputAttachment (uint32_t attachment, VkImageLayout layout) noexcept
			{
				m_inputAttachments.emplace_back(VkAttachmentReference{
					.attachment = attachment,
					.layout = layout
				});
			}

			/**
			 * @brief Adds a color attachment to the subpass.
			 * @param attachment The index of attachment.
			 * @param layout The image layout.
			 * @return void
			 */
			void
			addColorAttachment (uint32_t attachment, VkImageLayout layout) noexcept
			{
				m_colorAttachments.emplace_back(VkAttachmentReference{
					.attachment = attachment,
					.layout = layout
				});
			}

			/**
			 * @brief Adds a resolve attachment to the subpasses.
			 * @warning If there is a resolve attachment, it must be the same count as color attachments.
			 * @param attachment The index of attachment.
			 * @param layout The image layout.
			 * @return void
			 */
			void
			addResolveAttachment (uint32_t attachment, VkImageLayout layout) noexcept
			{
				m_resolveAttachments.emplace_back(VkAttachmentReference{
					.attachment = attachment,
					.layout = layout
				});
			}

			/**
			 * @brief Sets the only possible attachment depth/stencil reference.
			 * @param attachment The index of attachment.
			 * @param layout The image layout.
			 * @return void
			 */
			void
			setDepthStencilAttachment (uint32_t attachment, VkImageLayout layout) noexcept
			{
				m_depthStencilAttachment.attachment = attachment;
				m_depthStencilAttachment.layout = layout;
				m_depthStencilAttachmentSet = true;
			}

			/**
			 * @brief Adds a preserved attachment between subpasses.
			 * @param index An index to the attachment.
			 * @return void
			 */
			void
			addPreserveAttachment (uint32_t index) noexcept
			{
				m_preserveAttachments.emplace_back(index);
			}

		private:

			VkSubpassDescriptionFlags m_flags;
			VkPipelineBindPoint m_pipelineBindPoint;
			Libs::StaticVector< VkAttachmentReference, 8 > m_inputAttachments;
			Libs::StaticVector< VkAttachmentReference, 8 > m_colorAttachments;
			Libs::StaticVector< VkAttachmentReference, 8 > m_resolveAttachments; // Use the color attachments count.
			VkAttachmentReference m_depthStencilAttachment{};
			Libs::StaticVector< uint32_t, 8 > m_preserveAttachments;
			bool m_depthStencilAttachmentSet{false};
	};

	/**
	 * @brief The render-pass class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class RenderPass final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanRenderPass"};

			/**
			 * @brief Constructs a render pass.
			 * @param device A reference to a smart pointer to a device where the render pass will be performed.
			 * @param createFlags The createInfo flags. Default none.
			 */
			explicit
			RenderPass (const std::shared_ptr< Device > & device, VkRenderPassCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{device}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.attachmentCount = 0;
				m_createInfo.pAttachments = nullptr;
				m_createInfo.subpassCount = 0;
				m_createInfo.pSubpasses = nullptr;
				m_createInfo.dependencyCount = 0;
				m_createInfo.pDependencies = nullptr;
			}

			/**
			 * @brief Constructs a render pass with createInfo.
			 * @param device A reference to a smart pointer to a device where the render pass will be performed.
			 * @param createInfo A reference to a createInfo.
			 */
			RenderPass (const std::shared_ptr< Device > & device, const VkRenderPassCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{device},
				m_createInfo{createInfo}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			RenderPass (const RenderPass & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			RenderPass (RenderPass && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			RenderPass & operator= (const RenderPass & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			RenderPass & operator= (RenderPass && copy) noexcept = default;

			/**
			 * @brief Destructs the render pass.
			 */
			~RenderPass () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Adds an attachment description.
			 * @param attachmentDescription A reference to an attachment description.
			 * @return void
			 */
			void
			addAttachmentDescription (const VkAttachmentDescription & attachmentDescription) noexcept
			{
				m_attachmentDescriptions.emplace_back(attachmentDescription);
			}

			/**
			 * @brief Adds a render sub-pass description.
			 * @param subPass A reference to a render subpass structure.
			 * @return void
			 */
			void
			addSubPass (const RenderSubPass & subPass) noexcept
			{
				m_renderSubPasses.emplace_back(subPass);
			}

			/**
			 * @brief Adds a sub-pass dependency.
			 * @param dependency A reference to a subpass dependency.
			 * @return void
			 */
			void
			addSubPassDependency (const VkSubpassDependency & dependency) noexcept
			{
				m_subPassDependencies.emplace_back(dependency);
			}

			/**
			 * @brief Returns the render-pass vulkan handle.
			 * @return VkRenderPass
			 */
			[[nodiscard]]
			VkRenderPass
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the render-pass createInfo.
			 * @return VkRenderPassCreateInfo
			 */
			[[nodiscard]]
			const VkRenderPassCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Enables multiview rendering for cubemap (6 views).
			 * @note This requires Vulkan 1.1+ and multiview feature enabled.
			 * @return void
			 */
			void enableMultiview () noexcept;

			/**
			 * @brief Returns whether multiview is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMultiviewEnabled () const noexcept
			{
				return m_multiviewEnabled;
			}

		private:

			/**
			 * @brief Pack subpass descriptions.
			 * @return Libs::StaticVector< VkSubpassDescription, 4 >
			 */
			[[nodiscard]]
			Libs::StaticVector< VkSubpassDescription, 4 > getSubPassDescriptions () const noexcept;

			VkRenderPass m_handle{VK_NULL_HANDLE};
			VkRenderPassCreateInfo m_createInfo{};
			Libs::StaticVector< VkAttachmentDescription, 8 > m_attachmentDescriptions;
			Libs::StaticVector< RenderSubPass, 4 > m_renderSubPasses;
			Libs::StaticVector< VkSubpassDependency, 8 > m_subPassDependencies;
			VkRenderPassMultiviewCreateInfo m_multiviewCreateInfo{};
			uint32_t m_viewMask{0};
			uint32_t m_correlationMask{0};
			bool m_multiviewEnabled{false};
	};
}
