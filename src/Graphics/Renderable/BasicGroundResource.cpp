/*
 * src/Graphics/Renderable/BasicGroundResource.cpp
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

#include "BasicGroundResource.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Scenes/DefinitionResource.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Scenes;

	float
	BasicGroundResource::getLevelAt (const Vector< 3, float > & worldPosition) const noexcept
	{
		/* NOTE: If no geometry available,
		 * we send the -Y boundary limit. */
		if ( m_geometry == nullptr )
		{
			return 0.0F;
		}

		return m_geometry->localData().getHeightAt(worldPosition[X], worldPosition[Z]);
	}

	Vector< 3, float >
	BasicGroundResource::getLevelAt (float positionX, float positionZ, float deltaY) const noexcept
	{
		if ( m_geometry == nullptr )
		{
			return {positionX, 0.0F + deltaY, positionZ};
		}

		return {positionX, m_geometry->localData().getHeightAt(positionX, positionZ) + deltaY, positionZ};
	}

	Vector< 3, float >
	BasicGroundResource::getNormalAt (const Vector< 3, float > & worldPosition) const noexcept
	{
		if ( m_geometry == nullptr )
		{
			return Vector< 3, float >::positiveY();
		}

		return m_geometry->localData().getNormalAt(worldPosition[X], worldPosition[Z]);
	}

	bool
	BasicGroundResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		/* 1. Creating a default GridGeometry. */
		const auto defaultGeometry = std::make_shared< Geometry::VertexGridResource >("DefaultBasicGroundGeometry");

		if ( !defaultGeometry->load(DefaultSize, DefaultDivision) )
		{
			TraceError{ClassId} << "Unable to create default grid geometry to generate the default basic ground !";

			return this->setLoadSuccess(false);
		}

		/* 2. Retrieving the default material. */
		const auto defaultMaterial = serviceProvider.container< Material::BasicResource >()->getDefaultResource();

		if ( defaultMaterial == nullptr )
		{
			TraceError{ClassId} << "Unable to get default material to generate the default basic ground !";

			return this->setLoadSuccess(false);
		}

		/* 3. Use the common func. */
		return this->load(defaultGeometry, defaultMaterial);
	}

	bool
	BasicGroundResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
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
			TraceError{ClassId} << "This file doesn't contains a basic ground definition !";

			return this->setLoadSuccess(false);
		}

		return this->load(serviceProvider, groundObject[FastJSON::DataKey]);
	}

	bool
	BasicGroundResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		/* 1. Creating a geometry. */
		/* Checks size option. */
		if ( !data.isMember(SizeKey) || !data[SizeKey].isNumeric() )
		{
			TraceError{ClassId} << "The key '" << SizeKey << "' is not present or not numeric !";

			return this->setLoadSuccess(false);
		}

		/* Checks division option. */
		if ( !data.isMember(DivisionKey) || !data[DivisionKey].isNumeric() )
		{
			TraceError{ClassId} << "The key '" << DivisionKey << "' is not present or not numeric !";

			return this->setLoadSuccess(false);
		}

		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "Geometry");

		if ( !geometryResource->load(data[SizeKey].asFloat(), data[DivisionKey].asUInt(), DefaultGeometryFlags) )
		{
			TraceError{ClassId} << "Unable to create grid geometry to generate the basic ground !";

			return this->setLoadSuccess(false);
		}

		/* 2. Check for geometry options. */
		if ( data.isMember(HeightMapKey) )
		{
			const auto & subData = data[HeightMapKey];

			if ( subData.isMember(ImageNameKey) && subData[ImageNameKey].isString() )
			{
				const auto imageName = subData[ImageNameKey].asString();

				const auto imageResource = serviceProvider.container< ImageResource >()->getResource(imageName);

				if ( imageResource != nullptr )
				{
					/* Color inversion if requested. */
					auto inverse = false;

					if ( subData.isMember(InverseKey) )
					{
						inverse = subData[InverseKey].asBool();
					}

					/* Checks for scaling. */
					auto scale = 1.0F;

					if ( subData.isMember(ScaleKey) )
					{
						if ( subData[ScaleKey].isNumeric() )
						{
							scale = subData[ScaleKey].asFloat();
						}
						else
						{
							TraceWarning{ClassId} << "The key '" << ScaleKey << "' is not numeric !";
						}
					}

					/* Applies the height map on the geometry. */
					geometryResource->localData().applyDisplacementMapping(imageResource->data(), inverse ? -scale : scale);
				}
				else
				{
					TraceWarning{ClassId} << "Image '" << imageName << "' is not available in data stores !";
				}
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << ImageNameKey << "' is not present or not a string !";
			}
		}

		/* 3. Check material properties. */
		if ( !data.isMember(MaterialTypeKey) || !data[MaterialTypeKey].isString() )
		{
			TraceError{ClassId} << "The key '" << MaterialTypeKey << "' is not present or not a string !";

			return this->setLoadSuccess(false);
		}

		if ( !data.isMember(MaterialNameKey) || !data[MaterialTypeKey].isString() )
		{
			TraceError{ClassId} << "The key '" << MaterialNameKey << "' is not present or not a string !";

			return this->setLoadSuccess(false);
		}

		/* Checks if the UV multiplier parameter. */
		if ( data.isMember(UVMultiplierKey) )
		{
			if ( data[UVMultiplierKey].isNumeric() )
			{
				geometryResource->localData().setUVMultiplier(data[UVMultiplierKey].asFloat());
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << UVMultiplierKey << "' is not numeric !";
			}
		}

		/* Gets the resource from the geometry store. */
		std::shared_ptr< Material::Interface > materialResource;

		const auto materialType = data[MaterialTypeKey].asString();

		if ( materialType == Material::StandardResource::ClassId )
		{
			const auto name = data[MaterialNameKey].asString();

			materialResource = serviceProvider.container< Material::StandardResource >()->getResource(name);
		}
		else
		{
			TraceWarning{ClassId} << "Material resource type '" << materialType << "' for basic ground '" << this->name() << "' is not handled !";
		}

		/* 3. Use the common func. */
		return this->load(geometryResource, materialResource);
	}

	bool
	BasicGroundResource::load (const std::shared_ptr< Geometry::VertexGridResource > & vertexGridResource, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* 1. Check the grid geometry. */
		if ( !this->setGeometry(vertexGridResource) )
		{
			TraceError{ClassId} << "Unable to set grid geometry for basic ground '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 2. Check the material. */
		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to set material for basic ground '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* 3. Set rasterization options. */
		m_rasterizationOptions = rasterizationOptions;

		return this->setLoadSuccess(true);
	}

	bool
	BasicGroundResource::load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions, float UVMultiplier) noexcept
	{
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "GridGeometry", DefaultGeometryFlags);

		if ( !geometryResource->load(gridSize, gridDivision, UVMultiplier) )
		{
			Tracer::error(ClassId, "Unable to generate a basic ground geometry !");

			return false;
		}

		return this->load(geometryResource, materialResource, rasterizationOptions);
	}

	bool
	BasicGroundResource::loadDiamondSquare (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const DiamondSquareParams< float > & noise, const RasterizationOptions & rasterizationOptions, float UVMultiplier, float shiftHeight) noexcept
	{
		if ( !isPowerOfTwo(gridDivision) )
		{
			TraceError{ClassId} << "The grid division (" << gridDivision << ") must be a power of two to use diamond square!";

			return false;
		}

		Grid< float > grid{};

		if ( grid.initializeByGridSize(gridSize, gridDivision) )
		{
			grid.setUVMultiplier(UVMultiplier);
			grid.applyDiamondSquare(noise.factor, noise.roughness, noise.seed);

			if ( !Utility::isZero(shiftHeight) )
			{
				grid.shiftHeight(shiftHeight);
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to generate a grid shape !");

			return false;
		}

		/* Create the geometry resource from the shape. */
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "GridGeometryDiamondSquare", DefaultGeometryFlags);

		if ( !geometryResource->load(grid) )
		{
			Tracer::error(ClassId, "Unable to generate a basic ground geometry !");

			return false;
		}

		return this->load(geometryResource, materialResource, rasterizationOptions);
	}

	bool
	BasicGroundResource::loadPerlinNoise (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const PerlinNoiseParams< float > & noise, const RasterizationOptions & rasterizationOptions, float UVMultiplier, float shiftHeight) noexcept
	{
		Grid< float > grid{};

		if ( grid.initializeByGridSize(gridSize, gridDivision) )
		{
			grid.setUVMultiplier(UVMultiplier);
			grid.applyPerlinNoise(noise.size, noise.factor);

			if ( !Utility::isZero(shiftHeight) )
			{
				grid.shiftHeight(shiftHeight);
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to generate a grid shape !");

			return false;
		}

		/* Create the geometry resource from the shape. */
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->name() + "GridGeometryPerlinNoise", DefaultGeometryFlags);

		if ( !geometryResource->load(grid) )
		{
			Tracer::error(ClassId, "Unable to generate a basic ground geometry !");

			return false;
		}

		return this->load(geometryResource, materialResource, rasterizationOptions);
	}

	bool
	BasicGroundResource::setGeometry (const std::shared_ptr< Geometry::VertexGridResource > & geometryResource) noexcept
	{
		if ( geometryResource == nullptr )
		{
			TraceError{ClassId} << "Geometry pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometryResource;

		return this->addDependency(m_geometry);
	}

	bool
	BasicGroundResource::setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept
	{
		if ( materialResource == nullptr )
		{
			TraceError{ClassId} << "Material pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_material = materialResource;

		return this->addDependency(m_material);
	}
}
