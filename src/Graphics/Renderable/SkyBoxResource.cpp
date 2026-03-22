/*
 * src/Graphics/Renderable/SkyBoxResource.cpp
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

#include "SkyBoxResource.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics::Material;

	bool
	SkyBoxResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider())) )
		{
			return this->setLoadSuccess(false);
		}

		auto defaultCubemapResource = this->serviceProvider().container< TextureResource::TextureCubemap >()->getDefaultResource();

		const auto material = this->serviceProvider().container< BasicResource >()
			->getOrCreateResource("DefaultSkyboxMaterial", [defaultCubemapResource] (auto & materialResource) {
				if ( !materialResource.setTextureResource(defaultCubemapResource) )
				{
					return false;
				}

				return materialResource.setManualLoadSuccess(true);
			}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		/* Store the cubemap for environment IBL access. */
		m_environmentCubemap = defaultCubemapResource;

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider())) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !data.isMember(JKTexture) || !data[JKTexture].isString() )
		{
			TraceError{ClassId} << "The '" << JKTexture << "' key is not present or not a string in '" << this->name() << "' Json file ! ";

			return this->setLoadSuccess(false);
		}

		const auto textureName = data[JKTexture].asString();

		/* Store the cubemap for environment IBL access. */
		auto cubemapResource = this->serviceProvider().container< TextureResource::TextureCubemap >()->getResource(textureName, this->isDirectLoading());

		const auto material = this->serviceProvider().container< BasicResource >()
			->getOrCreateResource(textureName + "SkyboxMaterial", [cubemapResource] (auto & materialResource) {
				if ( !materialResource.setTextureResource(cubemapResource) )
				{
					return false;
				}

				return materialResource.setManualLoadSuccess(true);
			}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		this->setLightPosition(FastJSON::getValue< Vector< 3, float > >(data, JKLightPosition).value_or(Vector< 3, float >::origin()));

		this->setLightAmbientColor(FastJSON::getValue< Color< float > >(data, JKLightAmbientColor).value_or(Black));

		this->setLightDiffuseColor(FastJSON::getValue< Color< float > >(data, JKLightDiffuseColor).value_or(Black));

		this->setLightSpecularColor(FastJSON::getValue< Color< float > >(data, JKLightSpecularColor).value_or(Black));

		/* Store the cubemap for environment IBL access. */
		m_environmentCubemap = cubemapResource;

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const std::shared_ptr< Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(this->serviceProvider()) ) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(material) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept
	{
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Geometry pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometry;

		if ( !this->addDependency(m_geometry) )
		{
			TraceError{ClassId} << "Unable to set geometry for Skybox '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyBoxResource::setMaterial (const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Material pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = material;

		if ( !this->addDependency(m_material) )
		{
			TraceError{ClassId} << "Unable to set material for Skybox '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyBoxResource::onDependenciesLoaded () noexcept
	{
		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry(0)->isCreated() )
			{
				TraceError{ClassId} << "The geometry for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}

			/* NOTE: Check material resource. */
			if ( !this->material(0)->isCreated() )
			{
				TraceError{ClassId} << "The material for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}
		}

		this->setReadyForInstantiation(true);

		return true;
	}
}
