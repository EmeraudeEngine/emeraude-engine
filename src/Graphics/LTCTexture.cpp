/*
 * src/Graphics/LTCTexture.cpp
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

#include "LTCTexture.hpp"

/* Local inclusions. */
#include "Graphics/LTCTables.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	LTCTexture::create (Renderer & renderer) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		constexpr uint32_t LayerCount{2};
		constexpr size_t LayerBytes{LTC::TableValueCount * sizeof(uint16_t)};

		m_image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VkExtent3D{LTC::TableSize, LTC::TableSize, 1},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			0,
			1,
			LayerCount
		);
		m_image->setIdentifier(ClassId, "Tables", "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the LTC table image !";

			return false;
		}

		/* Layer 0 the inverse matrix, layer 1 the magnitude and Fresnel, one after the other in the staging buffer (the
		 * transfer copies layer n from offset n × the layer size). */
		const auto uploaded = renderer.transferManager().uploadImage(*m_image, LayerCount * LayerBytes, [] (const Buffer & stagingBuffer) {
			return
				stagingBuffer.writeData({LTC::MatrixTable.data(), LayerBytes, 0}) &&
				stagingBuffer.writeData({LTC::AmplitudeTable.data(), LayerBytes, LayerBytes});
		});

		if ( !uploaded )
		{
			TraceError{ClassId} << "Unable to upload the LTC tables !";

			return false;
		}

		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = LayerCount
			}
		);
		m_imageView->setIdentifier(ClassId, "Tables", "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the LTC table image view !";

			return false;
		}

		/* Bilinear between the fitted texels, clamped: the table is sampled at texel centres (63/64 scale, 0.5/64 bias). */
		m_sampler = renderer.getSampler("LTCTables", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 0.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the LTC table sampler !";

			return false;
		}

		return true;
	}

	void
	LTCTexture::destroy () noexcept
	{
		m_sampler.reset();
		m_imageView.reset();
		m_image.reset();
	}

	bool
	LTCTexture::isCreated () const noexcept
	{
		return m_image != nullptr && m_image->isCreated() && m_imageView != nullptr && m_imageView->isCreated() && m_sampler != nullptr && m_sampler->isCreated();
	}

	TextureType
	LTCTexture::type () const noexcept
	{
		return TextureType::Texture2DArray;
	}
}
