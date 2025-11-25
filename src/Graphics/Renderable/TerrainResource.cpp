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

	/*void
	Terrain::updateVisibility (const CartesianFrame< float > & coordinates) noexcept
	{
		/ * Checks if the current position still close to the center of sub-data. * /
		const auto position = coordinates.position();

		/ * Creates a new visible grid if position overlap the limit of the existing one. * /
		if ( Vector< 3, float >::distance(m_lastUpdatePosition, position) > m_geometry->getMinimalUpdateDistance() )
		{
			if ( !m_updatingActiveGeometryProcess )
			{
				m_lastUpdatePosition = position;

				std::thread process(&Terrain::updateActiveGeometryProcess, this);

				process.detach();
			}
			else
			{
				Tracer::warning(ClassId, "A new grid is already currently loading. Maybe the dimension of the active grid is too small !");
			}
		}

		/ * Updates the visibility of the active subgrid. * /
		m_geometry->updateVisibility(coordinates);
	}*/

	bool
	TerrainResource::setGeometry (const std::shared_ptr< Geometry::AdaptiveVertexGridResource > & geometryResource) noexcept
	{
		if ( geometryResource == nullptr )
		{
			TraceError{ClassId} << "Geometry smart pointer attached to Renderable '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		/* Change the material. */
		m_geometry = geometryResource;

		/* Checks if all is loaded */
		return this->addDependency(m_geometry);
	}

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
	TerrainResource::updateActiveGeometryProcess () noexcept
	{
		m_updatingActiveGeometryProcess = true;

		Tracer::info(ClassId, "Update process started...");

		/* This will processLogics local data from AdaptiveGridGeometry. */
		if ( m_geometry->updateLocalData(m_localData, m_lastUpdatePosition) )
		{
			//this->setRequestingVideoMemoryUpdate();
		}

		m_updatingActiveGeometryProcess = false;
	}

	bool
	TerrainResource::prepareGeometry (float size, uint32_t division) noexcept
	{
		/* 1. Create the local data. */
		if ( !m_localData.initializeData(size, division) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return false;
		}

		/* 2. Create the adaptive geometry (visible part). */
		if ( !m_geometry->load(m_localData, division / 4, {0.0F, 0.0F, 0.0F}) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return false;
		}

		return true;
	}

	bool
	TerrainResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->prepareGeometry(DefaultSize, DefaultDivision) )
		{
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

		if ( !root.isMember(DefinitionResource::SceneAreaKey) )
		{
			TraceError{ClassId} << "The key '" << DefinitionResource::SceneAreaKey << "' is not present !";

			return this->setLoadSuccess(false);
		}

		const auto & sceneAreaObject = root[DefinitionResource::SceneAreaKey];

		if ( !sceneAreaObject.isMember(FastJSON::TypeKey) && !sceneAreaObject[FastJSON::TypeKey].isString() )
		{
			TraceError{ClassId} << "The key '" << FastJSON::TypeKey << "' is not present or not a string !";

			return this->setLoadSuccess(false);
		}

		if ( sceneAreaObject[FastJSON::TypeKey].asString() != ClassId || !sceneAreaObject.isMember(FastJSON::DataKey) )
		{
			Tracer::error(ClassId, "This file doesn't contains a Terrain definition !");

			return this->setLoadSuccess(false);
		}

		return this->load(serviceProvider, sceneAreaObject[FastJSON::DataKey]);
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
		const auto size = FastJSON::getValue< float >(data, FastJSON::SizeKey).value_or(DefaultSize);
		const auto division = FastJSON::getValue< uint32_t >(data, FastJSON::DivisionKey).value_or(DefaultDivision);

		/* Checks material type. */
		const auto materialType = FastJSON::getValue< std::string >(data, MaterialTypeKey);

		if ( !materialType || materialType != Material::StandardResource::ClassId )
		{
			TraceError{ClassId} << "Material resource type '" << materialType.value() << "' for terrain '" << this->name() << "' is not handled !";

			return this->setLoadSuccess(false);
		}

		/* Then, we actually load the data. */

		/* The geometry. */
		if ( !m_localData.initializeData(size, division) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return false;
		}

		/* Checks for vertex color. */
		/*std::shared_ptr< Image > vertexColorMap;

		if ( data.isMember(VertexColorKey) )
		{
			auto vertexColorData = data[VertexColorKey];

			if ( vertexColorData.isObject() )
			{
				auto imageName = FastJSON::getString(vertexColorData, ImageNameKey);

				if ( !imageName.empty() )
				{
					vertexColorMap = resources.images().get(imageName, true);

					if ( vertexColorMap == nullptr )
						Tracer::warning(ClassId, Blob() << "Image '" << imageName << "' is not available in data stores !");
				}
				else
				{
					Tracer::warning(ClassId, Blob() << "The key '" << ImageNameKey << "' is not present or not a string !");
				}
			}
			else
			{
				Tracer::warning(ClassId, Blob() << "The key '" << VertexColorKey << "' must be an object !");
			}
		}

		m_farGeometry = std::make_shared< VertexGridResource >(this->name() + "FarGeometry");

		if ( vertexColorMap != nullptr )
			m_farGeometry->enableVertexColor(vertexColorMap);*/

		if ( !m_farGeometry->load(size, division / 64U) )
		{
			Tracer::error(ClassId, "Unable to create the far geometry !");

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

					m_farGeometry->localData().applyDisplacementMapping(imageResource->data(), inverse ? -scale : scale, mode);
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
			auto noiseFiltering = data[PerlinNoiseKey];

			if ( noiseFiltering.isArray() )
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

					m_farGeometry->localData().applyPerlinNoise(perlinSize, perlinScale, perlinMode);
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

		m_farGeometry->localData().setUVMultiplier(value);

		/* Checks if all is loaded */
		if ( !this->addDependency(m_farGeometry) )
		{
			return this->setLoadSuccess(false);
		}

		//if ( vertexColorMap != nullptr )
		//	m_geometry->enableVertexColor(vertexColorMap);

		/* Creates the adaptive geometry (visible part). */
		if ( !m_geometry->load(m_localData, division / 16U, {0.0F, 0.0F, 0.0F}) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::load (float size, uint32_t division, const std::shared_ptr< Material::Interface > & material) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->prepareGeometry(size, division) )
		{
			Tracer::error(ClassId, "Unable to prepare the geometry to generate the Terrain !");

			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(material) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
