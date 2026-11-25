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

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <ratio>
#include <span>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Geometry/OceanSurface.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/OceanWaves.hpp"
#include "Graphics/SeaShelterMap.hpp"
#include "Math/Vector.hpp"
#include "Resources/Container.hpp"
#include "Scenes/GroundLevelInterface.hpp"
#include "Scenes/SeaLevelInterface.hpp"
#include "Tracer.hpp"
#include "VertexFactory/ShelterMap.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;

	static_assert(Geometry::OceanSurface::MaxShelterMapSide == VertexFactory::MaxShelterMapSide, "The surface takes the shelter maps the base builds.");

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

		this->setSeaLevel(parameters.seaLevel);

		if ( !this->addDependency(m_geometry) || !this->setSeaMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	void
	OceanResource::adaptToGround (const Scenes::GroundLevelInterface & ground, float boundary) noexcept
	{
		if ( m_geometry == nullptr )
		{
			TraceWarning{ClassId} << "Ocean '" << this->name() << "': presented a ground before it is loaded, its water stays the open sea everywhere.";

			return;
		}

		const auto & parameters = m_geometry->parameters();
		/* The passage and the fade of NOW: the parameters' at load, then what a console command gave. */
		const auto tuning = m_geometry->shelterTuning();
		const auto side = static_cast< size_t >(parameters.shelterMapSide);

		if ( !std::isfinite(boundary) || boundary <= 0.0F || side == 0 )
		{
			TraceWarning{ClassId} << "Ocean '" << this->name() << "': a scene boundary of " << boundary << " m (or a shelter map without texel) gives no square to read the ground over.";

			return;
		}

		const auto started = std::chrono::steady_clock::now();
		const auto cell = (2.0F * boundary) / static_cast< float >(side);

		/* The ground at the centre of each cell: rows along +Z, a row along +X (SeaShelterMap). */
		std::vector< float > heights;
		heights.reserve(side * side);

		for ( size_t row = 0; row < side; ++row )
		{
			const auto positionZ = -boundary + ((static_cast< float >(row) + 0.5F) * cell);

			for ( size_t column = 0; column < side; ++column )
			{
				const auto positionX = -boundary + ((static_cast< float >(column) + 0.5F) * cell);

				heights.push_back(ground.getLevelAt(positionX, positionZ, 0.0F)[Y]);
			}
		}

		auto map = std::make_shared< SeaShelterMap >();
		map->side = parameters.shelterMapSide;
		map->originX = -boundary;
		map->originZ = -boundary;
		map->size = 2.0F * boundary;

		const VertexFactory::ShelterMapOptions options{
			.waterLevel = parameters.seaLevel,
			.minimumPassage = std::clamp(tuning.minimumPassage / cell, 0.0F, VertexFactory::MaxShelterMapReach),
			.fade = std::clamp(tuning.fade / cell, 0.0F, VertexFactory::MaxShelterMapReach)
		};

		if ( !VertexFactory::buildShelterMap({.heights = heights, .columns = side, .rows = side}, options, map->texels) )
		{
			TraceError{ClassId} << "Ocean '" << this->name() << "': unable to tell where the ground shelters its water, it stays the open sea everywhere.";

			return;
		}

		/* What the map found, for the log: the water the open sea does not reach. */
		size_t water = 0;
		size_t sheltered = 0;

		for ( size_t index = 0; index < heights.size(); ++index )
		{
			if ( heights[index] < parameters.seaLevel )
			{
				++water;

				if ( map->texels[index] != VertexFactory::OpenWater )
				{
					++sheltered;
				}
			}
		}

		const auto cellArea = cell * cell;
		const auto elapsed = std::chrono::duration< float, std::milli >(std::chrono::steady_clock::now() - started).count();

		TraceInfo{ClassId} <<
			"Ocean '" << this->name() << "': the ground shelters " << (static_cast< float >(sheltered) * cellArea) << " m² of the " << (static_cast< float >(water) * cellArea) << " m² of water over the scene's " << (2.0F * boundary) << " m square "
			"(" << side << " cells of " << cell << " m a side, passages under " << tuning.minimumPassage << " m closed; " << elapsed << " ms).";

		m_geometry->setShelterMap(std::move(map));
	}

	std::optional< Scenes::SeaShelterSettings >
	OceanResource::shelterSettings () const noexcept
	{
		if ( m_geometry == nullptr )
		{
			return std::nullopt;
		}

		static_assert(OceanWaves::CascadeCount == 3, "Scenes::SeaShelterSettings names three shares: the swell, the waves, the ripples.");

		const auto tuning = m_geometry->shelterTuning();

		return Scenes::SeaShelterSettings{
			.minimumPassage = tuning.minimumPassage,
			.fade = tuning.fade,
			.swellShare = std::get< 0 >(tuning.waveShares),
			.waveShare = std::get< 1 >(tuning.waveShares),
			.rippleShare = std::get< 2 >(tuning.waveShares)
		};
	}

	bool
	OceanResource::setShelterSettings (const Scenes::SeaShelterSettings & settings) noexcept
	{
		if ( m_geometry == nullptr || !settings.isValid() )
		{
			return false;
		}

		m_geometry->setShelterTuning({.minimumPassage = settings.minimumPassage, .fade = settings.fade, .waveShares = {settings.swellShare, settings.waveShare, settings.rippleShare}});

		return true;
	}

	std::optional< Scenes::SeaCausticSettings >
	OceanResource::causticSettings () const noexcept
	{
		if ( m_geometry == nullptr )
		{
			return std::nullopt;
		}

		const auto tuning = m_geometry->causticTuning();

		return Scenes::SeaCausticSettings{.strength = tuning.strength, .softening = tuning.softening, .fadeDepth = tuning.fadeDepth, .temporalResponse = tuning.temporalResponse};
	}

	bool
	OceanResource::setCausticSettings (const Scenes::SeaCausticSettings & settings) noexcept
	{
		if ( m_geometry == nullptr || !settings.isValid() )
		{
			return false;
		}

		m_geometry->setCausticTuning({.strength = settings.strength, .softening = settings.softening, .fadeDepth = settings.fadeDepth, .temporalResponse = settings.temporalResponse});

		return true;
	}

	std::optional< Scenes::SeaShaftSettings >
	OceanResource::shaftSettings () const noexcept
	{
		if ( m_geometry == nullptr )
		{
			return std::nullopt;
		}

		const auto tuning = m_geometry->shaftTuning();

		return Scenes::SeaShaftSettings{.strength = tuning.strength, .anisotropy = tuning.anisotropy};
	}

	bool
	OceanResource::setShaftSettings (const Scenes::SeaShaftSettings & settings) noexcept
	{
		if ( m_geometry == nullptr || !settings.isValid() )
		{
			return false;
		}

		m_geometry->setShaftTuning({.strength = settings.strength, .anisotropy = settings.anisotropy});

		return true;
	}

	bool
	OceanResource::onDependenciesLoaded (const std::stop_token & /*stopToken*/) noexcept
	{
		this->setReadyForInstantiation(true);

		return true;
	}
}
