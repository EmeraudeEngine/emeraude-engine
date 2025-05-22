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

/* STL inclusions. */
#include <memory>
#include <string>

/* Local inclusions. */
#include "Graphics/FramebufferPrecisions.hpp"
#include "Graphics/Renderer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Instance.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::RenderTarget
{
	using namespace EmEn::Libs;
	using namespace Vulkan;

	constexpr auto TracerTag{"RenderTarget"};

	Abstract::Abstract (const std::string & name, const FramebufferPrecisions & precisions, const VkExtent3D & extent, RenderTargetType renderType, AVConsole::ConnexionType allowedConnexionType, bool enableSyncPrimitives) noexcept
		: AbstractVirtualDevice(name, AVConsole::DeviceType::Video, allowedConnexionType),
		m_precisions(precisions),
		m_extent(extent),
		m_renderArea({
			.offset = {.x = 0, .y = 0},
			.extent = {.width = extent.width, .height = extent.height}
		}),
		m_renderType(renderType)
	{
		m_flags[EnableSyncPrimitive] = enableSyncPrimitives;
	}

	bool
	Abstract::create (Renderer & renderer) noexcept
	{
		if ( m_flags[EnableSyncPrimitive] )
		{
			m_fence = std::make_shared< Sync::Fence >(renderer.device(), VK_FENCE_CREATE_SIGNALED_BIT);
			m_fence->setIdentifier(TracerTag, this->id(), "Fence");

			if ( !m_fence->createOnHardware() )
			{
				Tracer::error(TracerTag, "Unable to create the render target fence !");

				m_fence.reset();

				return false;
			}

			m_semaphore = std::make_shared< Sync::Semaphore >(renderer.device());
			m_semaphore->setIdentifier(TracerTag, this->id(), "Semaphore");

			if ( !m_semaphore->createOnHardware() )
			{
				Tracer::error(TracerTag, "Unable to create the render target semaphore !");

				m_semaphore.reset();

				return false;
			}
		}

		m_flags[RenderOutOfDate] = true;

		return this->onCreate(renderer);
	}

	bool
	Abstract::destroy () noexcept
	{
		this->onDestroy();

		if ( m_flags[EnableSyncPrimitive] )
		{
			if ( m_semaphore != nullptr )
			{
				m_semaphore->destroyFromHardware();

				m_semaphore.reset();
			}

			if ( m_fence != nullptr )
			{
				m_fence->destroyFromHardware();

				m_fence.reset();
			}
		}

		return true;
	}

	bool
	Abstract::createColorBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & purposeId) const noexcept
	{
		image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0, // flags
			1, // Image mip levels
			1, //m_createInfo.imageArrayLayers, // Image array layers
			VK_SAMPLE_COUNT_1_BIT, // Image multi sampling
			VK_IMAGE_TILING_OPTIMAL // Image tiling
		);
		image->setIdentifier(purposeId + "-Image");

		if ( !image->createOnHardware() )
		{
			TraceError{TracerTag} << "Unable to create image '" << purposeId << "' !";

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
		imageView->setIdentifier(purposeId + "-ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		return true;
	}

	bool
	Abstract::createDepthBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & purposeId) const noexcept
	{
		image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0, // flags
			1, // Image mip levels
			1, //m_createInfo.imageArrayLayers, // Image array layers
			VK_SAMPLE_COUNT_1_BIT, // Image multi sampling
			VK_IMAGE_TILING_OPTIMAL // Image tiling
		);
		image->setIdentifier(purposeId + "-Image");

		if ( !image->createOnHardware() )
		{
			TraceError{TracerTag} << "Unable to create image '" << purposeId << "' !";

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
		imageView->setIdentifier(purposeId + "D-ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		return true;
	}

	bool
	Abstract::createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & purposeId) noexcept
	{
		if ( !this->createDepthBuffer(device, image, depthImageView, purposeId) )
		{
			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		stencilImageView = std::make_shared< ImageView >(
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
		stencilImageView->setIdentifier(purposeId + "S-ImageView");

		if ( !stencilImageView->createOnHardware() )
		{
			TraceFatal{TracerTag} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		return true;
	}
}
