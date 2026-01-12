/*
 * src/Graphics/Renderable/BasicSeaResource.cpp
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

#include "BasicSeaResource.hpp"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Graphics/Material/BasicResource.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;

	bool
	BasicSeaResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >("DefaultBasicSeaGeometry");

		if ( !geometryResource->load(DefaultSize, DefaultDivision) )
		{
			TraceError{ClassId} << "Unable to create default grid geometry to generate the default basic sea !";

			return this->setLoadSuccess(false);
		}

		if ( !this->setGeometry(geometryResource) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(serviceProvider.container< Material::BasicResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	BasicSeaResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* TODO... */

		return this->setLoadSuccess(false);
	}

	bool
	BasicSeaResource::load (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource, const std::shared_ptr< Material::Interface > & materialResource, float waterLevel) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* 1. Check the grid geometry. */
		if ( !this->setGeometry(geometryResource) )
		{
			TraceError{ClassId} << "Unable to use grid geometry for basic sea '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 2. Check the material. */
		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for basic sea '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 3. Set the water level height. */
		m_waterLevel = waterLevel;

		return this->setLoadSuccess(true);
	}

	bool
	BasicSeaResource::load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, float waterLevel, float UVMultiplier) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* 1. Generate the grid geometry. */
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "GridGeometry");

		if ( !geometryResource->load(gridSize, gridDivision, UVMultiplier) )
		{
			TraceError{ClassId} << "Unable to generate a basic sea geometry !";

			return this->setLoadSuccess(false);
		}

		if ( !this->setGeometry(geometryResource) )
		{
			TraceError{ClassId} << "Unable to use grid geometry for basic sea '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 2. Check the material. */
		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for basic sea '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 3. Set the water level height. */
		m_waterLevel = waterLevel;

		return this->setLoadSuccess(true);
	}

	bool
	BasicSeaResource::setGeometry (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource) noexcept
	{
		if ( geometryResource == nullptr )
		{
			TraceError{ClassId} <<
				"The geometry resource is null ! "
				"Unable to attach it to the renderable object '" << this->name() << "' " << this << ".";

			return false;
		}

		this->setReadyForInstantiation(false);

		/* Change the geometry. */
		m_geometry = geometryResource;

		/* Checks if all is loaded */
		return this->addDependency(m_geometry);
	}

	bool
	BasicSeaResource::setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( materialResource == nullptr )
		{
			TraceError{ClassId} <<
				"The material resource is null ! "
				"Unable to attach it to the renderable object '" << this->name() << "' " << this << ".";

			return false;
		}

		this->setReadyForInstantiation(false);

		/* Change the material. */
		m_material = materialResource;

		/* Checks if all is loaded */
		return this->addDependency(m_material);
	}
}
