/*
 * src/Graphics/Renderable/MeshResource.cpp
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

#include "MeshResource.hpp"

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Graphics/Geometry/Geometries.hpp"
#include "Graphics/Material/Materials.hpp"
#include "Graphics/Material/PBRResource.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics::Geometry;
	using namespace Graphics::Material;

	bool
	MeshResource::isOpaque (uint32_t layerIndex) const noexcept
	{
		if ( layerIndex >= static_cast< uint32_t >(m_layers.size()) )
		{
			TraceError{ClassId} << "MeshResource::isOpaque(), layer index " << layerIndex << " overflow on '" << this->name() << "' !";

			layerIndex = 0;
		}

		const auto materialResource = m_layers[layerIndex].material();

		if ( materialResource != nullptr )
		{
			return materialResource->isOpaque();
		}

		return true;
	}

	const Material::Interface *
	MeshResource::material (uint32_t layerIndex) const noexcept
	{
		if ( layerIndex >= static_cast< uint32_t >(m_layers.size()) )
		{
			TraceError{ClassId} << "MeshResource::material(), layer index " << layerIndex << " overflow on '" << this->name() << "' !";

			layerIndex = 0;
		}

		return m_layers[layerIndex].material().get();
	}

	const RasterizationOptions *
	MeshResource::layerRasterizationOptions (uint32_t layerIndex) const noexcept
	{
		if ( layerIndex >= static_cast< uint32_t >(m_layers.size()) )
		{
			TraceError{ClassId} << "MeshResource::layerRasterizationOptions(), layer index " << layerIndex << " overflow on '" << this->name() << "' !";

			return nullptr;
		}

		return &m_layers[layerIndex].rasterizationOptions();
	}

	bool
	MeshResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(serviceProvider.container< VertexResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(serviceProvider.container< BasicResource >()->getDefaultResource(), {}, 0) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	std::shared_ptr< Geometry::Interface >
	MeshResource::parseGeometry (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		/* Checks size option */
		if ( data.isMember(BaseSizeKey) )
		{
			if ( data[BaseSizeKey].isNumeric() )
			{
				m_baseSize = data[BaseSizeKey].asFloat();
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << BaseSizeKey << "' must be numeric !";
			}
		}

		const auto geometryType = FastJSON::getValidatedStringValue(data, GeometryTypeKey, Geometry::Types).value_or(IndexedVertexResource::ClassId);
		const auto geometryResourceName = FastJSON::getValue< std::string >(data, GeometryNameKey);

		if ( geometryType == VertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << GeometryTypeKey << "' for '" << VertexResource::ClassId << "' is not present or not a string !";

				return serviceProvider.container< VertexResource >()->getDefaultResource();
			}

			return serviceProvider.container< VertexResource >()->getResource(geometryResourceName.value());
		}

		if ( geometryType == IndexedVertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << GeometryTypeKey << "' for '" << IndexedVertexResource::ClassId << "' is not present or not a string !";

				return serviceProvider.container< IndexedVertexResource >()->getDefaultResource();
			}

			return serviceProvider.container< IndexedVertexResource >()->getResource(geometryResourceName.value());
		}

		TraceWarning{ClassId} << "Geometry resource type '" << geometryType << "' is not handled !";

		return serviceProvider.container< IndexedVertexResource >()->getDefaultResource();
	}

	std::shared_ptr< Material::Interface >
	MeshResource::parseLayer (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		const auto materialType = FastJSON::getValidatedStringValue(data, MaterialTypeKey, Material::Types).value_or(BasicResource::ClassId);
		const auto materialResourceName = FastJSON::getValue< std::string >(data, MaterialNameKey);

		/* C++20 lambda template to load material from the appropriate container. */
		auto loadMaterial = [&] < typename ResourceType > () -> std::shared_ptr< Material::Interface >
		{
			auto * container = serviceProvider.container< ResourceType >();

			if ( !materialResourceName )
			{
				TraceError{ClassId} << "The key '" << MaterialNameKey << "' for '" << ResourceType::ClassId << "' is not present or not a string !";

				return container->getDefaultResource();
			}

			return container->getResource(materialResourceName.value());
		};

		if ( materialType == PBRResource::ClassId )
		{
			return loadMaterial.operator() < PBRResource > ();
		}

		if ( materialType == StandardResource::ClassId )
		{
			return loadMaterial.operator() < StandardResource > ();
		}

		return loadMaterial.operator() < BasicResource > ();
	}

	RasterizationOptions
	MeshResource::parseLayerOptions (const Json::Value & data) noexcept
	{
		RasterizationOptions layerRasterizationOptions{};

		if ( data.isMember(EnableDoubleSidedFaceKey) && data[EnableDoubleSidedFaceKey].isBool() )
		{
			if ( data[EnableDoubleSidedFaceKey] )
			{
				layerRasterizationOptions.setCullingMode(CullingMode::None);
			}
			else
			{
				layerRasterizationOptions.setCullingMode(CullingMode::Back);
			}
		}

		if ( data.isMember(DrawingModeKey) && data[DrawingModeKey].isString() )
		{
			if ( data[DrawingModeKey] == "Fill" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Fill);
			}
			else if ( data[DrawingModeKey] == "Line" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Line);
			}
			else if ( data[DrawingModeKey] == "Point" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Point);
			}
		}

		return layerRasterizationOptions;
	}

	bool
	MeshResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* FIXME: Physics properties from Mesh definitions. */
		//this->parseOptions(data);

		/* Parse geometry definition. */
		const auto geometryResource = this->parseGeometry(serviceProvider, data);

		if ( geometryResource == nullptr )
		{
			Tracer::error(ClassId, "No suitable geometry resource found !");

			return this->setLoadSuccess(false);
		}

		this->setGeometry(geometryResource);

		/* Checks layers array presence and content. */
		if ( !data.isMember(LayersKey) )
		{
			TraceError{ClassId} << "'" << LayersKey << "' key doesn't exist !";

			return this->setLoadSuccess(false);
		}

		const auto & layerRules = data[LayersKey];

		if ( !layerRules.isArray() )
		{
			TraceError{ClassId} << "'" << LayersKey << "' key must be a JSON array !";

			return this->setLoadSuccess(false);
		}

		if ( layerRules.empty() )
		{
			TraceError{ClassId} << "'" << LayersKey << "' array is empty !";

			return this->setLoadSuccess(false);
		}

		m_layers.clear();

		for ( const auto & layerRule : layerRules )
		{
			/* Parse material definition and get default if an error occurs. */
			auto materialResource = MeshResource::parseLayer(serviceProvider, layerRule);

			/* Gets a default material. */
			if ( materialResource == nullptr )
			{
				Tracer::error(ClassId, "No suitable material resource found !");

				return this->setLoadSuccess(false);
			}

			if ( !this->addMaterial(materialResource, MeshResource::parseLayerOptions(layerRule), 0) )
			{
				Tracer::error(ClassId, "Unable to add material layer !");

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	MeshResource::load (const std::shared_ptr< Geometry::Interface > & geometry, const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & rasterizationOptions) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* 1. Check the geometry. */
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Unable to set geometry for mesh '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		this->setGeometry(geometry);

		/* 2. Check the materials. */
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Unable to set material for mesh '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		this->setMaterial(material, rasterizationOptions, 0);

		return this->setLoadSuccess(true);
	}

	bool
	MeshResource::load (const std::shared_ptr< Geometry::Interface > & geometry, const std::vector< std::shared_ptr< Material::Interface > > & materialList, const std::vector< RasterizationOptions > & /*rasterizationOptions*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Check the geometry. */
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Unable to set geometry for mesh '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		this->setGeometry(geometry);

		/* Check the materials. */
		m_layers.clear();

		for ( const auto & material : materialList )
		{
			if ( material == nullptr )
			{
				Tracer::error(ClassId, "One material of the list is empty !");

				return this->setLoadSuccess(false);
			}

			/* TODO: Find a better solution to load a multiple layer mesh. */
			this->addMaterial(material, {}, 0);
		}

		return this->setLoadSuccess(true);
	}

	bool
	MeshResource::setGeometry (const std::shared_ptr< Geometry::Interface > & geometry) noexcept
	{
		if ( geometry == nullptr )
		{
			TraceError{ClassId} << "Geometry pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		m_geometry = geometry;

		return this->addDependency(m_geometry);
	}

	bool
	MeshResource::addMaterial (const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & options, uint32_t flags) noexcept
	{
		if ( material == nullptr )
		{
			TraceError{ClassId} << "Material pointer tried to be attached to renderable object '" << this->name() << "' " << this << " is null !";

			return false;
		}

		this->setReadyForInstantiation(false);

		const auto layerName = (std::stringstream{} << "MeshLayer" << m_layers.size()).str();

		m_layers.emplace_back(layerName, material, options, flags);

		return this->addDependency(material);
	}

	float
	MeshResource::baseSize () const noexcept
	{
		return m_baseSize;
	}

	std::shared_ptr< MeshResource >
	MeshResource::getOrCreate (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Geometry::Interface > & geometryResource, const std::shared_ptr< Material::Interface > & materialResource, std::string resourceName) noexcept
	{
		if ( resourceName.empty() )
		{
			resourceName = (std::stringstream{} << "Mesh(" << geometryResource->name() << ',' << materialResource->name() << ')').str();
		}

		return serviceProvider.container< MeshResource >()->getOrCreateResource(resourceName, [&geometryResource, &materialResource] (MeshResource & newMesh) {
			return newMesh.load(geometryResource, materialResource);
		});
	}

	std::shared_ptr< MeshResource >
	MeshResource::getOrCreate (Resources::AbstractServiceProvider & serviceProvider, const std::shared_ptr< Geometry::Interface > & geometryResource, const std::vector< std::shared_ptr< Material::Interface > > & materialResources, std::string resourceName) noexcept
	{
		if ( resourceName.empty() )
		{
			std::stringstream output;

			output << "Mesh(" << geometryResource->name();

			for ( const auto & materialResource : materialResources )
			{
				output << ',' << materialResource->name();
			}

			output << ')';

			resourceName = output.str();
		}

		return serviceProvider.container< MeshResource >()->getOrCreateResource(resourceName, [&geometryResource, &materialResources] (MeshResource & newMesh) {
			return newMesh.load(geometryResource, materialResources);
		});
	}
}
