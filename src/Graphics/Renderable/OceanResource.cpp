/*
 * src/Graphics/Renderable/OceanResource.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "OceanResource.hpp"

/* Local inclusions. */
#include "Graphics/Material/StandardResource.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;

	bool
	OceanResource::load () noexcept
	{
		return this->load(Geometry::OceanSurfaceParameters{}, this->serviceProvider().container< Material::StandardResource >()->getDefaultResource());
	}

	bool
	OceanResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by a JSON file!");

		return this->setLoadSuccess(false);
	}

	bool
	OceanResource::load (const Geometry::OceanSurfaceParameters & parameters, const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( materialResource == nullptr )
		{
			TraceError{ClassId} << "Ocean '" << this->name() << "': no material !";

			return this->setLoadSuccess(false);
		}

		auto geometry = std::make_shared< Geometry::OceanSurfaceResource >(this->serviceProvider(), this->name() + "Surface");

		if ( !geometry->load(parameters) )
		{
			TraceError{ClassId} << "Ocean '" << this->name() << "': unable to build its surface !";

			return this->setLoadSuccess(false);
		}

		this->setReadyForInstantiation(false);

		m_geometry = std::move(geometry);
		m_material = materialResource;
		m_waterLevel = parameters.seaLevel;

		if ( !this->addDependency(m_geometry) || !this->addDependency(m_material) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	OceanResource::onDependenciesLoaded () noexcept
	{
		this->setReadyForInstantiation(true);

		return true;
	}
}
