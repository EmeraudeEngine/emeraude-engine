/*
 * src/TileWorld/Scatter.cpp
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

#include "Scatter.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <span>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Layout.hpp"
#include "PortableRandom.hpp"
#include "Relief.hpp"
#include "TileInterface.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::TileWorld
{
	using namespace Base::Math;

	namespace
	{
		constexpr auto TracerTag{"TileWorld::Scatter"};

		/** @brief A side as a direction in the world: north −Z, east +X, south +Z, west −X. */
		[[nodiscard]]
		Vector< 3, float >
		direction (Side side) noexcept
		{
			switch ( side )
			{
				case Side::North :
					return Vector< 3, float >::negativeZ();

				case Side::East :
					return Vector< 3, float >::positiveX();

				case Side::South :
					return Vector< 3, float >::positiveZ();

				case Side::West :
					return Vector< 3, float >::negativeX();
			}

			return Vector< 3, float >::negativeZ();
		}

		/** @brief How many steps fit a length: round(length / spacing), at least `minimum`; 0 for nothing usable. */
		[[nodiscard]]
		uint32_t
		stepsIn (float length, float spacing, uint32_t minimum) noexcept
		{
			if ( length <= 0.0F )
			{
				return 0;
			}

			return std::max(minimum, static_cast< uint32_t >(std::lround(length / spacing)));
		}

		/** @brief Whether a scatter fills a zone of this shape. */
		[[nodiscard]]
		bool
		fills (ScatterShapes shapes, ZoneShape shape) noexcept
		{
			switch ( shapes )
			{
				case ScatterShapes::All :
					return true;

				case ScatterShapes::Rectangles :
					return shape == ZoneShape::Rectangle;

				case ScatterShapes::Arcs :
					return shape == ZoneShape::Arc;
			}

			return false;
		}

		/** @brief What a scatter needs about one cell. */
		struct CellFrame
		{
			TileContext context; /**< Its centre and its rotation (worldPlanar()). */
			float subCell{0.0F};
			float half{0.0F};
		};

		/** @brief A point of a tile's own frame, in sub-cells. */
		struct OwnPoint
		{
			float x{0.0F};
			float z{0.0F};
		};

		/**
		 * @brief Collects the points of one scatter, refusing past MaxPlacements.
		 * @note Non-owning: it lives inside scatter(), shorter than the layout, the relief and the output it points to.
		 */
		class Collector final
		{
			public:

				Collector (const Layout & layout, const Relief & relief, std::vector< Placement > & placements) noexcept
					: m_layout{&layout},
					m_relief{&relief},
					m_placements{&placements}
				{

				}

				/**
				 * @brief Adds a point given in a tile's own frame.
				 * @return bool False once MaxPlacements is reached (logged).
				 */
				[[nodiscard]]
				bool
				add (const CellFrame & frame, const OwnPoint & point, const Vector< 3, float > & facing, uint32_t seed) const noexcept
				{
					if ( m_placements->size() >= MaxPlacements )
					{
						TraceError{TracerTag} << "scatter(): more than " << MaxPlacements << " points, refused !";

						return false;
					}

					const auto [worldX, worldZ] = worldPlanar(frame.context, (point.x * frame.subCell) - frame.half, (point.z * frame.subCell) - frame.half);
					const Vector< 3, float > position{worldX, 0.0F, worldZ};

					m_placements->push_back({
						.position = {worldX, m_relief->surfaceHeight(*m_layout, position).value_or(0.0F), worldZ},
						.facing = facing,
						.seed = seed,
						.column = frame.context.column,
						.row = frame.context.row
					});

					return true;
				}

			private:

				const Layout * m_layout;
				const Relief * m_relief;
				std::vector< Placement > * m_placements;
		};

		/** @brief A jittered draw in [−1, 1] from a cell's generator. */
		using Jitter = Base::PortableRandom::UniformReal< float >;

		/**
		 * @brief Fills a rectangle zone with a jittered grid.
		 * @return bool False once MaxPlacements is reached.
		 */
		[[nodiscard]]
		bool
		scatterRectangle (const Collector & collector, const CellFrame & frame, const Zone & zone, const ScatterParameters & parameters, std::mt19937 & generator) noexcept
		{
			const Jitter unit{-1.0F, 1.0F};
			const auto margin = parameters.margin / frame.subCell;
			const auto width = static_cast< float >(zone.area.width) - (2.0F * margin);
			const auto depth = static_cast< float >(zone.area.depth) - (2.0F * margin);
			/* minimumPerZone counts along the zone's LENGTH (its longer side); across it (a strip's width) one row at
			 * least: a 1 m prop strip holds its points in a single file. */
			const auto alongX = zone.area.width >= zone.area.depth;
			const auto columns = stepsIn(width * frame.subCell, parameters.spacing, alongX ? parameters.minimumPerZone : 1U);
			const auto rows = stepsIn(depth * frame.subCell, parameters.spacing, alongX ? 1U : parameters.minimumPerZone);

			if ( columns == 0 || rows == 0 )
			{
				return true;
			}

			const auto stepWidth = width / static_cast< float >(columns);
			const auto stepDepth = depth / static_cast< float >(rows);
			const auto facing = direction(rotated(zone.facing, frame.context.rotation));

			for ( uint32_t stepZ = 0; stepZ < rows; ++stepZ )
			{
				for ( uint32_t stepX = 0; stepX < columns; ++stepX )
				{
					const OwnPoint point{
						.x = static_cast< float >(zone.area.x) + margin + ((static_cast< float >(stepX) + 0.5F + (parameters.jitter * unit(generator))) * stepWidth),
						.z = static_cast< float >(zone.area.z) + margin + ((static_cast< float >(stepZ) + 0.5F + (parameters.jitter * unit(generator))) * stepDepth)
					};

					if ( !collector.add(frame, point, facing, static_cast< uint32_t >(generator())) )
					{
						return false;
					}
				}
			}

			return true;
		}

		/**
		 * @brief Places points along an arc zone's middle radius, every spacing of arc length, jittered along it; they face
		 * the arc's centre or away from it. An invalid arc places nothing.
		 * @return bool False once MaxPlacements is reached.
		 */
		[[nodiscard]]
		bool
		scatterArc (const Collector & collector, const CellFrame & frame, const Zone & zone, const ScatterParameters & parameters, std::mt19937 & generator) noexcept
		{
			if ( !isValid(zone.arc) )
			{
				return true;
			}

			const Jitter unit{-1.0F, 1.0F};
			const auto radius = 0.5F * static_cast< float >(zone.arc.innerRadius + zone.arc.outerRadius);
			const auto steps = stepsIn((radius * zone.arc.sweep * frame.subCell) - (2.0F * parameters.margin), parameters.spacing, parameters.minimumPerZone);

			if ( steps == 0 )
			{
				return true;
			}

			const auto marginAngle = parameters.margin / (radius * frame.subCell);
			const auto stepAngle = (zone.arc.sweep - (2.0F * marginAngle)) / static_cast< float >(steps);
			const auto sign = zone.arc.facesCentre ? -1.0F : 1.0F;
			/* worldPlanar() is a quarter turn plus a translation: the difference of two images turns a direction. */
			const auto [originX, originZ] = worldPlanar(frame.context, 0.0F, 0.0F);

			for ( uint32_t step = 0; step < steps; ++step )
			{
				const auto angle = zone.arc.startAngle + marginAngle + ((static_cast< float >(step) + 0.5F + (parameters.jitter * unit(generator))) * stepAngle);
				const auto [outX, outZ] = worldPlanar(frame.context, std::cos(angle), std::sin(angle));
				const OwnPoint point{
					.x = static_cast< float >(zone.arc.centreX) + (radius * std::cos(angle)),
					.z = static_cast< float >(zone.arc.centreZ) + (radius * std::sin(angle))
				};

				if ( !collector.add(frame, point, Vector< 3, float >{(outX - originX) * sign, 0.0F, (outZ - originZ) * sign}, static_cast< uint32_t >(generator())) )
				{
					return false;
				}
			}

			return true;
		}
	}

	std::optional< std::vector< Placement > >
	scatter (const Layout & layout, const Relief & relief, const ScatterParameters & parameters) noexcept
	{
		const auto valid = std::isfinite(parameters.spacing) && std::isfinite(parameters.jitter) && std::isfinite(parameters.margin)
			&& parameters.spacing >= MinSpacing && parameters.jitter >= 0.0F && parameters.jitter <= 0.5F && parameters.margin >= 0.0F
			&& parameters.minimumPerZone <= MaxMinimumPerZone;

		if ( !valid || relief.cornerColumns() != layout.columns() + 1 || relief.cornerRows() != layout.rows() + 1 )
		{
			TraceError{TracerTag} << "scatter(): refused (spacing ≥ " << MinSpacing << " m, jitter 0 to 0.5, a margin ≥ 0, a minimum per zone ≤ " << MaxMinimumPerZone << ", the relief sized for the layout) !";

			return std::nullopt;
		}

		std::vector< Placement > placements;
		const Collector collector{layout, relief, placements};

		for ( uint32_t row = 0; row < layout.rows(); ++row )
		{
			for ( uint32_t column = 0; column < layout.columns(); ++column )
			{
				const auto * cell = layout.cell(column, row);

				if ( cell == nullptr || cell->tile == nullptr )
				{
					continue;
				}

				/* One generator per cell: its points depend on its seed only. */
				std::mt19937 generator{cellSeed(parameters.seed, column, row)};
				const CellFrame frame{
					.context = {.centre = layout.cellCentre(column, row), .column = column, .row = row, .rotation = cell->rotation},
					.subCell = layout.tileSize() / static_cast< float >(SubCellsPerSide),
					.half = layout.tileSize() * 0.5F
				};

				for ( const auto & zone : cell->tile->zones() )
				{
					if ( zone.kind != parameters.zoneKind || !fills(parameters.shapes, zone.shape) )
					{
						continue;
					}

					const auto added = zone.shape == ZoneShape::Rectangle ?
						scatterRectangle(collector, frame, zone, parameters, generator) :
						scatterArc(collector, frame, zone, parameters, generator);

					if ( !added )
					{
						return std::nullopt;
					}
				}
			}
		}

		return placements;
	}
}
