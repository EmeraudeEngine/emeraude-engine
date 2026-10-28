/*
 * src/TileWorld/Builder.cpp
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

#include "Builder.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <ranges>
#include <span>
#include <string>
#include <utility>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Layout.hpp"
#include "Math/Matrix.hpp"
#include "Physics/BodyPhysicalProperties.hpp"
#include "Relief.hpp"
#include "Scenes/StaticEntity.hpp"
#include "Scenes/Toolkit.hpp"
#include "TileInterface.hpp"
#include "TileWorld/Types.hpp"
#include "Tracer.hpp"
#include "VertexFactory/ShapeAssembler.hpp"
#include "VertexFactory/ShapeBuilder.hpp"
#include "VertexFactory/ShapeBuilderOptions.hpp"

namespace EmEn::TileWorld
{
	using namespace Base::Math;
	using namespace Base::VertexFactory;

	namespace
	{
		constexpr auto TracerTag{"TileWorld::Builder"};

		/** @brief The plinth's texture repeats per metre (one repeat per 2 m, the `labyrinth` walls' scale). */
		constexpr float PlinthUVPerMetre{0.5F};

		/** @brief How deep the barrier's inner face starts under the tile surface: no gap on a sloped border. */
		constexpr float BarrierBurial{0.2F};

		/** @brief A sink holding one shape per declared slot. */
		class SlotShapes final : public SurfaceSink
		{
			public:

				explicit
				SlotShapes (size_t slotCount) noexcept
					: m_shapes(slotCount)
				{

				}

				[[nodiscard]]
				Shape< float > *
				surface (uint32_t slot) noexcept override
				{
					return slot < m_shapes.size() ? &m_shapes[slot] : nullptr;
				}

				[[nodiscard]]
				std::vector< Shape< float > > &
				shapes () noexcept
				{
					return m_shapes;
				}

			private:

				std::vector< Shape< float > > m_shapes;
		};

		/**
		 * @brief The placement of a tile: its quarter-turn rotation (its own north onto the world side Rotation names)
		 * then its offset. Built from exact cosines and sines (0, ±1): no trigonometric error.
		 */
		[[nodiscard]]
		Matrix< 4, float >
		placement (Rotation rotation, const Vector< 3, float > & offset) noexcept
		{
			auto cosine = 1.0F;
			auto sine = 0.0F;

			switch ( rotation )
			{
				case Rotation::R0 :
					break;

				case Rotation::R90 :
					cosine = 0.0F;
					sine = 1.0F;
					break;

				case Rotation::R180 :
					cosine = -1.0F;
					break;

				case Rotation::R270 :
					cosine = 0.0F;
					sine = -1.0F;
					break;
			}

			/* x' = c·x − s·z, z' = s·x + c·z (column major): R90 sends the own north (−Z) east (+X). */
			return Matrix< 4, float >{std::array< float, 16 >{
				cosine, 0.0F, sine, 0.0F,
				0.0F, 1.0F, 0.0F, 0.0F,
				-sine, 0.0F, cosine, 0.0F,
				offset[X], offset[Y], offset[Z], 1.0F
			}};
		}

		/** @brief A cell's world corner heights in the tile's own frame: its own corner k sits at world corner k + rotation. */
		[[nodiscard]]
		CornerHeights
		ownHeights (const CornerHeights & world, Rotation rotation) noexcept
		{
			const auto & [northWest, northEast, southEast, southWest] = world;

			switch ( rotation )
			{
				case Rotation::R0 :
					return world;

				case Rotation::R90 :
					return {northEast, southEast, southWest, northWest};

				case Rotation::R180 :
					return {southEast, southWest, northWest, northEast};

				case Rotation::R270 :
					return {southWest, northWest, northEast, southEast};
			}

			return world;
		}

		/** @brief A seed per cell, from the world seed (unsigned arithmetic: wraps by definition). */
		[[nodiscard]]
		uint32_t
		cellSeed (uint32_t worldSeed, uint32_t column, uint32_t row) noexcept
		{
			return worldSeed ^ (column * 73856093U) ^ (row * 19349663U);
		}

		/** @brief A position with its texture coordinates. */
		struct PlinthVertex
		{
			Vector< 3, float > position;
			float u{0.0F};
			float v{0.0F};
		};

		/**
		 * @brief Writes a quad facing `facing` (two triangles; the winding is chosen so the face is front towards
		 * `facing`: counter-clockwise around its normal, the base's convention).
		 */
		void
		addQuad (ShapeBuilder< float > & builder, const std::array< PlinthVertex, 4 > & quad, const Vector< 3, float > & facing) noexcept
		{
			const auto & [first, second, third, fourth] = quad;
			const auto normal = Vector< 3, float >::crossProduct(second.position - first.position, third.position - first.position);
			const bool reversed = Vector< 3, float >::dotProduct(normal, facing) < 0.0F;
			const std::array< const PlinthVertex *, 6 > order = reversed ?
				std::array< const PlinthVertex *, 6 >{&first, &third, &second, &first, &fourth, &third} :
				std::array< const PlinthVertex *, 6 >{&first, &second, &third, &first, &third, &fourth};

			for ( const auto * vertex : order )
			{
				builder.setPosition(vertex->position);
				builder.setNormal(facing);
				builder.setTextureCoordinates(vertex->u, vertex->v);
				builder.newVertex();
			}
		}
	}

	std::optional< uint32_t >
	Builder::declareSurface (SurfaceSlot slot) noexcept
	{
		if ( m_surfaces.size() >= MaxSurfaceSlots || slot.material == nullptr || !std::isfinite(slot.bounciness) || !std::isfinite(slot.stickiness) )
		{
			TraceError{TracerTag} << "declareSurface('" << slot.name << "'): refused (" << MaxSurfaceSlots << " slots at most, a material, finite contact values) !";

			return std::nullopt;
		}

		m_surfaces.push_back(std::move(slot));

		return static_cast< uint32_t >(m_surfaces.size() - 1);
	}

	bool
	Builder::setPlinth (const PlinthParameters & parameters) noexcept
	{
		const auto finite = std::isfinite(parameters.depth) && std::isfinite(parameters.barrierHeight) && std::isfinite(parameters.barrierThickness);

		if ( parameters.slot >= m_surfaces.size() || !finite || parameters.depth < 0.0F || parameters.barrierHeight < 0.0F || parameters.barrierThickness <= 0.0F )
		{
			TraceError{TracerTag} << "setPlinth(): refused (an undeclared slot, or a negative / non-finite size) !";

			return false;
		}

		m_plinth = parameters;

		return true;
	}

	bool
	Builder::setZoneOverlay (uint32_t zoneKind, uint32_t slot) noexcept
	{
		if ( slot >= m_surfaces.size() || m_zoneOverlays.size() >= MaxZoneOverlays )
		{
			TraceError{TracerTag} << "setZoneOverlay(" << zoneKind << ", " << slot << "): refused (an undeclared slot, or " << MaxZoneOverlays << " overlays already) !";

			return false;
		}

		m_zoneOverlays.emplace_back(zoneKind, slot);

		return true;
	}

	void
	Builder::buildZoneOverlays (const TileInterface & tile, const TileContext & context, SurfaceSink & sink) const noexcept
	{
		const auto & heights = context.heights;
		const auto subCell = context.tileSize / static_cast< float >(SubCellsPerSide);
		const auto half = context.tileSize * 0.5F;
		/* The bilinear surface of the corners (NW, NE, SE, SW), u along +X, v along +Z, both 0 to 1. */
		const auto surfaceAt = [&heights] (float u, float v) {
			return (heights[0] * (1.0F - u) * (1.0F - v)) + (heights[1] * u * (1.0F - v)) + (heights[2] * u * v) + (heights[3] * (1.0F - u) * v);
		};

		for ( const auto & zone : tile.zones() )
		{
			const auto overlay = std::ranges::find_if(m_zoneOverlays, [&zone] (const auto & entry) {
				return entry.first == zone.kind;
			});

			if ( overlay == m_zoneOverlays.end() )
			{
				continue;
			}

			auto * shape = sink.surface(overlay->second);

			if ( shape == nullptr )
			{
				continue;
			}

			const auto endX = std::min< uint32_t >(static_cast< uint32_t >(zone.area.x) + zone.area.width, SubCellsPerSide);
			const auto endZ = std::min< uint32_t >(static_cast< uint32_t >(zone.area.z) + zone.area.depth, SubCellsPerSide);

			ShapeBuilder< float > builder{*shape, ShapeBuilderOptions< float >{true, true, false, false, false}};
			builder.beginConstruction(ConstructionMode::Triangles);

			for ( uint32_t subZ = zone.area.z; subZ < endZ; ++subZ )
			{
				for ( uint32_t subX = zone.area.x; subX < endX; ++subX )
				{
					const auto corner = [&] (uint32_t cx, uint32_t cz) {
						const auto u = static_cast< float >(cx) / static_cast< float >(SubCellsPerSide);
						const auto v = static_cast< float >(cz) / static_cast< float >(SubCellsPerSide);

						return PlinthVertex{
							.position = {(static_cast< float >(cx) * subCell) - half, surfaceAt(u, v) + ZoneOverlayLift, (static_cast< float >(cz) * subCell) - half},
							.u = u,
							.v = v
						};
					};

					addQuad(builder, {corner(subX, subZ), corner(subX + 1, subZ), corner(subX + 1, subZ + 1), corner(subX, subZ + 1)}, Vector< 3, float >::positiveY());
				}
			}

			builder.endConstruction();
		}
	}

	uint32_t
	Builder::regionCount (const Layout & layout) noexcept
	{
		const auto regionColumns = (layout.columns() + RegionSide - 1) / RegionSide;
		const auto regionRows = (layout.rows() + RegionSide - 1) / RegionSide;

		return regionColumns * regionRows;
	}

	std::optional< std::vector< RegionMesh > >
	Builder::buildGeometry (const Layout & layout, const Relief & relief) const noexcept
	{
		std::vector< RegionMesh > meshes;
		const auto regions = regionCount(layout);

		for ( uint32_t region = 0; region < regions; ++region )
		{
			auto regionMeshes = this->buildRegion(layout, relief, region);

			if ( !regionMeshes.has_value() )
			{
				return std::nullopt;
			}

			for ( auto & mesh : regionMeshes.value() )
			{
				meshes.push_back(std::move(mesh));
			}
		}

		return meshes;
	}

	bool
	Builder::buildTiles (const Layout & layout, const Relief & relief, const CellRange & cells, const Vector< 3, float > & origin, std::vector< Shape< float > > & shapes) const noexcept
	{
		for ( uint32_t row = cells.firstRow; row < cells.endRow; ++row )
		{
			for ( uint32_t column = cells.firstColumn; column < cells.endColumn; ++column )
			{
				const auto * cell = layout.cell(column, row);

				if ( cell == nullptr || cell->tile == nullptr )
				{
					TraceError{TracerTag} << "buildTiles(): the cell (" << column << ", " << row << ") holds no tile (validate the layout first) !";

					return false;
				}

				const TileContext context{
					.heights = ownHeights(relief.cellHeights(column, row), cell->rotation),
					.centre = layout.cellCentre(column, row),
					.tileSize = layout.tileSize(),
					.column = column,
					.row = row,
					.variationSeed = cellSeed(m_variationSeed, column, row),
					.rotation = cell->rotation
				};

				SlotShapes tileShapes{m_surfaces.size()};

				if ( !cell->tile->build(context, tileShapes) )
				{
					TraceError{TracerTag} << "buildTiles(): the tile '" << cell->tile->name() << "' refused to build at (" << column << ", " << row << ") !";

					return false;
				}

				if ( !m_zoneOverlays.empty() )
				{
					this->buildZoneOverlays(*cell->tile, context, tileShapes);
				}

				/* The tile's own frame rotated and placed into the region's space, merged slot by slot. */
				const auto transform = placement(cell->rotation, layout.cellCentre(column, row) - origin);
				auto destination = shapes.begin();

				for ( const auto & tileShape : tileShapes.shapes() )
				{
					if ( !tileShape.empty() && !ShapeAssembler< float >{*destination}.merge(tileShape, transform) )
					{
						TraceError{TracerTag} << "buildTiles(): unable to merge the tile '" << cell->tile->name() << "' at (" << column << ", " << row << ") !";

						return false;
					}

					++destination;
				}
			}
		}

		return true;
	}

	std::optional< std::vector< RegionMesh > >
	Builder::buildRegion (const Layout & layout, const Relief & relief, uint32_t region) const noexcept
	{
		if ( relief.cornerColumns() != layout.columns() + 1 || relief.cornerRows() != layout.rows() + 1 || region >= regionCount(layout) )
		{
			TraceError{TracerTag} << "buildRegion(" << region << "): the relief is not sized for the layout, or no such region !";

			return std::nullopt;
		}

		const auto regionColumns = (layout.columns() + RegionSide - 1) / RegionSide;
		const auto firstColumn = (region % regionColumns) * RegionSide;
		const auto firstRow = (region / regionColumns) * RegionSide;
		const CellRange cells{
			.firstColumn = firstColumn,
			.firstRow = firstRow,
			.endColumn = std::min(firstColumn + RegionSide, layout.columns()),
			.endRow = std::min(firstRow + RegionSide, layout.rows())
		};
		const auto origin = (layout.cellCentre(cells.firstColumn, cells.firstRow) + layout.cellCentre(cells.endColumn - 1, cells.endRow - 1)) * 0.5F;

		SlotShapes regionShapes{m_surfaces.size()};

		if ( !this->buildTiles(layout, relief, cells, origin, regionShapes.shapes()) )
		{
			TraceError{TracerTag} << "buildRegion(" << region << "): its tiles failed !";

			return std::nullopt;
		}

		if ( m_plinth.enabled )
		{
			if ( auto * plinth = regionShapes.surface(m_plinth.slot); plinth != nullptr )
			{
				this->buildPlinth(layout, relief, cells, origin, *plinth);
			}
		}

		std::vector< RegionMesh > meshes;
		uint32_t slot = 0;

		for ( auto & shape : regionShapes.shapes() )
		{
			if ( !shape.empty() )
			{
				meshes.push_back({.shape = std::move(shape), .origin = origin, .region = region, .slot = slot});
			}

			++slot;
		}

		return meshes;
	}

	void
	Builder::buildBorderEdge (ShapeBuilder< float > & builder, const BorderEdge & edge, const Vector< 3, float > & origin, float baseLevel) const noexcept
	{
		const auto & a = edge.a;
		const auto & b = edge.b;
		const auto barrier = m_plinth.barrierHeight;
		const auto along = (b - a).normalized();
		const auto length = (b - a).length();
		/* The along-edge texture coordinate is the world coordinate the edge runs on: continuous across cells. */
		const bool alongX = std::abs(along[X]) > 0.5F;
		const auto worldU = [&origin, alongX] (const Vector< 3, float > & point) {
			return (alongX ? point[X] + origin[X] : point[Z] + origin[Z]) * PlinthUVPerMetre;
		};

		/* The outer skirt, from the top (the surface + the barrier) down to the base. */
		const PlinthVertex topA{.position = {a[X], a[Y] + barrier, a[Z]}, .u = worldU(a), .v = 0.0F};
		const PlinthVertex topB{.position = {b[X], b[Y] + barrier, b[Z]}, .u = worldU(b), .v = 0.0F};
		const PlinthVertex bottomB{.position = {b[X], baseLevel, b[Z]}, .u = worldU(b), .v = (b[Y] + barrier - baseLevel) * PlinthUVPerMetre};
		const PlinthVertex bottomA{.position = {a[X], baseLevel, a[Z]}, .u = worldU(a), .v = (a[Y] + barrier - baseLevel) * PlinthUVPerMetre};

		addQuad(builder, {topA, topB, bottomB, bottomA}, edge.outward);

		if ( barrier <= 0.0F )
		{
			return;
		}

		/* The barrier's inner face, inset by its thickness; its foot buried under the surface (linear along the edge, the
		 * burial covers the bilinear rest). */
		const auto inward = -edge.outward * m_plinth.barrierThickness;
		const auto innerA = a + inward + (along * edge.insetA);
		const auto innerB = b + inward - (along * edge.insetB);
		const auto ratioA = length > 0.0F ? edge.insetA / length : 0.0F;
		const auto ratioB = length > 0.0F ? (length - edge.insetB) / length : 1.0F;
		const auto innerHeightA = a[Y] + ((b[Y] - a[Y]) * ratioA);
		const auto innerHeightB = a[Y] + ((b[Y] - a[Y]) * ratioB);
		const auto innerV = (barrier + BarrierBurial) * PlinthUVPerMetre;

		const PlinthVertex innerTopA{.position = {innerA[X], innerHeightA + barrier, innerA[Z]}, .u = worldU(innerA), .v = 0.0F};
		const PlinthVertex innerTopB{.position = {innerB[X], innerHeightB + barrier, innerB[Z]}, .u = worldU(innerB), .v = 0.0F};
		const PlinthVertex innerBottomB{.position = {innerB[X], innerHeightB - BarrierBurial, innerB[Z]}, .u = worldU(innerB), .v = innerV};
		const PlinthVertex innerBottomA{.position = {innerA[X], innerHeightA - BarrierBurial, innerA[Z]}, .u = worldU(innerA), .v = innerV};

		addQuad(builder, {innerTopA, innerTopB, innerBottomB, innerBottomA}, -edge.outward);

		/* The barrier's top, from the outer edge to the inner one. */
		auto topInnerA = innerTopA;
		auto topInnerB = innerTopB;
		topInnerA.v = m_plinth.barrierThickness * PlinthUVPerMetre;
		topInnerB.v = m_plinth.barrierThickness * PlinthUVPerMetre;

		addQuad(builder, {topA, topB, topInnerB, topInnerA}, Vector< 3, float >::positiveY());
	}

	void
	Builder::buildPlinth (const Layout & layout, const Relief & relief, const CellRange & cells, const Vector< 3, float > & origin, Shape< float > & shape) const noexcept
	{
		const auto half = layout.tileSize() * 0.5F;
		const auto baseLevel = relief.range().first - m_plinth.depth;
		const auto thickness = m_plinth.barrierThickness;
		const auto lastColumn = layout.columns() - 1;
		const auto lastRow = layout.rows() - 1;

		ShapeBuilder< float > builder{shape, ShapeBuilderOptions< float >{true, true, false, false, false}};
		builder.beginConstruction(ConstructionMode::Triangles);

		/* A corner of the lattice in region space, its height the relief's. */
		const auto corner = [&layout, &relief, &origin, half] (uint32_t cornerColumn, uint32_t cornerRow) {
			const auto centre = layout.cellCentre(std::min(cornerColumn, layout.columns() - 1), std::min(cornerRow, layout.rows() - 1));
			const auto x = centre[X] + (cornerColumn < layout.columns() ? -half : half);
			const auto z = centre[Z] + (cornerRow < layout.rows() ? -half : half);

			return Vector< 3, float >{x - origin[X], relief.height(cornerColumn, cornerRow), z - origin[Z]};
		};

		/* The world-border edges of the block's cells. The east and west barriers stop short of the corner squares, which
		 * the north and south barriers fill. */
		for ( uint32_t row = cells.firstRow; row < cells.endRow; ++row )
		{
			for ( uint32_t column = cells.firstColumn; column < cells.endColumn; ++column )
			{
				const auto northEnd = row == 0 ? thickness : 0.0F;
				const auto southEnd = row == lastRow ? thickness : 0.0F;

				if ( row == 0 )
				{
					this->buildBorderEdge(builder, {.a = corner(column, row), .b = corner(column + 1, row), .outward = Vector< 3, float >::negativeZ()}, origin, baseLevel);
				}

				if ( row == lastRow )
				{
					this->buildBorderEdge(builder, {.a = corner(column, row + 1), .b = corner(column + 1, row + 1), .outward = Vector< 3, float >::positiveZ()}, origin, baseLevel);
				}

				if ( column == 0 )
				{
					this->buildBorderEdge(builder, {.a = corner(column, row), .b = corner(column, row + 1), .outward = Vector< 3, float >::negativeX(), .insetA = northEnd, .insetB = southEnd}, origin, baseLevel);
				}

				if ( column == lastColumn )
				{
					this->buildBorderEdge(builder, {.a = corner(column + 1, row), .b = corner(column + 1, row + 1), .outward = Vector< 3, float >::positiveX(), .insetA = northEnd, .insetB = southEnd}, origin, baseLevel);
				}
			}
		}

		/* The base under the block, facing down. */
		const auto northWest = corner(cells.firstColumn, cells.firstRow);
		const auto southEast = corner(cells.endColumn, cells.endRow);
		/* A base vertex from a corner's X and Z (its height dropped to the base). */
		const auto baseVertex = [&origin, baseLevel] (const Vector< 3, float > & at) {
			return PlinthVertex{.position = {at[X], baseLevel, at[Z]}, .u = (at[X] + origin[X]) * PlinthUVPerMetre, .v = (at[Z] + origin[Z]) * PlinthUVPerMetre};
		};

		addQuad(builder, {
			baseVertex(northWest),
			baseVertex({southEast[X], 0.0F, northWest[Z]}),
			baseVertex(southEast),
			baseVertex({northWest[X], 0.0F, southEast[Z]})
		}, Vector< 3, float >::negativeY());

		builder.endConstruction();
	}

	size_t
	Builder::instantiate (Scenes::Toolkit & toolkit, const std::vector< RegionMesh > & meshes, std::string_view namePrefix) const noexcept
	{
		size_t created = 0;

		for ( const auto & mesh : meshes )
		{
			if ( mesh.slot >= m_surfaces.size() )
			{
				TraceError{TracerTag} << "instantiate(): the mesh of region " << mesh.region << " names an undeclared slot " << mesh.slot << " !";

				continue;
			}

			const auto & surface = m_surfaces[mesh.slot];
			const auto contact = Physics::BodyPhysicalProperties::contactMaterial(surface.bounciness, surface.stickiness);
			auto name = std::string{namePrefix} + std::to_string(mesh.region) + "." + surface.name;

			const auto built = toolkit
				.setCursor(mesh.origin[X], mesh.origin[Y], mesh.origin[Z])
				.generateTriangleMeshInstance< Scenes::StaticEntity >(name, mesh.shape, surface.material, contact);

			if ( !built.isValid() )
			{
				TraceError{TracerTag} << "instantiate(): unable to create '" << name << "' !";

				continue;
			}

			++created;
		}

		return created;
	}
}
