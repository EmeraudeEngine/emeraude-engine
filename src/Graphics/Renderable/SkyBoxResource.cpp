/*
 * src/Graphics/Renderable/SkyBoxResource.cpp
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
	SkyBoxResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(serviceProvider)) )
		{
			return this->setLoadSuccess(false);
		}

		const auto materialResource = serviceProvider.container< BasicResource >()->getOrCreateResource("DefaultSkyboxMaterial", [&serviceProvider] (BasicResource & newMaterial) {
			if ( !newMaterial.setTextureResource(serviceProvider.container< TextureResource::TextureCubemap >()->getDefaultResource()) )
			{
				return false;
			}

			return newMaterial.setManualLoadSuccess(true);
		}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(serviceProvider)) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !data.isMember(TextureKey) || !data[TextureKey].isString() )
		{
			TraceError{ClassId} << "The '" << TextureKey << "' key is not present or not a string in '" << this->name() << "' Json file ! ";

			return this->setLoadSuccess(false);
		}

		const auto textureName = data[TextureKey].asString();

		const auto materialResource = serviceProvider.container< BasicResource >()->getOrCreateResource(textureName + "SkyboxMaterial", [&] (BasicResource & newMaterial) {
			if ( !newMaterial.setTextureResource(serviceProvider.container< TextureResource::TextureCubemap >()->getResource(textureName, this->isDirectLoading())) )
			{
				return false;
			}

			return newMaterial.setManualLoadSuccess(true);
		}, ComputePrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		this->setLightPosition(FastJSON::getValue< Vector< 3, float > >(data, LightPositionKey).value_or(Vector< 3, float >::origin()));

		this->setLightAmbientColor(FastJSON::getValue< Color< float > >(data, LightAmbientColorKey).value_or(Black));

		this->setLightDiffuseColor(FastJSON::getValue< Color< float > >(data, LightDiffuseColorKey).value_or(Black));

		this->setLightSpecularColor(FastJSON::getValue< Color< float > >(data, LightSpecularColorKey).value_or(Black));

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(SkyBoxResource::getSkyBoxGeometry(serviceProvider) ) )
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
}
