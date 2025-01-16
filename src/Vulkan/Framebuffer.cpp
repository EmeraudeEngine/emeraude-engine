/*
 * src/Vulkan/Framebuffer.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Framebuffer.hpp"

/* Local inclusions. */
#include "Device.hpp"
#include "RenderPass.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace Emeraude::Vulkan
{
	using namespace Libraries;

	Framebuffer::Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkExtent2D & extent, uint32_t layerCount, VkFramebufferCreateFlags createFlags) noexcept
		: AbstractDeviceDependentObject(renderPass->device()), m_renderPass(renderPass)
	{
		m_createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = createFlags;
		m_createInfo.renderPass = VK_NULL_HANDLE;
		m_createInfo.attachmentCount = 0;
		m_createInfo.pAttachments = nullptr;
		m_createInfo.width = extent.width;
		m_createInfo.height = extent.height;
		m_createInfo.layers = layerCount;

		m_attachments.reserve(3);
	}

	Framebuffer::Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkExtent3D & extent, VkFramebufferCreateFlags createFlags) noexcept
		: Framebuffer(renderPass, VkExtent2D{extent.width, extent.height}, extent.depth, createFlags)
	{

	}

	Framebuffer::Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkFramebufferCreateInfo & createInfo) noexcept
		: AbstractDeviceDependentObject(renderPass->device()), m_createInfo(createInfo), m_renderPass(renderPass)
	{

	}

	Framebuffer::~Framebuffer ()
	{
		this->destroyFromHardware();
	}

	bool
	Framebuffer::createOnHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			Tracer::error(ClassId, "No device to create this framebuffer !");

			return false;
		}

		if ( m_renderPass == nullptr || !m_renderPass->isCreated() )
		{
			Tracer::error(ClassId, "The render pass is null or not created ! Cannot make a framebuffer with it !");

			return false;
		}

		if ( m_attachments.empty() )
		{
			Tracer::error(ClassId, "No attachment to create this framebuffer !");

			return false;
		}

		m_createInfo.renderPass = m_renderPass->handle();
		m_createInfo.attachmentCount = static_cast< uint32_t >(m_attachments.size());
		m_createInfo.pAttachments = m_attachments.data();

		const auto result = vkCreateFramebuffer(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		if ( result != VK_SUCCESS )
		{
			TraceError{ClassId} << "Unable to create a framebuffer : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setCreated();

		return true;
	}

	bool
	Framebuffer::destroyFromHardware () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the framebuffer " << m_handle << " (" << this->identifier() << ") !";

			return false;
		}

		if ( m_handle != VK_NULL_HANDLE )
		{
			this->device()->waitIdle();

			vkDestroyFramebuffer(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		this->setDestroyed();

		return true;
	}
}
