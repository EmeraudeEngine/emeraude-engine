/*
 * src/Graphics/Renderable/SimpleMeshResource.cpp
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

#include "SimpleMeshResource.hpp"

/* Local inclusions. */
#include "MeshResource.hpp"
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
	SimpleMeshResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(serviceProvider.container< VertexResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(serviceProvider.container< BasicResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SimpleMeshResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Parse geometry definition (same as MeshResource). */
		const auto geometryType = FastJSON::getValidatedStringValue(data, MeshResource::GeometryTypeKey, Geometry::Types).value_or(IndexedVertexResource::ClassId);
		const auto geometryResourceName = FastJSON::getValue< std::string >(data, MeshResource::GeometryNameKey);

		std::shared_ptr< Geometry::Interface > geometryResource;

		if ( geometryType == VertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << MeshResource::GeometryNameKey << "' for '" << VertexResource::ClassId << "' is not present or not a string !";

				return this->setLoadSuccess(false);
			}

			geometryResource = serviceProvider.container< VertexResource >()->getResource(geometryResourceName.value());
		}
		else if ( geometryType == IndexedVertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << MeshResource::GeometryNameKey << "' for '" << IndexedVertexResource::ClassId << "' is not present or not a string !";

				return this->setLoadSuccess(false);
			}

			geometryResource = serviceProvider.container< IndexedVertexResource >()->getResource(geometryResourceName.value());
		}
		else
		{
			TraceWarning{ClassId} << "Geometry resource type '" << geometryType << "' is not handled !";

			return this->setLoadSuccess(false);
		}

		if ( !this->setGeometry(geometryResource) )
		{
			return this->setLoadSuccess(false);
		}

		/* Parse material definition.
		 * Two formats are supported:
		 * 1. Multi-layer format (MeshResource compatible): "Layers": [ { "MaterialType": "...", ... } ]
		 *    -> Only the first layer is used.
		 * 2. Simplified format: "MaterialType": "...", "MaterialName": "..." at root level.
		 */
		const Json::Value * layerData = nullptr;

		if ( data.isMember(MeshResource::LayersKey) && data[MeshResource::LayersKey].isArray() && !data[MeshResource::LayersKey].empty() )
		{
			/* Multi-layer format: use first layer only. */
			layerData = &data[MeshResource::LayersKey][0U];
		}
		else if ( data.isMember(MeshResource::MaterialTypeKey) || data.isMember(MeshResource::MaterialNameKey) )
		{
			/* Simplified format: material info at root level. */
			layerData = &data;
		}
		else
		{
			TraceError{ClassId} <<
				"No material definition found ! Expected '" << MeshResource::LayersKey << "' array "
				"or '" << MeshResource::MaterialTypeKey << "'/'" << MeshResource::MaterialNameKey << "' keys.";

			return this->setLoadSuccess(false);
		}

		/* Parse material from the layer data. */
		const auto materialResource = MeshResource::parseLayer(serviceProvider, *layerData);

		if ( materialResource == nullptr )
		{
			Tracer::error(ClassId, "No suitable material resource found !");

			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(materialResource) )
		{
			return this->setLoadSuccess(false);
		}

		/* Parse rasterization options. */
		m_rasterizationOptions = MeshResource::parseLayerOptions(*layerData);

		return this->setLoadSuccess(true);
	}

	bool
	SimpleMeshResource::load (const std::shared_ptr< Geometry::Interface > & geometry, const std::shared_ptr< Material::Interface > & material, const RasterizationOptions & rasterizationOptions) noexcept
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

		m_rasterizationOptions = rasterizationOptions;

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
