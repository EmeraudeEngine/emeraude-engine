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
#include "Libs/VertexFactory/ShapeDecimator.hpp"
#include "Graphics/Geometry/Geometries.hpp"
#include "Graphics/Material/Materials.hpp"
#include "Graphics/Material/PBRResource.hpp"
#include "Graphics/Renderer.hpp"
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

		if ( const auto materialResource = m_layers[layerIndex].material(); materialResource != nullptr )
		{
			return materialResource->isOpaque();
		}

		return true;
	}

	bool
	MeshResource::requiresGrabPass (uint32_t layerIndex) const noexcept
	{
		if ( layerIndex >= static_cast< uint32_t >(m_layers.size()) )
		{
			return false;
		}

		if ( const auto materialResource = m_layers[layerIndex].material(); materialResource != nullptr )
		{
			return materialResource->requiresGrabPass();
		}

		return false;
	}

	const Geometry::Interface *
	MeshResource::geometry (uint32_t LODLevel) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_geometryMutex};

		if ( m_geometry.empty() )
		{
			return nullptr;
		}

		const auto clamped = std::min(LODLevel, static_cast< uint32_t >(m_geometry.size() - 1));

		return m_geometry[clamped].get();
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
	MeshResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(this->serviceProvider().container< VertexResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(this->serviceProvider().container< BasicResource >()->getDefaultResource(), {}, 0) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	std::shared_ptr< Geometry::Interface >
	MeshResource::parseGeometry (const Json::Value & data) noexcept
	{
		if ( data.isMember(JKUniformScale) )
		{
			this->setUniformScale(FastJSON::getValue< float >(data, JKUniformScale).value_or(1.0F));
		}

		const auto geometryType = FastJSON::getValidatedStringValue(data, JKGeometryType, Geometry::Types).value_or(IndexedVertexResource::ClassId);
		const auto geometryResourceName = FastJSON::getValue< std::string >(data, JKGeometryName);

		if ( geometryType == VertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << JKGeometryType << "' for '" << VertexResource::ClassId << "' is not present or not a string !";

				return this->serviceProvider().container< VertexResource >()->getDefaultResource();
			}

			return this->serviceProvider().container< VertexResource >()->getResource(geometryResourceName.value());
		}

		if ( geometryType == IndexedVertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << JKGeometryType << "' for '" << IndexedVertexResource::ClassId << "' is not present or not a string !";

				return this->serviceProvider().container< IndexedVertexResource >()->getDefaultResource();
			}

			return this->serviceProvider().container< IndexedVertexResource >()->getResource(geometryResourceName.value());
		}

		TraceWarning{ClassId} << "Geometry resource type '" << geometryType << "' is not handled !";

		return this->serviceProvider().container< IndexedVertexResource >()->getDefaultResource();
	}

	std::shared_ptr< Material::Interface >
	MeshResource::parseLayer (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		const auto materialType = FastJSON::getValidatedStringValue(data, JKMaterialType, Material::Types).value_or(BasicResource::ClassId);
		const auto materialResourceName = FastJSON::getValue< std::string >(data, JKMaterialName);

		/* C++20 lambda template to load material from the appropriate container. */
		auto loadMaterial = [&] < typename ResourceType > () -> std::shared_ptr< Material::Interface >
		{
			auto * container = serviceProvider.container< ResourceType >();

			if ( !materialResourceName )
			{
				TraceError{ClassId} << "The key '" << JKMaterialName << "' for '" << ResourceType::ClassId << "' is not present or not a string !";

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

		if ( data.isMember(JKEnableDoubleSidedFace) && data[JKEnableDoubleSidedFace].isBool() )
		{
			if ( data[JKEnableDoubleSidedFace] )
			{
				layerRasterizationOptions.setCullingMode(CullingMode::None);
			}
			else
			{
				layerRasterizationOptions.setCullingMode(CullingMode::Back);
			}
		}

		if ( data.isMember(JKDrawingMode) && data[JKDrawingMode].isString() )
		{
			if ( data[JKDrawingMode] == "Fill" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Fill);
			}
			else if ( data[JKDrawingMode] == "Line" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Line);
			}
			else if ( data[JKDrawingMode] == "Point" )
			{
				layerRasterizationOptions.setPolygonMode(PolygonMode::Point);
			}
		}

		return layerRasterizationOptions;
	}

	bool
	MeshResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* FIXME: Physics properties from Mesh definitions. */
		//this->parseOptions(data);

		/* Parse geometry definition. */
		const auto geometryResource = this->parseGeometry(data);

		if ( geometryResource == nullptr )
		{
			Tracer::error(ClassId, "No suitable geometry resource found !");

			return this->setLoadSuccess(false);
		}

		this->setGeometry(geometryResource);

		/* Checks layers array presence and content. */
		if ( !data.isMember(JKLayers) )
		{
			TraceError{ClassId} << "'" << JKLayers << "' key doesn't exist !";

			return this->setLoadSuccess(false);
		}

		const auto & layerRules = data[JKLayers];

		if ( !layerRules.isArray() )
		{
			TraceError{ClassId} << "'" << JKLayers << "' key must be a JSON array !";

			return this->setLoadSuccess(false);
		}

		if ( layerRules.empty() )
		{
			TraceError{ClassId} << "'" << JKLayers << "' array is empty !";

			return this->setLoadSuccess(false);
		}

		m_layers.clear();

		for ( const auto & layerRule : layerRules )
		{
			/* Parse material definition and get default if an error occurs. */
			auto materialResource = MeshResource::parseLayer(this->serviceProvider(), layerRule);

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

		m_geometry.emplace_back(geometry);

		return this->addDependency(geometry);
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

	bool
	MeshResource::onDependenciesLoaded () noexcept
	{
		/* NOTE: Check for sub-geometries and layer count coherence. */
		if ( this->subGeometryCount() != this->layerCount() )
		{
			TraceError{ClassId} <<
				"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed! "
				"There is " << this->subGeometryCount() << " sub-geometries and " <<  this->layerCount() << " rendering layers!";

			return false;
		}

		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry(0)->isCreated() )
			{
				TraceError{ClassId} <<
					"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed! "
					"The geometry is not created!";

				return false;
			}

			/* NOTE: Check material resources. */
			const auto lCount = this->layerCount();

			for ( uint32_t layerIndex = 0; layerIndex < lCount; layerIndex++ )
			{
				if ( !this->material(layerIndex)->isCreated() )
				{
					TraceError{ClassId} <<
						"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed! "
						"The material #" << layerIndex << " is not created!";

					return false;
				}
			}
		}

		/* LOD 0 is ready — mark the renderable as instantiable immediately. */
		this->setReadyForInstantiation(true);

		/* Attempt automatic LOD generation for IndexedVertexResource with sufficient detail. */
		auto sourceGeometry = std::dynamic_pointer_cast< IndexedVertexResource >(m_geometry[0]);

		if ( sourceGeometry != nullptr )
		{
			const auto triangleCount = sourceGeometry->localData().triangles().size();

			/* Determine how many LOD levels to generate based on available detail. */
			uint32_t levelsToGenerate = 0;
			float ratio = LODReductionRatio;

			while ( levelsToGenerate < MaxLODLevels - 1 && static_cast< size_t >(static_cast< float >(triangleCount) * ratio) >= MinTrianglesPerLOD )
			{
				levelsToGenerate++;

				ratio *= LODReductionRatio;
			}

			if ( levelsToGenerate > 0 )
			{
				TraceInfo{ClassId} << "Generating " << levelsToGenerate << " LOD level(s) for '" << this->name() << "' (" << triangleCount << " triangles).";

				m_lodFutures.emplace_back(std::async(std::launch::async, [this, sourceGeometry, levelsToGenerate] {
					float levelRatio = LODReductionRatio;

					for ( uint32_t level = 1; level <= levelsToGenerate; level++ )
					{
						this->generateLODLevel(sourceGeometry, level, levelRatio);

						levelRatio *= LODReductionRatio;
					}
				}));
			}
		}

		return true;
	}

	void
	MeshResource::generateLODLevel (const std::shared_ptr< IndexedVertexResource > & sourceGeometry, uint32_t LODLevel, float ratio) noexcept
	{
		/* Decimate the source mesh (preserves multi-group structure). */
		const VertexFactory::ShapeDecimator decimator{sourceGeometry->localData(), ratio};
		auto decimatedShape = decimator.decimate();

		if ( !decimatedShape.isValid() || decimatedShape.triangles().empty() )
		{
			TraceWarning{ClassId} << "LOD " << LODLevel << " decimation failed for '" << this->name() << "'.";

			return;
		}

		/* Create a standalone geometry resource for this LOD level. */
		const auto LODName = this->name() + "_LOD" + std::to_string(LODLevel);
		auto lodGeometry = std::make_shared< IndexedVertexResource >(this->serviceProvider(), LODName, sourceGeometry->flags());

		/* load(Shape) triggers the full resource lifecycle:
		 * setLoadSuccess(true) → checkDependencies() → onDependenciesLoaded()
		 * → createOnHardware() + buildAccelerationStructure(). */
		if ( !lodGeometry->load(decimatedShape) )
		{
			TraceWarning{ClassId} << "LOD " << LODLevel << " load failed for '" << this->name() << "'.";

			return;
		}

		/* Wait for the geometry to be fully uploaded to GPU. */
		if ( !lodGeometry->isCreated() )
		{
			TraceWarning{ClassId} << "LOD " << LODLevel << " GPU upload failed for '" << this->name() << "'.";

			return;
		}

		/* Thread-safe swap into the geometry array. */
		{
			const std::lock_guard< std::mutex > lock{m_geometryMutex};

			/* Ensure sequential filling: LOD levels must be added in order. */
			if ( m_geometry.size() == LODLevel )
			{
				m_geometry.emplace_back(std::move(lodGeometry));

				TraceSuccess{ClassId} <<
					"LOD " << LODLevel << " ready for '" << this->name() << "' "
					"(" << decimatedShape.triangles().size() << " triangles, "
					<< decimatedShape.groupCount() << " group(s), "
					<< static_cast< int >(ratio * 100.0F) << "%).";
			}
		}
	}
}
