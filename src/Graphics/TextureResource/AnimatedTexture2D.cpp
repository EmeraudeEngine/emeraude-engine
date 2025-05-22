/*
 * src/Graphics/TextureResource/AnimatedTexture2D.cpp
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

#include "AnimatedTexture2D.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Graphics/Renderer.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::TextureResource::AnimatedTexture2D >::ClassId{"AnimatedTexture2DContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::TextureResource::AnimatedTexture2D >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::TextureResource
{
	using namespace EmEn::Libs;
	using namespace EmEn::Vulkan;

	const size_t AnimatedTexture2D::ClassUID{getClassUID(ClassId)};

	bool
	AnimatedTexture2D::isCreated () const noexcept
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
	AnimatedTexture2D::createOnHardware (Renderer & renderer) noexcept
	{
		for ( const auto & pixmap: m_localData->frames() | std::views::keys )
		{
			if ( !this->validateTexture(pixmap, !renderer.vulkanInstance().isStandardTextureCheckEnabled()) )
			{
				return false;
			}
		}

		/* Create a Vulkan image. */
		m_image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(m_localData->data(0).colorCount()),
			VkExtent3D{m_localData->width(), m_localData->height(), 1},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0, /* flags */
			1, /* mipLevels */
			m_localData->frameCount(), /* arrayLayers */
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL
		);
		m_image->setIdentifier(ClassId, this->name(), "Image");

		if ( !m_image->create(renderer.transferManager(), m_localData) )
		{
			Tracer::error(ClassId, "Unable to create an image !");

			m_image.reset();

			return false;
		}

		/* Create a Vulkan image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
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
		m_sampler = renderer.getSampler(0, 0);
		m_sampler->setIdentifier(ClassId, this->name(), "Sampler");

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a sampler !");

			return false;
		}

		return true;
	}

	bool
	AnimatedTexture2D::destroyFromHardware () noexcept
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
	AnimatedTexture2D::isGrayScale () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return false;
		}

		return m_localData->isGrayScale();
	}

	PixelFactory::Color< float >
	AnimatedTexture2D::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return PixelFactory::Black;
		}

		return m_localData->averageColor();
	}

	uint32_t
	AnimatedTexture2D::frameCount () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameCount();
	}

	uint32_t
	AnimatedTexture2D::duration () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->duration();
	}

	uint32_t
	AnimatedTexture2D::frameIndexAt (uint32_t sceneTime) const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameIndexAt(sceneTime);
	}

	bool
	AnimatedTexture2D::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = Resources::Manager::instance()->container< MovieResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	AnimatedTexture2D::load (const std::filesystem::path & filepath) noexcept
	{
		/* Looking for a movie resource by extracting the resource name from filepath.
		 * NOTE: The loading process is synchronous here. */
		const auto movieResource = Resources::Manager::instance()->container< MovieResource >()->getResource(
			getResourceNameFromFilepath(filepath, "Movies"),
			false
		);

		return this->load(movieResource);
	}

	bool
	AnimatedTexture2D::load (const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::warning(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	AnimatedTexture2D::load (const std::shared_ptr< MovieResource > & movieResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( movieResource == nullptr )
		{
			Tracer::error(ClassId, "The movie resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_localData = movieResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the movie '" << movieResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
