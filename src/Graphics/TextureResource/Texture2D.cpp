/*
 * src/Graphics/TextureResource/Texture2D.cpp
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

#include "Texture2D.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureCache.hpp"
#include "Graphics/TextureCompressor.hpp"

namespace EmEn::Graphics::TextureResource
{
	using namespace Libs;
	using namespace Vulkan;

	bool
	Texture2D::isCreated () const noexcept
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

	bool
	Texture2D::createTexture (Renderer & renderer) noexcept
	{
		if ( !this->validateTexture(m_localData->data(), !renderer.vulkanInstance().isStandardTextureCheckEnabled()) )
		{
			return false;
		}

		auto & settings = renderer.primaryServices().settings();

		const auto mipLevels = std::min(
			Image::getMIPLevels(m_localData->width(), m_localData->height()),
			settings.getOrSetDefault< uint32_t >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels)
		);

		/* Check if BC7 hardware compression is supported. */
		const auto useBC7 = renderer.device()->physicalDevice()->featuresVK10().textureCompressionBC == VK_TRUE;
		bool imageCreated = false;

		if ( useBC7 )
		{
			/* BC7 compressed path: try disk cache first, compress on CPU if miss. */
			TextureCompressor::initialize();

			auto & fileSystem = renderer.primaryServices().fileSystem();
			TextureCache::initialize(fileSystem.cacheDirectory());

			/* Compute source file identity for cache invalidation. */
			const auto sourceFileSize = static_cast< uint64_t >(m_localData->data().bytes());
			const auto sourceModTime = static_cast< uint64_t >(m_localData->data().width()) * 1000000ULL
				+ static_cast< uint64_t >(m_localData->data().height());

			/* Try loading from disk cache. */
			auto compressedMips = TextureCache::tryLoad(this->name(), sourceFileSize, sourceModTime);

			if ( compressedMips.empty() )
			{
				/* Cache miss: compress and store. */
				compressedMips = TextureCompressor::compress(
					m_localData->data(),
					mipLevels,
					*renderer.primaryServices().threadPool()
				);

				if ( !compressedMips.empty() )
				{
					[[maybe_unused]] const auto cached = TextureCache::store(this->name(), sourceFileSize, sourceModTime, compressedMips);
				}
			}

			if ( !compressedMips.empty() )
			{
				const auto bc7Format = this->isSRGB() ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;

				m_image = std::make_shared< Vulkan::Image >(
					renderer.device(),
					VK_IMAGE_TYPE_2D,
					bc7Format,
					VkExtent3D{m_localData->width(), m_localData->height(), 1U},
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					0,
					static_cast< uint32_t >(compressedMips.size())
				);
				m_image->setIdentifier(ClassId, this->name(), "Image");

				/* Build CompressedMip descriptors for the upload. */
				std::vector< Vulkan::Image::CompressedMip > mipDescs;
				mipDescs.reserve(compressedMips.size());

				for ( const auto & mip : compressedMips )
				{
					mipDescs.push_back({mip.data.data(), static_cast< uint32_t >(mip.data.size()), mip.width, mip.height});
				}

				if ( m_image->createFromCompressed(renderer.transferManager(), mipDescs) )
				{
					imageCreated = true;
				}
				else
				{
					TraceWarning{ClassId} << "BC7 upload failed for '" << this->name() << "', falling back to uncompressed.";

					m_image.reset();
				}
			}
			else
			{
				TraceWarning{ClassId} << "BC7 compression failed for '" << this->name() << "', falling back to uncompressed.";
			}
		}

		if ( !imageCreated )
		{
			/* Uncompressed path (fallback or no BC7 support). */
			m_image = std::make_shared< Vulkan::Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				Image::getFormat< uint8_t >(m_localData->data().colorCount(), this->isSRGB()),
				VkExtent3D{m_localData->width(), m_localData->height(), 1U},
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				mipLevels
			);
			m_image->setIdentifier(ClassId, this->name(), "Image");

			if ( !m_image->create(renderer.transferManager(), m_localData) )
			{
				Tracer::error(ClassId, "Unable to create an image !");

				m_image.reset();

				return false;
			}
		}

		/* Create a Vulkan image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = m_image->createInfo().mipLevels,
				.baseArrayLayer = 0,
				.layerCount = m_image->createInfo().arrayLayers
			}
		);
		m_imageView->setIdentifier(ClassId, this->name(), "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create an image view !");

			return false;
		}

		/* Get a Vulkan sampler. */
		m_sampler = renderer.getSampler("Texture2D", [] (Settings & settings, VkSamplerCreateInfo & createInfo) {
			const auto magFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
			const auto minFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);
			const auto mipmapMode = settings.getOrSetDefault< std::string >(GraphicsTextureMipFilteringKey, DefaultGraphicsTextureFiltering);
			const auto mipLevels = settings.getOrSetDefault< float >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
			const auto anisotropyLevels = settings.getOrSetDefault< float >(GraphicsTextureAnisotropyLevelsKey, DefaultGraphicsTextureAnisotropy);

			//createInfo.flags = 0;
			createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.mipmapMode = mipmapMode == "linear" ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
			//createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = anisotropyLevels > 1.0F ? VK_TRUE : VK_FALSE;
			createInfo.maxAnisotropy = anisotropyLevels;
			//createInfo.compareEnable = VK_FALSE;
			//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			//createInfo.minLod = 0.0F;
			createInfo.maxLod = mipLevels > 0.0F ? mipLevels : VK_LOD_CLAMP_NONE;
			//createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			//createInfo.unnormalizedCoordinates = VK_FALSE;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a sampler !");

			return false;
		}

		return true;
	}

	bool
	Texture2D::destroyTexture () noexcept
	{
		if ( m_image != nullptr )
		{
			m_image->destroyFromHardware();
			m_image.reset();
		}

		if ( m_imageView != nullptr )
		{
			m_imageView->destroyFromHardware();
			m_imageView.reset();
		}

		if ( m_sampler != nullptr )
		{
			m_sampler->destroyFromHardware();
			m_sampler.reset();
		}

		return true;
	}

	bool
	Texture2D::isGrayScale () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return false;
		}

		return m_localData->data().isGrayScale();
	}

	PixelFactory::Color< float >
	Texture2D::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return PixelFactory::Black;
		}

		return m_localData->data().averageColor();
	}

	bool
	Texture2D::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = this->serviceProvider().container< ImageResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	Texture2D::load (const std::filesystem::path & filepath) noexcept
	{
		return this->load(this->serviceProvider().container< ImageResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "Images"),
			true)
		);
	}

	bool
	Texture2D::load (const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::error(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	Texture2D::load (const std::shared_ptr< ImageResource > & imageResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( imageResource == nullptr )
		{
			Tracer::error(ClassId, "The image resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_localData = imageResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the image '" << imageResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	std::shared_ptr< ImageResource >
	Texture2D::localData () noexcept
	{
		return m_localData;
	}
}
