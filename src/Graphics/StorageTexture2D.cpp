/*
 * src/Graphics/StorageTexture2D.cpp
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

#include "StorageTexture2D.hpp"

/* STL inclusions. */
#include <utility>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	StorageTexture2D::StorageTexture2D (std::string name, VkExtent2D extent, VkFormat format) noexcept
		: m_name{std::move(name)},
		m_extent{extent},
		m_format{format}
	{

	}

	bool
	StorageTexture2D::create (Renderer & renderer) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		m_image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			m_format,
			VkExtent3D{m_extent.width, m_extent.height, 1U},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0,
			1,
			1
		);
		m_image->setIdentifier(ClassId, m_name, "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the image of '" << m_name << "' !";

			return false;
		}

		const VkImageSubresourceRange range{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		/* The same subresource, viewed twice: the sampled view and the storage view never alias a different range. */
		m_imageView = std::make_shared< ImageView >(m_image, VK_IMAGE_VIEW_TYPE_2D, range);
		m_imageView->setIdentifier(ClassId, m_name, "ImageView");

		m_storageView = std::make_shared< ImageView >(m_image, VK_IMAGE_VIEW_TYPE_2D, range);
		m_storageView->setIdentifier(ClassId, m_name, "StorageView");

		if ( !m_imageView->createOnHardware() || !m_storageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the views of '" << m_name << "' !";

			return false;
		}

		/* A lookup table: linear between texels, clamped at its edges (its parametrisations never wrap). */
		m_sampler = renderer.getSampler("StorageTexture2DLinearClamp", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.maxLod = 0.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the sampler of '" << m_name << "' !";

			return false;
		}

		return true;
	}

	void
	StorageTexture2D::destroy () noexcept
	{
		/* NOTE: The sampler comes from the renderer's shared cache; only release our reference. */
		m_sampler.reset();
		m_storageView.reset();
		m_imageView.reset();
		m_image.reset();
	}

	bool
	StorageTexture2D::isCreated () const noexcept
	{
		return m_image != nullptr && m_image->isCreated() &&
			m_imageView != nullptr && m_imageView->isCreated() &&
			m_storageView != nullptr && m_storageView->isCreated() &&
			m_sampler != nullptr && m_sampler->isCreated();
	}
}
