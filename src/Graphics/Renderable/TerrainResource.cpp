/*
 * src/Graphics/Renderable/TerrainResource.cpp
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

#include "TerrainResource.hpp"

/* STL inclusions. */
#include <thread>

/* 3rd inclusions. */
#include "magic_enum/magic_enum.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Scenes/DefinitionResource.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Scenes;

	bool
	TerrainResource::setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( materialResource == nullptr )
		{
			TraceError{ClassId} << "Material smart pointer attached to Renderable '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		/* Change the material. */
		m_material = materialResource;

		/* Checks if all is loaded */
		return this->addDependency(m_material);
	}

	void
	TerrainResource::updateVisibility (const Vector< 3, float > & worldPosition) noexcept
	{
		/* Skip if geometry is already updating. */
		if ( m_geometry->isUpdating() )
		{
			return;
		}

		const auto visibilityThreshold = m_visibleSize / 3.0F;
		const float distance = Vector< 2, float >::distance(m_lastAdaptiveGridPositionUpdated, {worldPosition[X], worldPosition[Z]});

		if ( distance > visibilityThreshold )
		{
			TraceInfo{ClassId} << "Threshold reached at " << worldPosition << "!";

			m_lastAdaptiveGridPositionUpdated = {worldPosition[X], worldPosition[Z]};

			/* Prepare the sub-grid data. */
			const auto subGrid = m_localData.subGrid(m_lastAdaptiveGridPositionUpdated, static_cast< uint32_t >(m_visibleSize));

			/* Launch update in a detached thread. */
			std::thread([geometry = m_geometry, subGrid = std::move(subGrid)]() {
				geometry->updateData(subGrid);
			}).detach();
		}
	}

	bool
	TerrainResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Create the local data. */
		if ( !m_localData.initializeByGridSize(DefaultGridSize, DefaultGridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* Create the initial adaptive geometry (visible part). */
		const auto subGrid = m_localData.subGrid({0.0F, 0.0F}, static_cast< uint32_t >(m_visibleSize));

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(serviceProvider.container< Material::StandardResource >()->getDefaultResource()) )
		{
			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath);

		if ( !rootCheck )
		{
			TraceError{ClassId} << "Unable to parse the resource file " << filepath << " !" "\n";

			return this->setLoadSuccess(false);
		}

		const auto & root = rootCheck.value();

		/* Checks if additional stores before loading (optional) */
		serviceProvider.update(root);

		if ( !root.isMember(DefinitionResource::GroundKey) )
		{
			TraceError{ClassId} << "The key '" << DefinitionResource::GroundKey << "' is not present !";

			return this->setLoadSuccess(false);
		}

		const auto & groundObject = root[DefinitionResource::GroundKey];

		if ( !groundObject.isMember(FastJSON::TypeKey) && !groundObject[FastJSON::TypeKey].isString() )
		{
			TraceError{ClassId} << "The key '" << FastJSON::TypeKey << "' is not present or not a string !";

			return this->setLoadSuccess(false);
		}

		if ( groundObject[FastJSON::TypeKey].asString() != ClassId || !groundObject.isMember(FastJSON::DataKey) )
		{
			Tracer::error(ClassId, "This file doesn't contains a Terrain definition !");

			return this->setLoadSuccess(false);
		}

		return this->load(serviceProvider, groundObject[FastJSON::DataKey]);
	}

	bool
	TerrainResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* First, we check every key from JSON data. */

		/* Checks size and division options... */
		const auto gridSize = FastJSON::getValue< float >(data, GridSizeKey).value_or(DefaultGridSize);
		const auto gridDivision = FastJSON::getValue< uint32_t >(data, GridDivisionKey).value_or(DefaultGridDivision);
		m_visibleSize = FastJSON::getValue< float >(data, GridVisibleSizeKey).value_or(DefaultVisibleSize);

		/* Checks material type. */
		const auto materialType = FastJSON::getValue< std::string >(data, MaterialTypeKey);

		if ( !materialType || materialType != Material::StandardResource::ClassId )
		{
			TraceError{ClassId} << "Material resource type '" << materialType.value() << "' for terrain '" << this->name() << "' is not handled !";

			return this->setLoadSuccess(false);
		}

		/* Then, we actually load the data. */

		/* Create the local data. */
		if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* The material. */
		std::shared_ptr< Material::Interface > materialResource{};

		auto * materials = serviceProvider.container< Material::StandardResource >();

		const auto materialName = FastJSON::getValue< std::string >(data, MaterialNameKey);

		if ( !materialName )
		{
			TraceWarning{ClassId} << "The key '" << MaterialNameKey << "' is not present or not a string !";

			materialResource = materials->getDefaultResource();
		}
		else
		{
			materialResource = materials->getResource(materialName.value());

			if ( materialResource == nullptr )
			{
				TraceError{ClassId} << "Material '" << materialName.value() << "' is not available in data stores, using default one !";

				materialResource = materials->getDefaultResource();
			}
		}

		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* After we check after optional parameters. */

		/* Checks for geometry relief generation options. */
		if ( data.isMember(HeightMapKey) )
		{
			if ( auto heightMapping = data[HeightMapKey]; heightMapping.isArray() )
			{
				auto * images = serviceProvider.container< ImageResource >();

				for ( const auto & iteration : heightMapping )
				{
					auto imageName = FastJSON::getValue< std::string >(iteration, ImageNameKey);

					if ( !imageName )
					{
						TraceWarning{ClassId} << "The key '" << ImageNameKey << "' is not present or not a string !";

						continue;
					}

					auto imageResource = images->getResource(imageName.value(), true);

					if ( imageResource == nullptr )
					{
						TraceWarning{ClassId} << "Image '" << imageName.value() << "' is not available in data stores !";

						continue;
					}

					/* Color inversion if requested. */
					const auto inverse = FastJSON::getValue< bool >(iteration, InverseKey).value_or(false);

					/* Checks for scaling. */
					const auto scale = FastJSON::getValue< float >(iteration, FastJSON::ScaleKey).value_or(1.0F);

					/* Checks the mode for leveling the vertices. */
					const auto modeString = FastJSON::getValidatedStringValue(iteration, FastJSON::ModeKey, PointTransformationModes).value_or("Replace");
					const auto mode = magic_enum::enum_cast< PointTransformationMode >(modeString).value();

					/* Applies the height map on the geometry. */
					m_localData.applyDisplacementMapping(imageResource->data(), inverse ? -scale : scale, mode);
				}
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << HeightMapKey << "' is not an array !";
			}
		}

		/* Perlin noise filtering application. */
		if ( data.isMember(PerlinNoiseKey) )
		{
			if ( auto noiseFiltering = data[PerlinNoiseKey]; noiseFiltering.isArray() )
			{
				for ( const auto & iteration : noiseFiltering )
				{
					/* Size parameter for perlin noise. */
					const auto perlinSize = FastJSON::getValue< float >(iteration, FastJSON::SizeKey).value_or(8.0F);

					/* Height scaling parameter. */
					const auto perlinScale = FastJSON::getValue< float >(iteration, FastJSON::ScaleKey).value_or(1.0F);

					/* Checks the mode for leveling the vertices. */
					const auto modeString = FastJSON::getValidatedStringValue(iteration, FastJSON::ModeKey, PointTransformationModes).value_or("Replace");
					const auto perlinMode = magic_enum::enum_cast< EmEn::Libs::VertexFactory::PointTransformationMode >(modeString).value();

					m_localData.applyPerlinNoise(perlinSize, perlinScale, perlinMode);
				}
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << PerlinNoiseKey << "' is not an array !";
			}
		}

		/* Checks if the UV multiplier parameter. */
		const auto value = FastJSON::getValue< float >(data, FastJSON::UVMultiplierKey).value_or(1.0F);

		m_localData.setUVMultiplier(value);

		// TODO: Check to enable vertex color from JSON.
		//if ( vertexColorMap != nullptr )
		//	m_geometry->enableVertexColor(vertexColorMap);

		/* Create the initial adaptive geometry (visible part). */
		const auto subGrid = m_localData.subGrid({0.0F, 0.0F}, static_cast< uint32_t >(m_visibleSize));

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions, float UVMultiplier) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Create the local data. */
		m_localData.setUVMultiplier(UVMultiplier);

		if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* Create the initial adaptive geometry (visible part). */
		const auto subGrid = m_localData.subGrid({0.0F, 0.0F}, static_cast< uint32_t >(m_visibleSize));

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		/* Set rasterization options. */
		m_rasterizationOptions = rasterizationOptions;

		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::loadDiamondSquare (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const DiamondSquareParams< float > & noise, const RasterizationOptions & rasterizationOptions, float UVMultiplier, float shiftHeight) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !isPowerOfTwo(gridDivision) )
		{
			TraceError{ClassId} << "The grid division (" << gridDivision << ") must be a power of two to use diamond square !";

			return this->setLoadSuccess(false);
		}

		/* Initialize local data. */
		if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* Apply diamond square algorithm. */
		m_localData.setUVMultiplier(UVMultiplier);
		m_localData.applyDiamondSquare(noise.factor, noise.roughness, noise.seed);

		if ( !Utility::isZero(shiftHeight) )
		{
			m_localData.shiftHeight(shiftHeight);
		}

		/* Create the initial adaptive geometry (visible part). */
		const auto subGrid = m_localData.subGrid({0.0F, 0.0F}, static_cast< uint32_t >(m_visibleSize));

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		/* Set rasterization options. */
		m_rasterizationOptions = rasterizationOptions;

		/* Set material. */
		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		TraceSuccess{ClassId} << "Terrain '" << this->name() << "' loaded!";

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::loadPerlinNoise (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const PerlinNoiseParams< float > & noise, const RasterizationOptions & rasterizationOptions, float UVMultiplier, float shiftHeight) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Initialize local data. */
		if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* Apply perlin noise. */
		m_localData.setUVMultiplier(UVMultiplier);
		m_localData.applyPerlinNoise(noise.size, noise.factor);

		if ( !Utility::isZero(shiftHeight) )
		{
			m_localData.shiftHeight(shiftHeight);
		}

		/* Create the initial adaptive geometry (visible part). */
		const auto subGrid = m_localData.subGrid({0.0F, 0.0F}, static_cast< uint32_t >(m_visibleSize));

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		/* Set rasterization options. */
		m_rasterizationOptions = rasterizationOptions;

		/* Set material. */
		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
