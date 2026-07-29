/*
 * src/Graphics/IBLTexture.cpp
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

#include "IBLTexture.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	IBLTexture::IBLTexture (Role role) noexcept
		: m_role{role}
	{

	}

	bool
	IBLTexture::create (Renderer & renderer) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		const auto device = renderer.device();

		const bool isCube = m_role != Role::BRDFLut;
		const uint32_t layerCount = isCube ? 6 : 1;

		uint32_t size = 0;
		uint32_t mipLevels = 1;

		switch ( m_role )
		{
			case Role::BRDFLut :
				size = BRDFLutSize;
				break;

			case Role::IrradianceCubemap :
				size = IrradianceSize;
				break;

			case Role::PrefilteredCubemap :
				size = PrefilteredSize;
				mipLevels = PrefilteredMipLevels;
				break;
		}

		/* NOTE: RGBA16F is the only 16F format with mandatory STORAGE_IMAGE support. */
		m_image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VkExtent3D{size, size, 1U},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
			mipLevels,
			layerCount
		);
		m_image->setIdentifier(ClassId, this->roleName(), "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the IBL texture image (" << this->roleName() << ") !";

			return false;
		}

		/* The sampled view covers the whole image. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = layerCount
			}
		);
		m_imageView->setIdentifier(ClassId, this->roleName(), "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the IBL texture image view (" << this->roleName() << ") !";

			return false;
		}

		/* One storage view per mip level for compute writes (2D-array for cubemaps:
		 * imageStore addresses the six faces as layers). */
		m_storageViews.reserve(mipLevels);

		for ( uint32_t mipLevel = 0; mipLevel < mipLevels; mipLevel++ )
		{
			auto storageView = std::make_shared< ImageView >(
				m_image,
				isCube ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = mipLevel,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = layerCount
				}
			);
			storageView->setIdentifier(ClassId, this->roleName(), "StorageView");

			if ( !storageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the IBL texture storage view #" << mipLevel << " (" << this->roleName() << ") !";

				return false;
			}

			m_storageViews.emplace_back(storageView);
		}

		switch ( m_role )
		{
			/* NOTE: The LUT is parameterized by (NdotV, roughness) — clamp both axes. */
			case Role::BRDFLut :
				m_sampler = renderer.getSampler("IBLBrdfLut", [] (Settings &, VkSamplerCreateInfo & createInfo) {
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
					createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.maxLod = 0.0F;
				});
				break;

			case Role::IrradianceCubemap :
				m_sampler = renderer.getSampler("IBLIrradiance", [] (Settings &, VkSamplerCreateInfo & createInfo) {
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
					createInfo.maxLod = 0.0F;
				});
				break;

			/* NOTE: The whole point of the prefiltered chain is roughness-driven LOD:
			 * never clamp it (the shared "Cubemap" sampler would). */
			case Role::PrefilteredCubemap :
				m_sampler = renderer.getSampler("IBLPrefiltered", [] (Settings &, VkSamplerCreateInfo & createInfo) {
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					createInfo.maxLod = VK_LOD_CLAMP_NONE;
				});
				break;
		}

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get a sampler for the IBL texture (" << this->roleName() << ") !";

			return false;
		}

		return true;
	}

	void
	IBLTexture::destroy () noexcept
	{
		/* NOTE: The sampler comes from the renderer's shared cache; only release our reference. */
		m_sampler.reset();
		m_storageViews.clear();
		m_imageView.reset();
		m_image.reset();
	}

	uint32_t
	IBLTexture::mipLevels () const noexcept
	{
		return m_image == nullptr ? 0 : m_image->createInfo().mipLevels;
	}

	std::shared_ptr< ImageView >
	IBLTexture::storageView (uint32_t mipLevel) const noexcept
	{
		if ( mipLevel >= m_storageViews.size() )
		{
			return nullptr;
		}

		return m_storageViews[mipLevel];
	}

	bool
	IBLTexture::isCreated () const noexcept
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
	IBLTexture::type () const noexcept
	{
		return m_role == Role::BRDFLut ? TextureType::Texture2D : TextureType::TextureCube;
	}

	uint32_t
	IBLTexture::dimensions () const noexcept
	{
		return m_role == Role::BRDFLut ? 2 : 3;
	}

	bool
	IBLTexture::isCubemapTexture () const noexcept
	{
		return m_role != Role::BRDFLut;
	}

	std::shared_ptr< Image >
	IBLTexture::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	IBLTexture::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	IBLTexture::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	IBLTexture::request3DTextureCoordinates () const noexcept
	{
		return m_role != Role::BRDFLut;
	}
}
