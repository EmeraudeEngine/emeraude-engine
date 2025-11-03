/*
 * src/Graphics/TextureResource/Texture1D.cpp
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

#include "Texture1D.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Graphics::TextureResource
{
	using namespace Libs;
	using namespace Vulkan;

	bool
	Texture1D::isCreated () const noexcept
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
	Texture1D::createTexture (Renderer & /*renderer*/) noexcept
	{
		Tracer::error(ClassId, "Not yet implemented !");

		return false;
	}

	bool
	Texture1D::destroyTexture () noexcept
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
	Texture1D::isGrayScale () const noexcept
	{
		if ( m_localData == nullptr )
		{
			return false;
		}

		return m_localData->data().isGrayScale();
	}

	PixelFactory::Color< float >
	Texture1D::averageColor () const noexcept
	{
		if ( m_localData == nullptr )
		{
			return PixelFactory::Black;
		}

		return m_localData->data().averageColor();
	}

	bool
	Texture1D::load (Resources::ServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = serviceProvider.container< ImageResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(false);
	}

	bool
	Texture1D::load (Resources::ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		return this->load(serviceProvider.container< ImageResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "Images"),
			true)
		);
	}

	bool
	Texture1D::load (Resources::ServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::error(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	Texture1D::load (const std::shared_ptr< ImageResource > & imageResource) noexcept
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
}
