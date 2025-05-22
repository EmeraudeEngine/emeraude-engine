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

/* Defining the resource manager class id. */
template<>
const char * const EmEn::Resources::Container< EmEn::Graphics::Renderable::SkyBoxResource >::ClassId{"SkyboxContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Graphics::Renderable::SkyBoxResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Graphics::Renderable
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Graphics::Material;

	const size_t SkyBoxResource::ClassUID{getClassUID(ClassId)};

	bool
	SkyBoxResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(getSkyBoxGeometry()) )
		{
			return this->setLoadSuccess(false);
		}

		auto * resources = Resources::Manager::instance();

		const auto materialResource = resources->container< BasicResource >()->getOrCreateResource("DefaultSkyboxMaterial", [resources] (BasicResource & newMaterial) {
			if ( !newMaterial.setTexture(resources->container< TextureResource::TextureCubemap >()->getDefaultResource()) )
			{
				return false;
			}

			return newMaterial.setManualLoadSuccess(true);
		}, ComputesPrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(getSkyBoxGeometry()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !data.isMember(TextureKey) || !data[TextureKey].isString() )
		{
			TraceError{ClassId} << "The '" << TextureKey << "' key is not present or not a string in '" << this->name() << "' Json file ! ";

			return this->setLoadSuccess(false);
		}

		const auto textureName = data[TextureKey].asString();

		auto * resources = Resources::Manager::instance();

		const auto materialResource = resources->container< BasicResource >()->getOrCreateResource(textureName + "SkyboxMaterial", [&] (BasicResource & newMaterial) {
			if ( !newMaterial.setTexture(resources->container< TextureResource::TextureCubemap >()->getResource(textureName, this->isDirectLoading())) )
			{
				return false;
			}

			return newMaterial.setManualLoadSuccess(true);
		}, ComputesPrimaryTextureCoordinates | PrimaryTextureCoordinatesUses3D);

		if ( !this->setMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		this->setLightPosition(FastJSON::getVector< 3, float >(data, LightPositionKey));

		this->setLightAmbientColor(FastJSON::getColor(data, LightAmbientColorKey));

		this->setLightDiffuseColor(FastJSON::getColor(data, LightDiffuseColorKey));

		this->setLightSpecularColor(FastJSON::getColor(data, LightSpecularColorKey));

		return this->setLoadSuccess(true);
	}

	bool
	SkyBoxResource::load (const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(getSkyBoxGeometry()) )
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
