/*
 * src/Vulkan/RenderPass.hpp
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

/* STL inclusions. */
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"

/* Local inclusions for usages. */
#include "RenderSubPass.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The render-pass class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class EMEN_API RenderPass final : public AbstractDeviceDependentObject
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
			 * @brief Enables multiview rendering with a custom number of views.
			 * @note This requires Vulkan 1.1+ and multiview feature enabled.
			 * @param viewCount The number of views (1-32, limited by implementation).
			 * @return void
			 */
			void enableMultiview (uint32_t viewCount) noexcept;

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

			/**
			 * @brief Returns the number of color attachments in a given subpass.
			 * @param subPassIndex The subpass index to query. Default 0.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			colorAttachmentCount (uint32_t subPassIndex = 0) const noexcept
			{
				if ( subPassIndex >= m_renderSubPasses.size() )
				{
					return 0;
				}

				return static_cast< uint32_t >(m_renderSubPasses[subPassIndex].m_colorAttachments.size());
			}

		private:

			/**
			 * @brief Pack subpass descriptions.
			 * @return Base::StaticVector< VkSubpassDescription, 4 >
			 */
			[[nodiscard]]
			Base::StaticVector< VkSubpassDescription, 4 > getSubPassDescriptions () const noexcept;

			/**
			 * @brief Returns whether any subpass requires depth/stencil resolve (Vulkan 1.2+).
			 * @return bool
			 */
			[[nodiscard]]
			bool needsRenderPass2 () const noexcept;

			/**
			 * @brief Creates the render pass using vkCreateRenderPass2() for depth/stencil resolve.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOnHardwareV2 () noexcept;

			VkRenderPass m_handle{VK_NULL_HANDLE};
			VkRenderPassCreateInfo m_createInfo{};
			Base::StaticVector< VkAttachmentDescription, 8 > m_attachmentDescriptions;
			Base::StaticVector< RenderSubPass, 4 > m_renderSubPasses;
			Base::StaticVector< VkSubpassDependency, 8 > m_subPassDependencies;
			VkRenderPassMultiviewCreateInfo m_multiviewCreateInfo{};
			uint32_t m_viewMask{0};
			uint32_t m_correlationMask{0};
			bool m_multiviewEnabled{false};
	};
}
