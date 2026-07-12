/*
 * src/Vulkan/RenderSubPass.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "StaticVector.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The render subpass class to complete a render pass.
	 */
	class EMEN_API RenderSubPass final
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

			/**
			 * @brief Sets a depth/stencil resolve attachment for MSAA depth resolve.
			 * @note Requires Vulkan 1.2+ (VK_KHR_depth_stencil_resolve, core in 1.2).
			 * When configured, RenderPass will use vkCreateRenderPass2() with
			 * VkSubpassDescriptionDepthStencilResolve chained to VkSubpassDescription2.
			 * @param attachment The index of the resolve attachment.
			 * @param layout The image layout for the resolve attachment.
			 * @param depthResolveMode The resolve mode for depth (e.g. VK_RESOLVE_MODE_SAMPLE_ZERO_BIT).
			 * @param stencilResolveMode The resolve mode for stencil. Default VK_RESOLVE_MODE_NONE.
			 * @return void
			 */
			void
			setDepthStencilResolveAttachment (uint32_t attachment, VkImageLayout layout, VkResolveModeFlagBits depthResolveMode, VkResolveModeFlagBits stencilResolveMode = VK_RESOLVE_MODE_NONE) noexcept
			{
				m_depthStencilResolveAttachment.attachment = attachment;
				m_depthStencilResolveAttachment.layout = layout;
				m_depthResolveMode = depthResolveMode;
				m_stencilResolveMode = stencilResolveMode;
				m_depthStencilResolveSet = true;
			}

			/**
			 * @brief Returns whether a depth/stencil resolve attachment is configured.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasDepthStencilResolve () const noexcept
			{
				return m_depthStencilResolveSet;
			}

		private:

			friend class RenderPass;

			VkSubpassDescriptionFlags m_flags;
			VkPipelineBindPoint m_pipelineBindPoint;
			Base::StaticVector< VkAttachmentReference, 8 > m_inputAttachments;
			Base::StaticVector< VkAttachmentReference, 8 > m_colorAttachments;
			Base::StaticVector< VkAttachmentReference, 8 > m_resolveAttachments; // Use the color attachments count.
			VkAttachmentReference m_depthStencilAttachment{};
			Base::StaticVector< uint32_t, 8 > m_preserveAttachments;
			bool m_depthStencilAttachmentSet{false};
			VkAttachmentReference m_depthStencilResolveAttachment{};
			VkResolveModeFlagBits m_depthResolveMode{VK_RESOLVE_MODE_NONE};
			VkResolveModeFlagBits m_stencilResolveMode{VK_RESOLVE_MODE_NONE};
			bool m_depthStencilResolveSet{false};
	};
}
