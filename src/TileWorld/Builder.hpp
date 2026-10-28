/*
 * src/TileWorld/Builder.hpp
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
#include <cstdint>
#include <iterator> // IWYU pragma: keep (std::pair: IWYU adds then removes it across the TUs, libstdc++ mapping)
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "VertexFactory/Shape.hpp"
#include "VertexFactory/ShapeBuilder.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics::Material
	{
		class Interface;
	}

	namespace Scenes
	{
		class Toolkit;
	}

	namespace TileWorld
	{
		class Layout;
		class Relief;
		class SurfaceSink;
		class TileInterface;
		struct TileContext;
	}
}

namespace EmEn::TileWorld
{
	/** @brief A surface the tiles write into: its name, its material, the feel of its contacts. */
	struct SurfaceSlot
	{
		std::string name;
		std::shared_ptr< Graphics::Material::Interface > material;
		float bounciness{0.0F}; /**< Physics::BodyPhysicalProperties::contactMaterial(). */
		float stickiness{0.5F};
		bool collidable{true}; /**< False: drawn only, no collision model (road markings a centimetre above the asphalt, a debug overlay): the bodies pass through it. */
	};

	/**
	 * @brief The plinth the world stands on (owner decision 2026-10-28: the tiles are not a ground, the world may have
	 * none): a vertical skirt from the outer edge of the border tiles down to a flat base, and a low barrier on the edge.
	 */
	struct PlinthParameters
	{
		uint32_t slot{0}; /**< The surface slot the skirt, the base and the barrier are written into. */
		float depth{4.0F}; /**< The base, this far below the lowest corner, metres. */
		float barrierHeight{1.0F}; /**< 0 = no barrier. */
		float barrierThickness{0.3F};
		bool enabled{true};
	};

	/** @brief The merged geometry of one region for one surface slot, in the region's own space. */
	struct RegionMesh
	{
		Base::VertexFactory::Shape< float > shape;
		Base::Math::Vector< 3, float > origin; /**< The region's centre, world space: the entity's position. */
		uint32_t region{0};
		uint32_t slot{0};
	};

	/**
	 * @brief Builds a world from a validated layout and a constrained relief: every tile writes its geometry, rotated
	 * and placed, merged per region of RegionSide × RegionSide cells and per surface slot (the `labyrinth` pattern:
	 * few, large static meshes), then instantiated as solid static entities colliding as their triangles.
	 * @note Two halves: buildGeometry() is pure (no scene, testable, may run off the main thread), instantiate() adds the
	 * entities to a scene through its toolkit (the scene's thread).
	 */
	class EMEN_API Builder final
	{
		public:

			/** @brief The cells per region side (8 × 16 m = 128 m). */
			static constexpr uint32_t RegionSide{8};

			/** @brief The most surface slots. */
			static constexpr uint32_t MaxSurfaceSlots{64};

			/** @brief The most zone kinds with an overlay. */
			static constexpr uint32_t MaxZoneOverlays{64};

			/** @brief How far above the tile surface a zone overlay floats, metres. */
			static constexpr float ZoneOverlayLift{0.03F};

			/** @brief Constructs a builder with no surface slot. */
			Builder () noexcept = default;

			/**
			 * @brief Declares a surface slot.
			 * @return std::optional< uint32_t > Its index, or nothing when MaxSurfaceSlots is reached or the material is null.
			 */
			[[nodiscard]]
			std::optional< uint32_t > declareSurface (SurfaceSlot slot) noexcept;

			/**
			 * @brief Sets the plinth.
			 * @return bool False when its slot is not declared or a value is not finite / negative (nothing changed).
			 */
			[[nodiscard]]
			bool setPlinth (const PlinthParameters & parameters) noexcept;

			/**
			 * @brief Makes the zones of a kind visible: a flat overlay ZoneOverlayLift above the tile surface, written
			 * into a surface slot (a debug view, an editor's highlight). Several kinds may share a slot.
			 * @param zoneKind The consumer's zone kind.
			 * @param slot A declared slot.
			 * @return bool False when the slot is not declared or MaxZoneOverlays is reached.
			 */
			[[nodiscard]]
			bool setZoneOverlay (uint32_t zoneKind, uint32_t slot) noexcept;

			/**
			 * @brief Sets the seed the tiles' per-cell variation seeds are derived from (TileContext::variationSeed).
			 * @param seed Any value. Default 0.
			 */
			void
			setVariationSeed (uint32_t seed) noexcept
			{
				m_variationSeed = seed;
			}

			/**
			 * @brief Builds the merged geometry.
			 * @param layout A VALIDATED layout.
			 * @param relief The relief sized for it (constrained).
			 * @return std::optional< std::vector< RegionMesh > > Nothing when the relief does not match the layout or a tile
			 * refused to build (logged with its cell).
			 */
			[[nodiscard]]
			std::optional< std::vector< RegionMesh > > buildGeometry (const Layout & layout, const Relief & relief) const noexcept;

			/**
			 * @brief Builds the merged geometry of ONE region (an editor rebuilds the region of an edited cell, and the regions
			 * of its four neighbours when they differ: a tile may read its neighbours, TileContext::neighbours).
			 * @param region The region index, row-major over the regions (regionCount()).
			 * @return std::optional< std::vector< RegionMesh > > One mesh per slot the region uses; nothing as buildGeometry().
			 */
			[[nodiscard]]
			std::optional< std::vector< RegionMesh > > buildRegion (const Layout & layout, const Relief & relief, uint32_t region) const noexcept;

			/** @brief The regions a layout splits into (RegionSide × RegionSide cells each, the last ones partial). */
			[[nodiscard]]
			static uint32_t regionCount (const Layout & layout) noexcept;

			/**
			 * @brief Adds one solid static entity per region mesh to a scene (named `<prefix><region>.<slot name>`).
			 * @return size_t The entities created; fewer than the meshes means some failed (each logged).
			 */
			size_t instantiate (Scenes::Toolkit & toolkit, const std::vector< RegionMesh > & meshes, std::string_view namePrefix) const noexcept;

			/** @brief The declared slots. */
			[[nodiscard]]
			const std::vector< SurfaceSlot > &
			surfaces () const noexcept
			{
				return m_surfaces;
			}

		private:

			/** @brief A block of cells: [firstColumn, endColumn) × [firstRow, endRow). */
			struct CellRange
			{
				uint32_t firstColumn{0};
				uint32_t firstRow{0};
				uint32_t endColumn{0};
				uint32_t endRow{0};
			};

			/** @brief One world-border edge of a cell, in region space: its two corners and where it faces. */
			struct BorderEdge
			{
				Base::Math::Vector< 3, float > a; /**< X, the surface height, Z. */
				Base::Math::Vector< 3, float > b;
				Base::Math::Vector< 3, float > outward;
				float insetA{0.0F}; /**< How far the barrier's inner face starts from a, along the edge. */
				float insetB{0.0F};
			};

			/**
			 * @brief Writes the tiles of a block of cells into per-slot shapes, in the space of a region centred on `origin`.
			 * @return bool False when a cell is empty, a tile refused to build or a merge failed (logged).
			 */
			[[nodiscard]]
			bool buildTiles (const Layout & layout, const Relief & relief, const CellRange & cells, const Base::Math::Vector< 3, float > & origin, std::vector< Base::VertexFactory::Shape< float > > & shapes) const noexcept;

			/**
			 * @brief Writes one border edge of the plinth: the outer skirt down to the base, the barrier's inner face and
			 * top (none when the barrier height is 0).
			 */
			void buildBorderEdge (Base::VertexFactory::ShapeBuilder< float > & builder, const BorderEdge & edge, const Base::Math::Vector< 3, float > & origin, float baseLevel) const noexcept;

			/**
			 * @brief Writes the plinth pieces of a block of cells (the skirt and the barrier on its world-border edges,
			 * the base under it) into a shape, in the space of a region centred on `origin`.
			 */
			void buildPlinth (const Layout & layout, const Relief & relief, const CellRange & cells, const Base::Math::Vector< 3, float > & origin, Base::VertexFactory::Shape< float > & shape) const noexcept;

			/**
			 * @brief Writes the overlays of a tile's zones into its own-frame shapes, on the bilinear surface of its corner
			 * heights: a rectangle in sub-cell quads, an arc in steps of at most one sub-cell along its outer radius (an
			 * invalid arc is logged and skipped).
			 */
			void buildZoneOverlays (const TileInterface & tile, const TileContext & context, SurfaceSink & sink) const noexcept;

			std::vector< SurfaceSlot > m_surfaces;
			std::vector< std::pair< uint32_t, uint32_t > > m_zoneOverlays; /**< (zone kind, slot). */
			PlinthParameters m_plinth{.slot = 0, .depth = 4.0F, .barrierHeight = 1.0F, .barrierThickness = 0.3F, .enabled = false};
			uint32_t m_variationSeed{0};
	};
}
