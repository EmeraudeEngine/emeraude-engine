/*
 * src/Graphics/Renderable/TerrainResource.cpp
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

#include "TerrainResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <utility>

/* Third-party inclusions. */
#include "magic_enum/magic_enum.hpp"

/* Local inclusions. */
#include "Resources/Container.hpp"
#include "FastJSON.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "PrimaryServices.hpp"
#include "Scenes/DefinitionResource.hpp"
#include "ThreadPool.hpp"
#include "Types.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
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

	uint32_t
	TerrainResource::visibleCellCount () const noexcept
	{
		const auto quadSize = m_localData.quadSize();

		if ( quadSize <= 0.0F )
		{
			return 1;
		}

		return std::max(1U, static_cast< uint32_t >(std::round(m_visibleSize / quadSize)));
	}

	void
	TerrainResource::updateVisibility (const Vector< 3, float > & worldPosition) noexcept
	{
		/* Logic thread, once per cycle. One slide in flight at a time. */
		if ( m_geometry->isUpdating() )
		{
			return;
		}

		/* The slide fires on the distance to the window's EDGE: the margin is what the camera must
		 * always have ahead of it. A margin the window cannot honour is pulled back (a window can
		 * only be recentred, never widened here). */
		const auto halfWindow = m_visibleSize * 0.5F;
		const auto margin = std::min(m_slideMargin, halfWindow * 0.75F);
		const auto distanceToCenter = Vector< 2, float >::distance(m_windowCenter, {worldPosition[X], worldPosition[Z]});

		if ( distanceToCenter <= halfWindow - margin )
		{
			return;
		}

		/* Where the window CAN go: snapped and clamped the way the extraction will do it. The slide is
		 * worth its 896 MiB only if that window differs from the one we hold by more than the slack
		 * itself. ⚠️ Against the grid border the camera stays past the threshold for ever (the window
		 * cannot follow it), and testing the wanted centre for mere INEQUALITY regenerated the whole
		 * window for every metre the camera drifted along the border — four 896 MiB uploads in eight
		 * seconds, measured 2026-09-22. The distance between the two centres is the honest test. */
		const auto cellCount = this->visibleCellCount();
		const auto wantedCenter = m_localData.subGridCenter({worldPosition[X], worldPosition[Z]}, cellCount);

		if ( Vector< 2, float >::distance(wantedCenter, m_windowCenter) <= halfWindow - margin )
		{
			return;
		}

		/* The slot is claimed HERE, before the work leaves this thread: the next cycle must see it
		 * taken even if the worker has not started yet. */
		if ( !m_geometry->beginUpdate() )
		{
			return;
		}

		TraceInfo{ClassId} <<
			"Terrain '" << this->name() << "' slides its window to (" << wantedCenter[0] << ", " << wantedCenter[1] << "): "
			"the camera is " << (halfWindow - distanceToCenter) << " m from the edge (margin " << margin << " m).";

		m_windowCenter = wantedCenter;

		/* The engine's pool, never a raw std::thread: its constructor throws on exhaustion and the
		 * cascade is built without exceptions. The worker EXTRACTS the window from the full grid (a
		 * read of data nothing mutates after load — 26 ms that used to stall the logic tick), builds
		 * and STAGES it; the render thread publishes it (AdaptiveVertexGridResource::updateVideoMemory()). */
		const auto threadPool = this->serviceProvider().primaryServices().threadPool();

		const auto enqueued = threadPool != nullptr && threadPool->enqueue([self = std::static_pointer_cast< TerrainResource >(this->shared_from_this()), geometry = m_geometry, wantedCenter, cellCount] () {
			if ( !geometry->updateData(self->m_localData.subGrid(wantedCenter, cellCount)) )
			{
				TraceError{ClassId} << "Unable to stage the new terrain window of '" << geometry->name() << "' !";
			}
		});

		if ( !enqueued )
		{
			TraceError{ClassId} << "Unable to hand the terrain window of '" << this->name() << "' to the thread pool !";

			m_geometry->cancelUpdate();
		}
	}

	bool
	TerrainResource::load () noexcept
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
		m_windowCenter = m_localData.subGridCenter({0.0F, 0.0F}, this->visibleCellCount());

		const auto subGrid = m_localData.subGrid(m_windowCenter, this->visibleCellCount());

		if ( !m_geometry->load(subGrid) )
		{
			Tracer::error(ClassId, "Unable to create adaptive grid from local data !");

			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		if ( !this->setMaterial(this->serviceProvider().container< Material::StandardResource >()->getDefaultResource()) )
		{
			m_localData.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	TerrainResource::load (const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath);

		if ( !rootCheck )
		{
			TraceError{ClassId} << "Unable to parse the resource file " << filepath << " !" "\n";

			return this->setLoadSuccess(false);
		}

		const auto & root = rootCheck.value();

		/* Checks if additional stores before loading (optional) */
		this->serviceProvider().update(root);

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

		return this->load(groundObject[FastJSON::DataKey]);
	}

	bool
	TerrainResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* First, we check every key from JSON data. */

		/* Checks size and division options... */
		const auto gridSize = FastJSON::getValue< float >(data, JKGridSize).value_or(DefaultGridSize);
		const auto gridDivision = FastJSON::getValue< uint32_t >(data, JKGridDivision).value_or(DefaultGridDivision);
		m_visibleSize = FastJSON::getValue< float >(data, JKGridVisibleSize).value_or(DefaultVisibleSize);
		this->setSlideMargin(FastJSON::getValue< float >(data, JKGridSlideMargin).value_or(DefaultSlideMargin));

		/* Checks material type. */
		const auto materialType = FastJSON::getValue< std::string >(data, JKMaterialType);

		if ( !materialType || materialType != Material::StandardResource::ClassId )
		{
			TraceError{ClassId} << "Material resource type '" << materialType.value_or("<missing>") << "' for terrain '" << this->name() << "' is not handled !";

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
		const auto materialName = FastJSON::getValue< std::string >(data, JKMaterialName);

		const auto resolveMaterial = [&] (auto * materials) -> std::shared_ptr< Material::Interface > {
			if ( !materialName )
			{
				TraceWarning{ClassId} << "The key '" << JKMaterialName << "' is not present or not a string !";

				return materials->getDefaultResource();
			}

			if ( auto resource = materials->getResource(materialName.value()); resource != nullptr )
			{
				return resource;
			}

			TraceError{ClassId} << "Material '" << materialName.value() << "' is not available in data stores, using default one !";

			return materials->getDefaultResource();
		};

		const auto materialResource = resolveMaterial(this->serviceProvider().container< Material::StandardResource >());

		if ( !this->setMaterial(materialResource) )
		{
			TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* After we check after optional parameters. */

		/* Checks for geometry relief generation options. */
		if ( data.isMember(JKHeightMap) )
		{
			if ( auto heightMapping = data[JKHeightMap]; heightMapping.isArray() )
			{
				auto * images = this->serviceProvider().container< ImageResource >();

				for ( const auto & iteration : heightMapping )
				{
					auto imageName = FastJSON::getValue< std::string >(iteration, JKImageName);

					if ( !imageName )
					{
						TraceWarning{ClassId} << "The key '" << JKImageName << "' is not present or not a string !";

						continue;
					}

					auto imageResource = images->getResource(imageName.value(), true);

					if ( imageResource == nullptr )
					{
						TraceWarning{ClassId} << "Image '" << imageName.value() << "' is not available in data stores !";

						continue;
					}

					/* Color inversion if requested. */
					const auto inverse = FastJSON::getValue< bool >(iteration, JKInverse).value_or(false);

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
				TraceWarning{ClassId} << "The key '" << JKHeightMap << "' is not an array !";
			}
		}

		/* Perlin noise filtering application. */
		if ( data.isMember(JKPerlinNoise) )
		{
			if ( auto noiseFiltering = data[JKPerlinNoise]; noiseFiltering.isArray() )
			{
				for ( const auto & iteration : noiseFiltering )
				{
					/* Size parameter for perlin noise. */
					const auto perlinSize = FastJSON::getValue< float >(iteration, FastJSON::SizeKey).value_or(8.0F);

					/* Height scaling parameter. */
					const auto perlinScale = FastJSON::getValue< float >(iteration, FastJSON::ScaleKey).value_or(1.0F);

					/* Checks the mode for leveling the vertices. */
					const auto modeString = FastJSON::getValidatedStringValue(iteration, FastJSON::ModeKey, PointTransformationModes).value_or("Replace");
					const auto perlinMode = magic_enum::enum_cast< EmEn::Base::VertexFactory::PointTransformationMode >(modeString).value();

					m_localData.applyPerlinNoise(perlinSize, perlinScale, perlinMode);
				}
			}
			else
			{
				TraceWarning{ClassId} << "The key '" << JKPerlinNoise << "' is not an array !";
			}
		}

		/* Checks if the UV multiplier parameter. */
		const auto value = FastJSON::getValue< float >(data, FastJSON::UVMultiplierKey).value_or(1.0F);

		m_localData.setUVMultiplier(value);

		// TODO: Check to enable vertex color from JSON.
		//if ( vertexColorMap != nullptr )
		//	m_geometry->enableVertexColor(vertexColorMap);

		/* Create the initial adaptive geometry (visible part). */
		m_windowCenter = m_localData.subGridCenter({0.0F, 0.0F}, this->visibleCellCount());

		const auto subGrid = m_localData.subGrid(m_windowCenter, this->visibleCellCount());

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
		m_windowCenter = m_localData.subGridCenter({0.0F, 0.0F}, this->visibleCellCount());

		const auto subGrid = m_localData.subGrid(m_windowCenter, this->visibleCellCount());

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

		/* Diamond-square displaces the grid by recursive halving, so the division must be a power
		 * of two. Rather than failing on a non-power-of-two value, snap UP to the next power of two
		 * so the relief is still generated (zero-failure: the terrain is always produced). */
		const auto division = nextPowerOfTwo(gridDivision);

		if ( division != gridDivision )
		{
			TraceInfo{ClassId} << "The grid division (" << gridDivision << ") is not a power of two; snapped to " << division << " for diamond-square.";
		}

		/* Initialize local data. */
		if ( !m_localData.initializeByGridSize(gridSize, division) )
		{
			Tracer::error(ClassId, "Unable to initialize local data !");

			return this->setLoadSuccess(false);
		}

		/* Apply diamond square algorithm. */
		m_localData.setUVMultiplier(UVMultiplier);
		m_localData.applyDiamondSquare(noise);

		if ( !Utility::isZero(shiftHeight) )
		{
			m_localData.shiftHeight(shiftHeight);
		}

		/* Create the initial adaptive geometry (visible part). */
		m_windowCenter = m_localData.subGridCenter({0.0F, 0.0F}, this->visibleCellCount());

		const auto subGrid = m_localData.subGrid(m_windowCenter, this->visibleCellCount());

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
		m_windowCenter = m_localData.subGridCenter({0.0F, 0.0F}, this->visibleCellCount());

		const auto subGrid = m_localData.subGrid(m_windowCenter, this->visibleCellCount());

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

	bool
	TerrainResource::onDependenciesLoaded () noexcept
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
