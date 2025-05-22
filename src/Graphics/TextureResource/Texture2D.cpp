/*
 * src/Graphics/TextureResource/Texture2D.cpp
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

#include "Texture2D.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions. */
#include "Abstract.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Renderer.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Resources/Container.hpp"
#include "Resources/Manager.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Tracer.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture2D >::ClassId{"Texture2DContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture2D >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::TextureResource
{
	using namespace EmEn::Libs;
	using namespace Vulkan;

	const size_t Texture2D::ClassUID{getClassUID(ClassId)};

	Texture2D::Texture2D (const std::string & name, uint32_t resourceFlags) noexcept
		: Abstract(name, resourceFlags)
	{

	}

	Texture2D::~Texture2D ()
	{
		this->destroyFromHardware();
	}

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
	Texture2D::createOnHardware (Renderer & renderer) noexcept
	{
		if ( !this->validateTexture(m_localData->data(), !renderer.vulkanInstance().isStandardTextureCheckEnabled()) )
		{
			return false;
		}

		auto & settings = renderer.primaryServices().settings();

		const auto mipLevels = std::min(
			Image::getMIPLevels(m_localData->width(), m_localData->height()),
			settings.get< uint32_t >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels)
		);

		/* Create a Vulkan image. */
		m_image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(m_localData->data().colorCount()),
			VkExtent3D{m_localData->width(), m_localData->height(), 1U},
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0,
			mipLevels,
			1,
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
		m_sampler = Renderer::instance()->getSampler(0, 0);
		m_sampler->setIdentifier(ClassId, this->name(), "Sampler");

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a sampler !");

			return false;
		}

		return true;
	}

	bool
	Texture2D::destroyFromHardware () noexcept
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

	size_t
	Texture2D::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	Texture2D::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	Type
	Texture2D::type () const noexcept
	{
		return Type::Texture2D;
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

	uint32_t
	Texture2D::dimensions () const noexcept
	{
		return 2;
	}

	bool
	Texture2D::isCubemapTexture () const noexcept
	{
		return false;
	}

	uint32_t
	Texture2D::frameCount () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return 1;
	}

	uint32_t
	Texture2D::duration () const noexcept
	{
		return 0;
	}

	uint32_t
	Texture2D::frameIndexAt (uint32_t /*sceneTime*/) const noexcept
	{
		return 0;
	}

	std::shared_ptr< Image >
	Texture2D::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	Texture2D::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	Texture2D::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	Texture2D::request3DTextureCoordinates () const noexcept
	{
		return false;
	}

	const char *
	Texture2D::classLabel () const noexcept
	{
		return ClassId;
	}

	bool
	Texture2D::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = Resources::Manager::instance()->container< ImageResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	Texture2D::load (const std::filesystem::path & filepath) noexcept
	{
		return this->load(Resources::Manager::instance()->container< ImageResource >()->getResource(getResourceNameFromFilepath(filepath, "Images"), true));
	}

	bool
	Texture2D::load (const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container ! */
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
