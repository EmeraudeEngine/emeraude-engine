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

/* STL inclusions. */
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
#include "Tracer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture3D >::ClassId{"Texture3DContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::TextureResource::Texture3D >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::TextureResource
{
	using namespace EmEn::Libs;
	using namespace Vulkan;

	const size_t Texture3D::ClassUID{getClassUID(ClassId)};

	Texture3D::Texture3D (const std::string & name, uint32_t resourceFlags) noexcept
		: Abstract(name, resourceFlags)
	{

	}

	Texture3D::~Texture3D ()
	{
		this->destroyFromHardware();
	}

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

	size_t
	Texture3D::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	Texture3D::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	Type
	Texture3D::type () const noexcept
	{
		return Type::Texture3D;
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

	uint32_t
	Texture3D::dimensions () const noexcept
	{
		return 3;
	}

	bool
	Texture3D::isCubemapTexture () const noexcept
	{
		return false;
	}

	uint32_t
	Texture3D::frameCount () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return 1;
	}

	uint32_t
	Texture3D::duration () const noexcept
	{
		return 0;
	}

	uint32_t
	Texture3D::frameIndexAt (uint32_t /*sceneTime*/) const noexcept
	{
		return 0;
	}

	std::shared_ptr< Image >
	Texture3D::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	Texture3D::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	Texture3D::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	Texture3D::request3DTextureCoordinates () const noexcept
	{
		return true;
	}

	const char *
	Texture3D::classLabel () const noexcept
	{
		return ClassId;
	}

	bool
	Texture3D::load () noexcept
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
	Texture3D::load (const std::filesystem::path & filepath) noexcept
	{
		return this->load(Resources::Manager::instance()->container< ImageResource >()->getResource(getResourceNameFromFilepath(filepath, "Images"), true));
	}

	bool
	Texture3D::load (const Json::Value & /*data*/) noexcept
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
