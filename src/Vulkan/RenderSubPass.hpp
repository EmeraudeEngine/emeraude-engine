/*
 * src/Vulkan/RenderSubPass.hpp
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
#include <cstdint>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Vulkan
{
	/**
	 * @brief The render sub-pass class to complete a render pass.
	 */
	class RenderSubPass final
	{
		public:

			/**
			 * @brief Construct a render subpass.
			 * @param pipelineBindPoint Set the type of pipeline being bound to this render sub-pass. Default VK_PIPELINE_BIND_POINT_GRAPHICS.
			 * @param flags Set flags of the sub-pass. Default none.
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
			 * @brief Adds an input attachment to the sub-pass.
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
			 * @brief Adds a color attachment to the sub-pass.
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
			 * @brief Adds a resolve attachment to the sub-passes.
			 * @warning If there is resolve attachment, it must be the same count as color attachments.
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
			 * @brief Adds a preserved attachment between sub-passes.
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
			std::vector< VkAttachmentReference > m_inputAttachments;
			std::vector< VkAttachmentReference > m_colorAttachments;
			std::vector< VkAttachmentReference > m_resolveAttachments; // Use the color attachments count.
			VkAttachmentReference m_depthStencilAttachment{};
			std::vector< uint32_t > m_preserveAttachments;
			bool m_depthStencilAttachmentSet{false};
	};
}
