/*
 * src/Vulkan/RenderPass.cpp
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

#include "RenderPass.hpp"

/* STL inclusions. */
#include <cstdint>
#include <algorithm>

/* Local inclusions. */
#include "Device.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	VkSubpassDescription
	RenderSubPass::generateSubPassDescription () const noexcept
	{
		VkSubpassDescription description{};
		description.flags = m_flags;
		description.pipelineBindPoint = m_pipelineBindPoint;

		if ( m_inputAttachments.empty() )
		{
			description.inputAttachmentCount = 0;
			description.pInputAttachments = nullptr;
		}
		else
		{
			description.inputAttachmentCount = static_cast< uint32_t >(m_inputAttachments.size());
			description.pInputAttachments = m_inputAttachments.data();
		}

		if ( m_colorAttachments.empty() )
		{
			description.colorAttachmentCount = 0;
			description.pColorAttachments = nullptr;
		}
		else
		{
			description.colorAttachmentCount = static_cast< uint32_t >(m_colorAttachments.size());
			description.pColorAttachments = m_colorAttachments.data();
		}

		if ( m_resolveAttachments.empty() )
		{
			description.pResolveAttachments = nullptr;
		}
		else
		{
			/* FIXME: Check if vulkan authorize resolve attachments without color. */
			if ( m_colorAttachments.empty() )
			{
				description.colorAttachmentCount = static_cast< uint32_t >(m_resolveAttachments.size());
			}

			description.pResolveAttachments = m_resolveAttachments.data();
		}

		if ( m_depthStencilAttachmentSet )
		{
			description.pDepthStencilAttachment = &m_depthStencilAttachment;
		}

		if ( m_preserveAttachments.empty() )
		{
			description.preserveAttachmentCount = 0;
			description.pPreserveAttachments = nullptr;
		}
		else
		{
			description.preserveAttachmentCount = static_cast< uint32_t >(m_preserveAttachments.size());
			description.pPreserveAttachments = m_preserveAttachments.data();
		}

		return description;
	}

	void
	RenderPass::enableMultiview () noexcept
	{
		/* Configure for 6 views (cubemap faces) */
		this->enableMultiview(6);
	}

	void
	RenderPass::enableMultiview (uint32_t viewCount) noexcept
	{
		/* Create a bitmask for the specified number of views.
		 * For viewCount=4: mask = 0b00001111 (4 bits set)
		 * For viewCount=6: mask = 0b00111111 (6 bits set) */
		m_viewMask = (1U << viewCount) - 1;
		m_correlationMask = m_viewMask;

		m_multiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		m_multiviewCreateInfo.pNext = nullptr;
		m_multiviewCreateInfo.subpassCount = 1;
		m_multiviewCreateInfo.pViewMasks = &m_viewMask;
		m_multiviewCreateInfo.dependencyCount = 0;
		m_multiviewCreateInfo.pViewOffsets = nullptr;
		m_multiviewCreateInfo.correlationMaskCount = 1;
		m_multiviewCreateInfo.pCorrelationMasks = &m_correlationMask;

		// Chain into pNext
		m_multiviewCreateInfo.pNext = m_createInfo.pNext;
		m_createInfo.pNext = &m_multiviewCreateInfo;
		m_multiviewEnabled = true;
	}

	StaticVector< VkSubpassDescription, 4 >
	RenderPass::getSubPassDescriptions () const noexcept
	{
		StaticVector< VkSubpassDescription, 4 > descriptions;

		std::ranges::transform(m_renderSubPasses, std::back_inserter(descriptions), [] (const auto & renderSubPass) {
			return renderSubPass.generateSubPassDescription();
		});

		return descriptions;
	}

	bool
	RenderPass::needsRenderPass2 () const noexcept
	{
		return std::ranges::any_of(m_renderSubPasses, [] (const auto & subPass) {
			return subPass.hasDepthStencilResolve();
		});
	}

	bool
	RenderPass::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this render pass !");

			return false;
		}

		/* If any subpass requires depth/stencil resolve, use the Vulkan 1.2+ render pass 2 path. */
		if ( this->needsRenderPass2() )
		{
			return this->createOnHardwareV2();
		}

		if ( m_attachmentDescriptions.empty() )
		{
			Tracer::warning(ClassId, "There is no attachment for this render pass !");
		}
		else
		{
			m_createInfo.attachmentCount = static_cast< uint32_t >(m_attachmentDescriptions.size());
			m_createInfo.pAttachments = m_attachmentDescriptions.data();
		}

		const auto subPassDescriptions = this->getSubPassDescriptions();

		if ( subPassDescriptions.empty() )
		{
			Tracer::warning(ClassId, "There is no sub-pass description for this render pass !");
		}
		else
		{
			m_createInfo.subpassCount = static_cast< uint32_t >(subPassDescriptions.size());
			m_createInfo.pSubpasses = subPassDescriptions.data();
		}

		if ( !m_subPassDependencies.empty() )
		{
			m_createInfo.dependencyCount = static_cast< uint32_t >(m_subPassDependencies.size());
			m_createInfo.pDependencies = m_subPassDependencies.data();
		}

		if ( const auto result = vkCreateRenderPass(this->device()->handle(), &m_createInfo, nullptr, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a render pass : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	RenderPass::createOnHardwareV2 () noexcept
	{
		/* Convert VkAttachmentDescription -> VkAttachmentDescription2. */
		StaticVector< VkAttachmentDescription2, 8 > attachmentDescriptions2;

		for ( const auto & desc : m_attachmentDescriptions )
		{
			attachmentDescriptions2.emplace_back(VkAttachmentDescription2{
				.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
				.pNext = nullptr,
				.flags = desc.flags,
				.format = desc.format,
				.samples = desc.samples,
				.loadOp = desc.loadOp,
				.storeOp = desc.storeOp,
				.stencilLoadOp = desc.stencilLoadOp,
				.stencilStoreOp = desc.stencilStoreOp,
				.initialLayout = desc.initialLayout,
				.finalLayout = desc.finalLayout
			});
		}

		/* Convert each subpass to VkSubpassDescription2 with optional depth resolve.
		 * We need persistent storage for attachment reference arrays since VkSubpassDescription2
		 * contains pointers into these arrays. */
		struct SubPassData
		{
			StaticVector< VkAttachmentReference2, 8 > inputAttachments;
			StaticVector< VkAttachmentReference2, 8 > colorAttachments;
			StaticVector< VkAttachmentReference2, 8 > resolveAttachments;
			VkAttachmentReference2 depthStencilAttachment{};
			VkAttachmentReference2 depthStencilResolveAttachment{};
			VkSubpassDescriptionDepthStencilResolve depthStencilResolve{};
			VkSubpassDescription2 description{};
		};

		StaticVector< SubPassData, 4 > subPassDataArray;

		for ( const auto & subPass : m_renderSubPasses )
		{
			auto & data = subPassDataArray.emplace_back();

			/* Convert input attachments. */
			for ( const auto & ref : subPass.m_inputAttachments )
			{
				data.inputAttachments.emplace_back(VkAttachmentReference2{
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.pNext = nullptr,
					.attachment = ref.attachment,
					.layout = ref.layout,
					.aspectMask = 0
				});
			}

			/* Convert color attachments. */
			for ( const auto & ref : subPass.m_colorAttachments )
			{
				data.colorAttachments.emplace_back(VkAttachmentReference2{
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.pNext = nullptr,
					.attachment = ref.attachment,
					.layout = ref.layout,
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
				});
			}

			/* Convert color resolve attachments. */
			for ( const auto & ref : subPass.m_resolveAttachments )
			{
				data.resolveAttachments.emplace_back(VkAttachmentReference2{
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.pNext = nullptr,
					.attachment = ref.attachment,
					.layout = ref.layout,
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
				});
			}

			/* Convert depth/stencil attachment. */
			if ( subPass.m_depthStencilAttachmentSet )
			{
				data.depthStencilAttachment = VkAttachmentReference2{
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.pNext = nullptr,
					.attachment = subPass.m_depthStencilAttachment.attachment,
					.layout = subPass.m_depthStencilAttachment.layout,
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
				};
			}

			/* Build the VkSubpassDescription2. */
			data.description.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
			data.description.pNext = nullptr;
			data.description.flags = subPass.m_flags;
			data.description.pipelineBindPoint = subPass.m_pipelineBindPoint;
			data.description.viewMask = 0;

			if ( !data.inputAttachments.empty() )
			{
				data.description.inputAttachmentCount = static_cast< uint32_t >(data.inputAttachments.size());
				data.description.pInputAttachments = data.inputAttachments.data();
			}

			data.description.colorAttachmentCount = static_cast< uint32_t >(data.colorAttachments.size());
			data.description.pColorAttachments = data.colorAttachments.empty() ? nullptr : data.colorAttachments.data();
			data.description.pResolveAttachments = data.resolveAttachments.empty() ? nullptr : data.resolveAttachments.data();
			data.description.pDepthStencilAttachment = subPass.m_depthStencilAttachmentSet ? &data.depthStencilAttachment : nullptr;
			data.description.preserveAttachmentCount = static_cast< uint32_t >(subPass.m_preserveAttachments.size());
			data.description.pPreserveAttachments = subPass.m_preserveAttachments.empty() ? nullptr : subPass.m_preserveAttachments.data();

			/* Chain depth/stencil resolve if configured. */
			if ( subPass.m_depthStencilResolveSet )
			{
				data.depthStencilResolveAttachment = VkAttachmentReference2{
					.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
					.pNext = nullptr,
					.attachment = subPass.m_depthStencilResolveAttachment.attachment,
					.layout = subPass.m_depthStencilResolveAttachment.layout,
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
				};

				data.depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
				data.depthStencilResolve.pNext = nullptr;
				data.depthStencilResolve.depthResolveMode = subPass.m_depthResolveMode;
				data.depthStencilResolve.stencilResolveMode = subPass.m_stencilResolveMode;
				data.depthStencilResolve.pDepthStencilResolveAttachment = &data.depthStencilResolveAttachment;

				data.description.pNext = &data.depthStencilResolve;
			}
		}

		/* Collect subpass description pointers. */
		StaticVector< VkSubpassDescription2, 4 > subPassDescriptions2;

		for ( auto & data : subPassDataArray )
		{
			subPassDescriptions2.emplace_back(data.description);
		}

		/* Convert subpass dependencies. */
		StaticVector< VkSubpassDependency2, 8 > subPassDependencies2;

		for ( const auto & dep : m_subPassDependencies )
		{
			subPassDependencies2.emplace_back(VkSubpassDependency2{
				.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
				.pNext = nullptr,
				.srcSubpass = dep.srcSubpass,
				.dstSubpass = dep.dstSubpass,
				.srcStageMask = dep.srcStageMask,
				.dstStageMask = dep.dstStageMask,
				.srcAccessMask = dep.srcAccessMask,
				.dstAccessMask = dep.dstAccessMask,
				.dependencyFlags = dep.dependencyFlags,
				.viewOffset = 0
			});
		}

		/* Build VkRenderPassCreateInfo2. */
		VkRenderPassCreateInfo2 createInfo2{};
		createInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		createInfo2.pNext = nullptr;
		createInfo2.flags = m_createInfo.flags;
		createInfo2.attachmentCount = static_cast< uint32_t >(attachmentDescriptions2.size());
		createInfo2.pAttachments = attachmentDescriptions2.empty() ? nullptr : attachmentDescriptions2.data();
		createInfo2.subpassCount = static_cast< uint32_t >(subPassDescriptions2.size());
		createInfo2.pSubpasses = subPassDescriptions2.empty() ? nullptr : subPassDescriptions2.data();
		createInfo2.dependencyCount = static_cast< uint32_t >(subPassDependencies2.size());
		createInfo2.pDependencies = subPassDependencies2.empty() ? nullptr : subPassDependencies2.data();
		createInfo2.correlatedViewMaskCount = 0;
		createInfo2.pCorrelatedViewMasks = nullptr;

		/* Handle multiview if enabled. */
		if ( m_multiviewEnabled )
		{
			createInfo2.correlatedViewMaskCount = 1;
			createInfo2.pCorrelatedViewMasks = &m_correlationMask;

			/* Set view masks on subpass descriptions. */
			for ( auto & desc : subPassDescriptions2 )
			{
				desc.viewMask = m_viewMask;
			}
		}

		if ( const auto result = vkCreateRenderPass2(this->device()->handle(), &createInfo2, nullptr, &m_handle); result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a render pass (v2 with depth resolve) : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	RenderPass::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the render pass " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroyRenderPass(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		m_attachmentDescriptions.clear();
		m_renderSubPasses.clear();
		m_subPassDependencies.clear();

		this->setDestroyed();

		return true;
	}
}
