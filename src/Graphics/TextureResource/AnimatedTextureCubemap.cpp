/*
 * src/Graphics/TextureResource/AnimatedTextureCubemap.cpp
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

#include "AnimatedTextureCubemap.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Graphics/Renderer.hpp"

namespace EmEn::Graphics::TextureResource
{
	using namespace Libs;
	using namespace Vulkan;

	bool
	AnimatedTextureCubemap::isCreated () const noexcept
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
	AnimatedTextureCubemap::createTexture (Renderer & renderer) noexcept
	{
		/* Validate all faces of all frames. */
		for ( const auto & [faces, duration] : m_localData->frames() )
		{
			for ( const auto & pixmap : faces )
			{
				if ( !this->validateTexture(pixmap, !renderer.vulkanInstance().isStandardTextureCheckEnabled()) )
				{
					return false;
				}
			}
		}

		const auto totalLayers = static_cast< uint32_t >(CubemapFaceCount) * m_localData->frameCount();

		/* Create a Vulkan image (cube array). */
		m_image = std::make_shared< Vulkan::Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(m_localData->data(0, 0).colorCount()),
			VkExtent3D{m_localData->cubeSize(), m_localData->cubeSize(), 1},
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			1, /* mipLevels: no mipmaps for animated textures */
			totalLayers
		);
		m_image->setIdentifier(ClassId, this->name(), "Image");

		if ( !m_image->create(renderer.transferManager(), m_localData) )
		{
			Tracer::error(ClassId, "Unable to create an image !");

			m_image.reset();

			return false;
		}

		/* Create a Vulkan image view (cube array). */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
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
		m_sampler = renderer.getSampler("AnimatedCubemap", [] (Settings & settings, VkSamplerCreateInfo & createInfo) {
			const auto magFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
			const auto minFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);

			//createInfo.flags = 0;
			createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			//createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = VK_FALSE;
			//createInfo.maxAnisotropy = 1.0F;
			//createInfo.compareEnable = VK_FALSE;
			//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			//createInfo.minLod = 0.0F;
			createInfo.maxLod = 0.0F;
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
	AnimatedTextureCubemap::destroyTexture () noexcept
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
	AnimatedTextureCubemap::isGrayScale () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return false;
		}

		return m_localData->isGrayScale();
	}

	PixelFactory::Color< float >
	AnimatedTextureCubemap::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return PixelFactory::Black;
		}

		return m_localData->averageColor();
	}

	uint32_t
	AnimatedTextureCubemap::frameCount () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameCount();
	}

	uint32_t
	AnimatedTextureCubemap::duration () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->duration();
	}

	uint32_t
	AnimatedTextureCubemap::frameIndexAt (uint32_t sceneTime) const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameIndexAt(sceneTime);
	}

	bool
	AnimatedTextureCubemap::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = serviceProvider.container< CubemapMovieResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	AnimatedTextureCubemap::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		/* Looking for a cubemap movie resource by extracting the resource name from the filepath.
		 * NOTE: The loading process is synchronous here. */
		const auto cubemapMovieResource = serviceProvider.container< CubemapMovieResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "CubemapMovies"),
			false
		);

		return this->load(cubemapMovieResource);
	}

	bool
	AnimatedTextureCubemap::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::warning(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	AnimatedTextureCubemap::load (const std::shared_ptr< CubemapMovieResource > & cubemapMovieResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( cubemapMovieResource == nullptr )
		{
			Tracer::error(ClassId, "The cubemap movie resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_localData = cubemapMovieResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the cubemap movie '" << cubemapMovieResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
