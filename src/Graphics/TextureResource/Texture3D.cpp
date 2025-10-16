/*
 * src/Graphics/TextureResource/Texture3D.cpp
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

#include "Texture3D.hpp"

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
	Texture3D::isCreated () const noexcept
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
	Texture3D::createOnHardware (Renderer & /*renderer*/) noexcept
	{
		Tracer::error(ClassId, "Not yet implemented !");

		return false;
	}

	bool
	Texture3D::destroyFromHardware () noexcept
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
	Texture3D::isGrayScale () const noexcept
	{
		/* FIXME: No local data for now. */
		return false;
	}

	PixelFactory::Color< float >
	Texture3D::averageColor () const noexcept
	{
		/* FIXME: No local data for now. */
		return PixelFactory::Black;
	}

	bool
	Texture3D::load (Resources::ServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		constexpr size_t size = 32;

		m_localData.resize(size * size * size * 4);

		for ( uint32_t xIndex = 0; xIndex < size; xIndex++ )
		{
			for ( uint32_t yIndex = 0; yIndex < size; yIndex++ )
			{
				for ( uint32_t zIndex = 0; zIndex < size; zIndex++ )
				{
					const auto index = (xIndex * yIndex * size) * zIndex;

					m_localData[index] = xIndex * 8;
					m_localData[index+1] = yIndex * 8;
					m_localData[index+2] = zIndex * 8;
					m_localData[index+3] = 255;
				}
			}
		}

		return this->setLoadSuccess(false);
	}

	bool
	Texture3D::load (Resources::ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		return this->load(serviceProvider.container< ImageResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "Images"),
			true)
		);
	}

	bool
	Texture3D::load (Resources::ServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::error(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	Texture3D::load (const std::shared_ptr< ImageResource > & imageResource) noexcept
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

		Tracer::warning(ClassId, "This function is not available yet !");

		/*m_localData = imageResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the image '" << imageResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}*/

		return this->setLoadSuccess(true);
	}
}
