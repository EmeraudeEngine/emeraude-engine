/*
 * src/Graphics/Renderable/AbstractBackground.cpp
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

#include "AbstractBackground.hpp"

/* Local inclusions. */
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;

	std::shared_ptr< Geometry::IndexedVertexResource >
	AbstractBackground::getSkyBoxGeometry (Resources::ServiceProvider & serviceProvider) noexcept
	{
		auto * geometries = serviceProvider.container< Geometry::IndexedVertexResource >();

		if ( geometries->isResourceLoaded(SkyBoxGeometryName) )
		{
			return geometries->getResource(SkyBoxGeometryName);
		}

		ShapeBuilderOptions< float > options{};
		options.enableGeometryFlipping(true);

		const auto shape = ShapeGenerator::generateCuboid(SkySize, SkySize, SkySize, options);

		auto geometry = std::make_shared< Geometry::IndexedVertexResource >(SkyBoxGeometryName);

		if ( !geometry->load(shape) )
		{
			return nullptr;
		}

		geometries->addResource(geometry);

		return geometry;
	}

	std::shared_ptr< Geometry::IndexedVertexResource >
	AbstractBackground::getSkyDomeGeometry (Resources::ServiceProvider & serviceProvider) noexcept
	{
		auto * geometries = serviceProvider.container< Geometry::IndexedVertexResource >();

		if ( geometries->isResourceLoaded(SkyDomeGeometryName) )
		{
			return geometries->getResource(SkyDomeGeometryName);
		}

		ShapeBuilderOptions< float > options{};
		options.enableGeometryFlipping(true);

		const auto shape = ShapeGenerator::generateSphere< float, uint32_t >(SkySize, 16, 16, options);

		auto geometry = std::make_shared< Geometry::IndexedVertexResource >(SkyBoxGeometryName);

		if ( !geometry->load(shape) )
		{
			return nullptr;
		}

		geometries->addResource(geometry);

		return geometry;
	}
}
