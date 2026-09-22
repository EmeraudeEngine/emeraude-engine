/*
 * src/Graphics/Renderable/TerrainResource.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <algorithm>
#include <memory>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Scenes/GroundLevelInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Geometry/AdaptiveVertexGridResource.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief The terrain resource class.
	 * @extends EmEn::Graphics::Renderable::Abstract This class is a renderable object in the 3D world.
	 * @extends EmEn::Scenes::GroundLevelInterface This is the scene ground.
	 */
	class EMEN_API TerrainResource final : public Abstract, public Scenes::GroundLevelInterface
	{
		friend class Resources::Container< TerrainResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TerrainResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Complex};

			static constexpr auto DefaultGridSize{5000.0F}; /* NOTE: 5 kilometer. */
			/** @brief Side of the visible window, in METRES (4 km). Converted to a cell count against the grid's own cell size — see visibleCellCount(). */
			static constexpr auto DefaultVisibleSize{4096.0F};
			/**
			 * @brief Terrain kept AHEAD of the camera before the window slides, in metres (owner decision, 2026-09-22).
			 * @note The slide fires when the camera is closer than this to the window's edge — a distance
			 * to the EDGE, never a fraction of the window: the former `visibleSize / 3` let the edge come
			 * within 683 m before reacting, and a slide then took 1.3 s during which the camera kept
			 * closing in. With 1500 m on a 4096 m window the slide fires after 548 m of travel.
			 */
			static constexpr auto DefaultSlideMargin{1500.0F};
			/**
			 * @brief Cell of the FAR MESH, in metres (owner decision, 2026-09-22): the whole terrain at this step
			 * surrounds the streamed window, so its edge is never a picture. 0 disables it.
			 * @note Rounded to a power-of-two multiple of the grid's cell; must divide the window's sector edge.
			 */
			static constexpr auto DefaultFarCellSize{32.0F};
			/**
			 * @brief Sector of the far mesh, in metres. The window's centre snaps to this, so the hole the window
			 * leaves in the far mesh is a whole number of far sectors — no overlap, no gap, no coarse surface
			 * under the fine one to shadow or fight it.
			 */
			static constexpr auto DefaultFarSectorSize{1024.0F};
			/**
			 * @brief How far BELOW the terrain the far mesh sits, in metres (owner decision, 2026-09-22): the
			 * fine surface must win any depth contest near the camera; far away the offset is lost in the
			 * pixels. A skirt hangs from the window's edge down to the far mesh so the step is never open.
			 */
			static constexpr auto DefaultFarDepthOffset{0.5F};
			static constexpr auto DefaultGridDivision{5000U}; /* NOTE: Cell wil be 1 meter. */
			static constexpr auto DefaultUVMultiplier{5000.0F};

			/**
			 * @brief Constructs a terrain resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			TerrainResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: Abstract{serviceProvider, std::move(name), resourceFlags},
				  m_geometry{std::make_unique< Geometry::AdaptiveVertexGridResource >(serviceProvider, this->name() + "AdaptiveGrid", Geometry::EnableTangentSpace | Geometry::EnablePrimaryTextureCoordinates | Geometry::EnablePrimitiveRestart)}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::subGeometryCount() const */
			[[nodiscard]]
			uint32_t
			subGeometryCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerCount() const */
			[[nodiscard]]
			uint32_t
			layerCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::isOpaque(uint32_t) const */
			[[nodiscard]]
			bool
			isOpaque (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material != nullptr ? m_material->isOpaque() : true;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::requiresGrabPass(uint32_t) const */
			[[nodiscard]]
			bool
			requiresGrabPass (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material != nullptr ? m_material->requiresGrabPass() : false;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::geometry(uint32_t) const */
			[[nodiscard]]
			const Geometry::Interface *
			geometry (uint32_t /*LODIndex*/) const noexcept override
			{
				return m_geometry.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::material(uint32_t) const */
			[[nodiscard]]
			const Material::Interface *
			material (uint32_t /*layerIndex*/) const noexcept override
			{
				return m_material.get();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::layerRasterizationOptions(uint32_t) const */
			[[nodiscard]]
			const RasterizationOptions *
			layerRasterizationOptions (uint32_t /*layerIndex*/) const noexcept override
			{
				return &m_rasterizationOptions;
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			boundingBox () const noexcept override
			{
				return m_localData.boundingBox();
			}

			/** @copydoc EmEn::Graphics::Renderable::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			boundingSphere () const noexcept override
			{
				return m_localData.boundingSphere();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				/* The whole grid's heights on the CPU, plus the visible window's geometry (its heights, VBO, IBO). */
				return m_localData.pointCount() * sizeof(float) + (m_geometry != nullptr ? m_geometry->memoryOccupied() : 0);
			}

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getLevelAt(const Base::Math::Vector< 3, float > &) const */
			[[nodiscard]]
			float
			getLevelAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_localData.getHeightAt(worldPosition[Base::Math::X], worldPosition[Base::Math::Z]);
			}

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getLevelAt(float, float, float) const */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getLevelAt (float positionX, float positionZ, float deltaY) const noexcept override
			{
				return {positionX, m_localData.getHeightAt(positionX, positionZ) + deltaY, positionZ};
			}

			/** @copydoc EmEn::Scenes::GroundLevelInterface::getNormalAt() const */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			getNormalAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept override
			{
				return m_localData.getNormalAt(worldPosition[Base::Math::X], worldPosition[Base::Math::Z]);
			}

			/** @copydoc EmEn::Scenes::GroundLevelInterface::updateVisibility() */
			void updateVisibility (const Base::Math::Vector< 3, float > & worldPosition) noexcept override;

			/**
			 * @brief Sets how much terrain is kept ahead of the camera before the window slides, in metres.
			 * @note Clamped at use to three quarters of the half-window: a margin the window cannot honour
			 * would slide on every cycle. Negative values are taken as 0 (slide when the edge is reached).
			 * @param margin The margin, in metres.
			 * @return void
			 */
			void
			setSlideMargin (float margin) noexcept
			{
				m_slideMargin = std::max(0.0F, margin);
			}

			/**
			 * @brief Returns the terrain kept ahead of the camera before the window slides, in metres.
			 * @return float
			 */
			[[nodiscard]]
			float
			slideMargin () const noexcept
			{
				return m_slideMargin;
			}

			/**
			 * @brief Sets the far mesh around the streamed window, before loading.
			 * @param cellSize Its cell, in metres (0 = no far mesh). Rounded to a power-of-two multiple of the grid's cell.
			 * @param sectorSize Its sector, in metres; the window's centre snaps to it.
			 * @param depthOffset How far below the terrain it sits, in metres (0 = flush; a skirt closes the step otherwise).
			 * @return void
			 */
			void
			setFarMesh (float cellSize, float sectorSize, float depthOffset = DefaultFarDepthOffset) noexcept
			{
				m_farCellSize = std::max(0.0F, cellSize);
				m_farSectorSize = std::max(0.0F, sectorSize);
				m_farDepthOffset = std::max(0.0F, depthOffset);
			}

			/**
			 * @brief Loads a parametric terrain with a material.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material resource smart-pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @return bool
			 */
			bool load (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F) noexcept;

			/**
			 * @brief Loads a terrain by using parameters to generate the ground with a displacement map and a material to paint it.
			 * @tparam pixmapData_t The type used within the pixmap.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param displacementMap A pixmap to use as a displacement map.
			 * @param displacementFactor Factor of displacement.
			 * @param materialResource A reference to a material smart pointer.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @return bool
			 */
			template< typename pixmapData_t >
			bool
			load (float gridSize, uint32_t gridDivision, const Base::PixelFactory::Pixmap< pixmapData_t > & displacementMap, float displacementFactor, const std::shared_ptr< Material::Interface > & materialResource, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F) noexcept requires (std::is_arithmetic_v< pixmapData_t >)
			{
				if ( !this->beginLoading() )
				{
					return false;
				}

				/* 1. Initialize local data. */
				if ( !m_localData.initializeByGridSize(gridSize, gridDivision) )
				{
					Tracer::error(ClassId, "Unable to initialize local data !");

					return this->setLoadSuccess(false);
				}

				/* 2. Apply displacement mapping. */
				m_localData.setUVMultiplier(UVMultiplier);
				m_localData.applyDisplacementMapping(displacementMap, displacementFactor);

				/* 3. Create adaptive geometry from local data. */
				if ( !this->createGeometryFromLocalData() )
				{
					m_localData.clear();

					return this->setLoadSuccess(false);
				}

				/* 4. Set material and rasterization options. */
				m_rasterizationOptions = rasterizationOptions;

				if ( !this->setMaterial(materialResource) )
				{
					TraceError{ClassId} << "Unable to use material for Terrain '" << this->name() << "' !";

					m_localData.clear();

					return this->setLoadSuccess(false);
				}

				return this->setLoadSuccess(true);
			}

			/**
			 * @brief Loads a terrain by using parameters to generate the ground with diamond square and a material to paint it.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param noise A reference to a struct.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @param shiftHeight Apply a shift on each final height. Default none.
			 * @return bool
			 */
			bool loadDiamondSquare (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const Base::VertexFactory::DiamondSquareParams< float > & noise, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F, float shiftHeight = 0.0F) noexcept;

			/**
			 * @brief Loads a terrain by using parameters to generate the ground with perlin noise and a material to paint it.
			 * @param gridSize The size of the whole size of one dimension of the grid. I.e., If the size is 1024, the grid will be from +512 to -512.
			 * @param gridDivision How many cells in one dimension.
			 * @param materialResource A reference to a material smart pointer.
			 * @param noise A reference to a struct.
			 * @param rasterizationOptions Rasterization options. Default none.
			 * @param UVMultiplier Texture coordinates multiplier. Default 1.
			 * @param shiftHeight Apply a shift on each final height. Default none.
			 * @return bool
			 */
			bool loadPerlinNoise (float gridSize, uint32_t gridDivision, const std::shared_ptr< Material::Interface > & materialResource, const Base::VertexFactory::PerlinNoiseParams< float > & noise, const RasterizationOptions & rasterizationOptions = {}, float UVMultiplier = 1.0F, float shiftHeight = 0.0F) noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Sets a material.
			 * @param materialResource A reference to a material smart pointer.
			 * @return bool
			 */
			bool setMaterial (const std::shared_ptr< Material::Interface > & materialResource) noexcept;

			/**
			 * @brief Returns the visible window as a CELL count of the current grid.
			 * @note ⚠️ `m_visibleSize` is in metres — the slide threshold compares it to a travelled
			 * distance — while Grid::subGrid() takes a cell count. The two coincided on every scene so
			 * far only because their cells were 1 m; the cast that stood here made a 2 m grid stream a
			 * window twice as wide as asked. At least one cell.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t visibleCellCount () const noexcept;

			/**
			 * @brief Builds the geometry from the full grid: the far mesh if the numbers allow it, then the
			 * first window, centred on the origin and snapped to the far sector.
			 * @note THE single site every load path ends in. The far mesh is skipped, with the reason traced,
			 * when the grid cannot be cut to it (a cell that is not a power-of-two multiple, a division count
			 * the sectors do not divide) or when the window already covers the whole grid.
			 * @return bool
			 */
			[[nodiscard]]
			bool createGeometryFromLocalData () noexcept;

			std::shared_ptr< Geometry::AdaptiveVertexGridResource > m_geometry;
			std::shared_ptr< Material::Interface > m_material;
			Base::VertexFactory::Grid< float > m_localData;
			Base::Math::Vector< 2, float > m_windowCenter; ///< The CLAMPED centre of the window the geometry holds or is staging (X, Z), as Grid::subGridCenter() gives it.
			RasterizationOptions m_rasterizationOptions;
			float m_visibleSize{DefaultVisibleSize}; ///< Side of the streamed window, in metres.
			float m_slideMargin{DefaultSlideMargin}; ///< Terrain kept ahead of the camera before the window slides, in metres.
			float m_farCellSize{DefaultFarCellSize}; ///< Cell of the far mesh, in metres (0 = none).
			float m_farSectorSize{DefaultFarSectorSize}; ///< Sector of the far mesh, in metres; the window's centre snaps to it.
			float m_farDepthOffset{DefaultFarDepthOffset}; ///< How far below the terrain the far mesh sits, in metres.
			uint32_t m_windowSnapCells{0}; ///< The window's centre snaps to this many grid cells (the far sector); 0 = any cell.
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Terrains = Container< Graphics::Renderable::TerrainResource >;
}
