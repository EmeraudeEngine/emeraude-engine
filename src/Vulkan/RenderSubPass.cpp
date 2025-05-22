/*
 * src/Vulkan/RenderSubPass.cpp
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

#include "RenderSubPass.hpp"

namespace EmEn::Vulkan
{
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
}
