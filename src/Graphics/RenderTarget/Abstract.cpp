/*
 * src/Graphics/RenderTarget/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Instance.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::RenderTarget
{
	using namespace Libs;
	using namespace Vulkan;

	constexpr auto TracerTag{"RenderTarget"};

	bool
	Abstract::create (Renderer & renderer) noexcept
	{
		if ( m_enableSyncPrimitive )
		{
			m_semaphore = std::make_shared< Sync::Semaphore >(renderer.device());
			m_semaphore->setIdentifier(TracerTag, this->id(), "Semaphore");

			if ( !m_semaphore->createOnHardware() )
			{
				Tracer::error(TracerTag, "Unable to create the render target semaphore!");

				m_semaphore.reset();

				return false;
			}
		}

		if ( !this->onCreate(renderer) )
		{
			Tracer::error(TracerTag, "Unable to create a complete render target! Destroying it...");

			this->destroy();

			return false;
		}

		m_renderOutOfDate = true;

		return true;
	}

	bool
	Abstract::destroy () noexcept
	{
		this->onDestroy();

		m_semaphore.reset();

		return true;
	}

	void
	Abstract::setViewport (const CommandBuffer & commandBuffer) const noexcept
	{
		VkViewport viewport{};
		viewport.x = 0.0F;
		viewport.y = 0.0F;
		viewport.width = static_cast< float >(m_extent.width);
		viewport.height = static_cast< float >(m_extent.height);
		viewport.minDepth = 0.0F;
		viewport.maxDepth = 1.0F;

		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);
	}

	bool
	Abstract::createColorBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & identifier) const noexcept
	{
		const auto instanceID = identifier + "ColorBuffer";

		image = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		);
		image->setIdentifier(TracerTag, instanceID, "Image");

		if ( !image->createOnHardware() )
		{
			TraceError{TracerTag} << "Unable to create image '" << instanceID << "' !";

			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		imageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		imageView->setIdentifier(TracerTag, instanceID, "ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << instanceID << "' !";

			return false;
		}

		return true;
	}

	bool
	Abstract::createDepthBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & identifier) const noexcept
	{
		const auto instanceID = identifier + "DepthBuffer";

		image = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
		image->setIdentifier(TracerTag, instanceID, "Image");

		if ( !image->createOnHardware() )
		{
			TraceError{TracerTag} << "Unable to create image '" << instanceID << "' !";

			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		imageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		imageView->setIdentifier(TracerTag, instanceID, "ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << instanceID << "' !";

			return false;
		}

		return true;
	}

	bool
	Abstract::createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & identifier) noexcept
	{
		if ( !this->createDepthBuffer(device, image, depthImageView, identifier) )
		{
			return false;
		}

		const auto instanceID = identifier + "StencilBuffer";

		const auto & imageCreateInfo = image->createInfo();

		stencilImageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		stencilImageView->setIdentifier(TracerTag, instanceID, "ImageView");

		if ( !stencilImageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << instanceID << "' !";

			return false;
		}

		return true;
	}
}
