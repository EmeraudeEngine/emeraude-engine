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
#include "Libs/FastJSON.hpp"
#include "Libs/VertexFactory/ShapeDecimator.hpp"
#include "Graphics/Geometry/Geometries.hpp"
#include "Graphics/Material/Materials.hpp"
#include "Graphics/Renderer.hpp"
#include "Resources/Manager.hpp"
#include "MeshResource.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics::Geometry;
	using namespace Graphics::Material;

	/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry() const */
	[[nodiscard]]
	const Geometry::Interface *
	SimpleMeshResource::geometry (uint32_t LODIndex) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_geometryMutex};

		if ( m_geometry.empty() )
		{
			return nullptr;
		}

		const auto clamped = std::min(LODIndex, static_cast< uint32_t >(m_geometry.size() - 1));

		return m_geometry[clamped].get();
	}

	bool
	SimpleMeshResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !this->setGeometry(this->serviceProvider().container< VertexResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(this->serviceProvider().container< BasicResource >()->getDefaultResource()) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SimpleMeshResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Parse geometry definition (same as MeshResource). */
		const auto geometryType = FastJSON::getValidatedStringValue(data, JKGeometryType, Geometry::Types).value_or(IndexedVertexResource::ClassId);
		const auto geometryResourceName = FastJSON::getValue< std::string >(data, JKGeometryName);

		std::shared_ptr< Geometry::Interface > geometryResource;

		if ( geometryType == VertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << JKGeometryName << "' for '" << VertexResource::ClassId << "' is not present or not a string !";

				return this->setLoadSuccess(false);
			}

			geometryResource = this->serviceProvider().container< VertexResource >()->getResource(geometryResourceName.value());
		}
		else if ( geometryType == IndexedVertexResource::ClassId )
		{
			if ( !geometryResourceName )
			{
				TraceError{ClassId} << "The key '" << JKGeometryName << "' for '" << IndexedVertexResource::ClassId << "' is not present or not a string !";

				return this->setLoadSuccess(false);
			}

			geometryResource = this->serviceProvider().container< IndexedVertexResource >()->getResource(geometryResourceName.value());
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

		if ( data.isMember(JKUniformScale) )
		{
			this->setUniformScale(FastJSON::getValue< float >(data, JKUniformScale).value_or(1.0F));
		}

		/* Parse material definition.
		 * Two formats are supported:
		 * 1. Multi-layer format (MeshResource compatible): "Layers": [ { "MaterialType": "...", ... } ]
		 *    -> Only the first layer is used.
		 * 2. Simplified format: "MaterialType": "...", "MaterialName": "..." at root level.
		 */
		const Json::Value * layerData = nullptr;

		if ( data.isMember(JKLayers) && data[JKLayers].isArray() && !data[JKLayers].empty() )
		{
			/* Multi-layer format: use first layer only. */
			layerData = &data[JKLayers][0U];
		}
		else if ( data.isMember(JKMaterialType) || data.isMember(JKMaterialName) )
		{
			/* Simplified format: material info at root level. */
			layerData = &data;
		}
		else
		{
			TraceError{ClassId} <<
				"No material definition found ! Expected '" << JKLayers << "' array "
				"or '" << JKMaterialType << "'/'" << JKMaterialName << "' keys.";

			return this->setLoadSuccess(false);
		}

		/* Parse material from the layer data. */
		const auto materialResource = MeshResource::parseLayer(this->serviceProvider(), *layerData);

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

		m_geometry.emplace_back(geometryResource);

		return this->addDependency(geometryResource);
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

	bool
	SimpleMeshResource::onDependenciesLoaded () noexcept
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

		/* LOD 0 is ready — mark the renderable as instantiable immediately. */
		this->setReadyForInstantiation(true);

		/* Attempt automatic LOD generation for IndexedVertexResource with sufficient detail.
		 * Each LOD halves the triangle count. We only generate a level if the result
		 * would still have at least MinTrianglesPerLOD triangles. */
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
	SimpleMeshResource::generateLODLevel (const std::shared_ptr< IndexedVertexResource > & sourceGeometry, uint32_t LODLevel, float ratio) noexcept
	{
		/* Decimate the source mesh. */
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
					<< static_cast< int >(ratio * 100.0F) << "%).";
			}
		}
	}
}
