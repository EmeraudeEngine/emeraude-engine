/*
 * src/Graphics/DummyShadowTexture.cpp
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

#include "DummyShadowTexture.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	DummyShadowTexture::DummyShadowTexture (bool isCubemap) noexcept
		: m_isCubemap{isCubemap}
	{

	}

	bool
	DummyShadowTexture::create (Renderer & renderer) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		const auto device = renderer.device();
		const uint32_t layerCount = m_isCubemap ? 6 : 1;

		/* Create a 1x1 depth image with value 1.0 (no shadow). */
		m_image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			VK_FORMAT_D32_SFLOAT,
			VkExtent3D{1, 1, 1},
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			m_isCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
			1,
			layerCount
		);
		m_image->setIdentifier(ClassId, m_isCubemap ? "Cubemap" : "2D", "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the dummy shadow texture image !";

			return false;
		}

		/* Initialize the depth value to 1.0 (maximum depth = no shadow).
		 * NOTE: We need to transition the image layout and clear it. */
		{
			const auto & transferManager = renderer.transferManager();

			/* Transition to transfer destination for clearing. */
			if ( !transferManager.transitionImageLayout(
				*m_image,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			) )
			{
				TraceError{ClassId} << "Unable to transition dummy shadow texture for clearing !";

				return false;
			}

			/* Clear to depth 1.0 (maximum = no shadow). */
			if ( !transferManager.clearDepthImage(*m_image, 1.0F) )
			{
				TraceError{ClassId} << "Unable to clear dummy shadow texture !";

				return false;
			}

			/* Transition to shader read layout for sampling. */
			if ( !transferManager.transitionImageLayout(
				*m_image,
				VK_IMAGE_ASPECT_DEPTH_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			) )
			{
				TraceError{ClassId} << "Unable to transition dummy shadow texture for shader read !";

				return false;
			}
		}

		m_image->setCurrentImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		/* Create the image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			m_isCubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = layerCount
			}
		);
		m_imageView->setIdentifier(ClassId, m_isCubemap ? "Cubemap" : "2D", "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the dummy shadow texture image view !";

			return false;
		}

		/* Get or create the shadow sampler (same as real shadow maps). */
		m_sampler = renderer.getSampler("ShadowMap", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_TRUE;
			createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 1.0F;
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the shadow sampler for dummy shadow texture !";

			return false;
		}

		TraceSuccess{ClassId} << "Dummy shadow texture (" << (m_isCubemap ? "Cubemap" : "2D") << ") created successfully.";

		return true;
	}

	void
	DummyShadowTexture::destroy () noexcept
	{
		m_sampler.reset();
		m_imageView.reset();
		m_image.reset();
	}

	bool
	DummyShadowTexture::isCreated () const noexcept
	{
		if ( m_image == nullptr || !m_image->isCreated() )
		{
			return false;
		}

		if ( m_imageView == nullptr || !m_imageView->isCreated() )
		{
			return false;
		}

		if ( m_sampler == nullptr || !m_sampler->isCreated() )
		{
			return false;
		}

		return true;
	}

	TextureType
	DummyShadowTexture::type () const noexcept
	{
		return m_isCubemap ? TextureType::TextureCube : TextureType::Texture2D;
	}

	uint32_t
	DummyShadowTexture::dimensions () const noexcept
	{
		return m_isCubemap ? 3 : 2;
	}

	bool
	DummyShadowTexture::isCubemapTexture () const noexcept
	{
		return m_isCubemap;
	}

	std::shared_ptr< Image >
	DummyShadowTexture::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	DummyShadowTexture::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	DummyShadowTexture::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	DummyShadowTexture::request3DTextureCoordinates () const noexcept
	{
		return m_isCubemap;
	}
}
