/*
 * src/Graphics/Renderable/SimpleMeshResource.cpp
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

#include "SimpleMeshResource.hpp"

/* Local inclusions. */
#include "Graphics/Geometry/VertexResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;

	bool
	SimpleMeshResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(serviceProvider.container< Geometry::VertexResource >()->getRandomResource()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(serviceProvider.container< Material::BasicResource >()->getRandomResource()) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SimpleMeshResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}

	bool
	SimpleMeshResource::load (const std::shared_ptr< Geometry::Interface > & geometry, const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(geometry) )
		{
			return this->setLoadSuccess(false);
		}

		if ( material != nullptr )
		{
			if ( !this->setMaterial(material) )
			{
				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	SimpleMeshResource::setGeometry (const std::shared_ptr< Geometry::Interface > & geometryResource) noexcept
	{
		if ( geometryResource == nullptr )
		{
			TraceError{ClassId} <<
				"The geometry resource is null ! "
				"Unable to attach it to the renderable object '" << this->name() << "' " << this << ".";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometryResource;

		return this->addDependency(m_geometry);
	}

	bool
	SimpleMeshResource::setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( materialResource == nullptr )
		{
			TraceError{ClassId} <<
				"The material resource is null ! "
				"Unable to attach it to the renderable object '" << this->name() << "' " << this << ".";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = materialResource;

		return this->addDependency(m_material);
	}

	std::shared_ptr< SimpleMeshResource >
	SimpleMeshResource::getOrCreate (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Geometry::Interface > & geometryResource, const std::shared_ptr< Material::Interface > & materialResource, std::string resourceName) noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = (std::stringstream{} << "Mesh(" << geometryResource->name() << ',' << materialResource->name() << ')').str();
		}

		return serviceProvider.container< SimpleMeshResource >()->getOrCreateResource(resourceName, [&geometryResource, &materialResource] (SimpleMeshResource & newMesh) {
			return newMesh.load(geometryResource, materialResource);
		});
	}
}
