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
		m_viewMask = 0b00111111; /* 6 bits for 6 faces */
		m_correlationMask = 0b00111111;

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
	RenderPass::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this render pass !");

			return false;
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
