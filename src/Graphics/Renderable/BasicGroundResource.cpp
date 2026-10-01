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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "BasicGroundResource.hpp"

/* Local inclusions. */
#include "Resources/Container.hpp"
#include "FastJSON.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Scenes/DefinitionResource.hpp"
#include "Types.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
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
	BasicGroundResource::load () noexcept
	{
		/* 1. Creating a default GridGeometry. */
		const auto defaultGeometry = std::make_shared< Geometry::VertexGridResource >(this->serviceProvider(), "DefaultBasicGroundGeometry");

		if ( !defaultGeometry->load(DefaultSize, DefaultDivision) )
		{
			TraceError{ClassId} << "Unable to create default grid geometry to generate the default basic ground !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* 2. Retrieving the default material. */
		const auto defaultMaterial = this->serviceProvider().container< Material::StandardResource >()->getDefaultResource();

		if ( defaultMaterial == nullptr )
		{
			TraceError{ClassId} << "Unable to get default material to generate the default basic ground !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* 3. Use the common func. */
		return this->load(defaultGeometry, defaultMaterial);
	}

	bool
	BasicGroundResource::load (const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath);

		if ( !rootCheck )
		{
			TraceError{ClassId} << "Unable to parse the resource file " << filepath << " !" "\n";

			static_cast< void >(this->failLoading());

			return false;
		}

		const auto & root = *rootCheck;

		/* NOTE: jsoncpp's member access aborts on anything but an object (or null). */
		if ( !root.isObject() )
		{
			TraceError{ClassId} << "The resource file " << filepath << " does not hold a JSON object !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* Checks if additional stores before loading (optional) */
		this->serviceProvider().update(root);

		if ( !root.isMember(DefinitionResource::GroundKey) || !root[DefinitionResource::GroundKey].isObject() )
		{
			TraceError{ClassId} << "The key '" << DefinitionResource::GroundKey << "' is not present or not an object !";

			static_cast< void >(this->failLoading());

			return false;
		}

		const auto & groundObject = root[DefinitionResource::GroundKey];
		const auto type = FastJSON::getValue< std::string >(groundObject, FastJSON::TypeKey);

		if ( !type.has_value() )
		{
			TraceError{ClassId} << "The key '" << FastJSON::TypeKey << "' is not present or not a string !";

			static_cast< void >(this->failLoading());

			return false;
		}

		if ( *type != ClassId || !groundObject.isMember(FastJSON::DataKey) )
		{
			TraceError{ClassId} << "This file doesn't contains a basic ground definition !";

			static_cast< void >(this->failLoading());

			return false;
		}

		return this->load(groundObject[FastJSON::DataKey]);
	}

	bool
	BasicGroundResource::load (const Json::Value & data) noexcept
	{
		/* 1. Creating a geometry. */
		/* Checks size option. */
		const auto gridSize = FastJSON::getValue< float >(data, JKGridSize);

		if ( !gridSize.has_value() )
		{
			TraceError{ClassId} << "The key '" << JKGridSize << "' is not present or not a finite number !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* Checks division option. */
		const auto gridDivision = FastJSON::getValue< uint32_t >(data, JKGridDivision);

		if ( !gridDivision.has_value() )
		{
			TraceError{ClassId} << "The key '" << JKGridDivision << "' is not present or not an unsigned integer !";

			static_cast< void >(this->failLoading());

			return false;
		}

		if ( *gridDivision > Geometry::VertexGridResource::MaxGridDivision )
		{
			TraceError{ClassId} << "Basic ground '" << this->name() << "': a division of " << *gridDivision << " exceeds " << Geometry::VertexGridResource::MaxGridDivision << " !";

			static_cast< void >(this->failLoading());

			return false;
		}

		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->serviceProvider(), this->name() + "Geometry", DefaultGeometryFlags);

		if ( !geometryResource->load(*gridSize, *gridDivision) )
		{
			TraceError{ClassId} << "Unable to create grid geometry to generate the basic ground !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* 2. Check for geometry options. */
		if ( data.isMember(JKHeightMap) )
		{
			const auto & subData = data[JKHeightMap];

			if ( const auto imageName = FastJSON::getValue< std::string >(subData, JKImageName); imageName.has_value() )
			{
				const auto imageResource = this->serviceProvider().container< ImageResource >()->getResource(*imageName);

				if ( imageResource != nullptr )
				{
					/* Color inversion if requested. */
					const auto inverse = FastJSON::getValue< bool >(subData, JKInverse).value_or(false);

					/* Checks for scaling. */
					auto scale = 1.0F;

					if ( subData.isMember(JKScale) )
					{
						if ( const auto value = FastJSON::getValue< float >(subData, JKScale); value.has_value() )
						{
							scale = *value;
						}
						else
						{
							TraceWarning{ClassId} << "The key '" << JKScale << "' is not a finite number !";
						}
					}

					/* Applies the height map on the geometry. */
					geometryResource->localData().applyDisplacementMapping(imageResource->data(), inverse ? -scale : scale);
				}
				else
				{
					TraceWarning{ClassId} << "Image '" << *imageName << "' is not available in data stores !";
				}
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << JKImageName << "' is not present or not a string !";
			}
		}

		/* 3. Check material properties. */
		const auto materialType = FastJSON::getValue< std::string >(data, JKMaterialType);

		if ( !materialType.has_value() )
		{
			TraceError{ClassId} << "The key '" << JKMaterialType << "' is not present or not a string !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* NOTE: This check used to test the TYPE key a second time. */
		const auto materialName = FastJSON::getValue< std::string >(data, JKMaterialName);

		if ( !materialName.has_value() )
		{
			TraceError{ClassId} << "The key '" << JKMaterialName << "' is not present or not a string !";

			static_cast< void >(this->failLoading());

			return false;
		}

		/* Checks if the UV multiplier parameter. */
		if ( data.isMember(JKUVMultiplier) )
		{
			if ( const auto value = FastJSON::getValue< float >(data, JKUVMultiplier); value.has_value() )
			{
				geometryResource->localData().setUVMultiplier(*value);
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << JKUVMultiplier << "' is not a finite number !";
			}
		}

		/* Gets the resource from the geometry store. */
		std::shared_ptr< Material::Interface > materialResource;

		if ( *materialType == Material::StandardResource::ClassId )
		{
			materialResource = this->serviceProvider().container< Material::StandardResource >()->getResource(*materialName);
		}
		else
		{
			TraceWarning{ClassId} << "Material resource type '" << *materialType << "' for basic ground '" << this->name() << "' is not handled !";
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
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->serviceProvider(), this->name() + "GridGeometry", DefaultGeometryFlags);

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
		/* Diamond-square displaces the grid by recursive halving, so the division must be a power
		 * of two. Rather than failing on a non-power-of-two value, snap UP to the next power of two
		 * so the relief is still generated (zero-failure: the ground is always produced). */
		const auto division = nextPowerOfTwo(gridDivision);

		if ( division != gridDivision )
		{
			TraceInfo{ClassId} << "The grid division (" << gridDivision << ") is not a power of two; snapped to " << division << " for diamond-square.";
		}

		Grid< float > grid{};

		if ( grid.initializeByGridSize(gridSize, division) )
		{
			grid.setUVMultiplier(UVMultiplier);
			grid.applyDiamondSquare(noise);

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
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->serviceProvider(), this->name() + "GridGeometryDiamondSquare", DefaultGeometryFlags);

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
		const auto geometryResource = std::make_shared< Geometry::VertexGridResource >(this->serviceProvider(), this->name() + "GridGeometryPerlinNoise", DefaultGeometryFlags);

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

	bool
	BasicGroundResource::onDependenciesLoaded () noexcept
	{
		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry(0)->isCreated() )
			{
				TraceError{ClassId} << "The geometry for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}

			/* NOTE: Check material resource. */
			if ( !this->material(0)->isCreated() )
			{
				TraceError{ClassId} << "The material for '" << this->name() << "' (" << this->classLabel() << ") is not created!";

				return false;
			}
		}

		this->setReadyForInstantiation(true);

		return true;
	}
}
