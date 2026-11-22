/*
 * src/TileWorld/Navigation.hpp
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
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

/* Local inclusions for inheritances. */
#include "Scenes/NavigationInterface.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "TileWorld/Types.hpp"

/* Forward declarations. */
namespace EmEn::TileWorld
{
	class Layout;
}

namespace EmEn::TileWorld
{
	/**
	 * @brief Where a body may go over a layout, and the way to a point: each cell is OPEN or CLOSED, and a body walks
	 * from an open cell to the next through the side they share.
	 * @note The CONSUMER says which cell is open (a built lot is closed; a road, a park are open): the engine never
	 * knows "a building". A WHOLE cell is open or closed: what stands inside an open cell (a post, a bench) and the
	 * relief are not seen. Outside the grid nothing stands (a segment is clear there) and no way goes.
	 * @note The way round: a breadth-first wavefront from the target's cell (C. Y. Lee, "An Algorithm for Path
	 * Connections and Its Applications", IRE Transactions on Electronic Computers EC-10(3), 1961), kept until another
	 * target cell is asked — every body after ONE target shares one walk, the idea of the flow fields of crowd path
	 * finding (E. Emerson, "Crowd Pathfinding and Steering Using Flow Field Tiles", Game AI Pro, CRC Press, 2013).
	 * The straight way: the cells a segment crosses (J. Amanatides, A. Woo, "A Fast Voxel Traversal Algorithm for
	 * Ray Tracing", Eurographics 1987).
	 * @note Thread: built once, then asked from ONE thread (the logic thread). waypoint() keeps the distances of the
	 * last target cell: a cache, hence the mutable members.
	 * @extends EmEn::Scenes::NavigationInterface This is a navigation.
	 */
	class EMEN_API Navigation final : public Scenes::NavigationInterface
	{
		public:

			/** @brief The distance of a cell no way reaches. */
			static constexpr uint32_t Unreached{std::numeric_limits< uint32_t >::max()};

			/**
			 * @brief How many cells down the way waypoint() looks for one it can head straight for: the corners are
			 * cut wherever the grid shows room, a body does not walk from cell centre to cell centre.
			 */
			static constexpr uint32_t LookAhead{8};

			/**
			 * @brief Constructs an EMPTY navigation: nothing stands anywhere, no way is known (the target is the
			 * waypoint). build() gives it its cells.
			 */
			Navigation () noexcept = default;

			/**
			 * @brief Sizes the grid as a layout's and asks the consumer for each cell.
			 * @param layout The layout: its columns, its rows, its tile size. The navigation keeps none of its tiles: a
			 * later edit of the layout is not seen, build again.
			 * @param isOpen Answers whether a body may stand in the cell (column, row). Called once per cell, in row-major
			 * order, during this call only.
			 * @return bool False (and the navigation left EMPTY) for an empty layout or an empty function.
			 */
			[[nodiscard]]
			bool build (const Layout & layout, const std::function< bool (uint32_t column, uint32_t row) > & isOpen) noexcept;

			/**
			 * @brief Returns whether the navigation holds a grid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBuilt () const noexcept
			{
				return !m_open.empty();
			}

			/**
			 * @brief Returns whether a cell is open.
			 * @param column The cell's column.
			 * @param row The cell's row.
			 * @return bool False outside the grid.
			 */
			[[nodiscard]]
			bool isOpen (uint32_t column, uint32_t row) const noexcept;

			/**
			 * @brief Returns the number of open cells.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			openCellCount () const noexcept
			{
				return m_openCellCount;
			}

			/**
			 * @brief Returns how many cells of walk separate a cell from the cell of the last target waypoint() was
			 * asked for.
			 * @param column The cell's column.
			 * @param row The cell's row.
			 * @return uint32_t Unreached for a closed cell, a cell no way reaches, outside the grid, or before any
			 * target was asked.
			 */
			[[nodiscard]]
			uint32_t distance (uint32_t column, uint32_t row) const noexcept;

			/**
			 * @copydoc EmEn::Scenes::NavigationInterface::isClear()
			 * @note The segment and its two parallels a clearance away cross open cells only (or leave the grid). The
			 * heights are ignored. An empty navigation answers true.
			 */
			[[nodiscard]]
			bool isClear (const Base::Math::Vector< 3, float > & from, const Base::Math::Vector< 3, float > & to, float clearance) const noexcept override;

			/**
			 * @copydoc EmEn::Scenes::NavigationInterface::waypoint()
			 * @note The target when the straight way is clear. Else, from an open cell: the centre of the farthest of the
			 * next LookAhead cells down the way that the body can head straight for (the next one at least — two
			 * neighbouring open cells make a rectangle, the straight way to its centre never leaves them). From a
			 * CLOSED cell (a body pushed against a wall): the centre of the nearest open cell. A target in a closed cell
			 * is reached through the open cell nearest its own. The answer's height is the target's.
			 * @note The target when no way exists, when the body is outside the grid, or for an empty navigation.
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > waypoint (const Base::Math::Vector< 3, float > & from, const Base::Math::Vector< 3, float > & to, float clearance) const noexcept override;

		private:

			/** @brief No cell. */
			static constexpr uint32_t NoCell{std::numeric_limits< uint32_t >::max()};

			/**
			 * @brief Returns the fractional cell coordinates of a world point (column along +X, row along +Z; the grid
			 * is centred on the origin).
			 * @param position The world point (its Y ignored).
			 * @return Base::Math::Vector< 2, float > {column, row}, outside [0, columns] × [0, rows] off the grid.
			 */
			[[nodiscard]]
			Base::Math::Vector< 2, float > gridCoordinates (const Base::Math::Vector< 3, float > & position) const noexcept;

			/**
			 * @brief Returns the row-major index of the cell a world point falls in.
			 * @param position The world point (its Y ignored), finite.
			 * @return std::optional< uint32_t > Nothing outside the grid.
			 */
			[[nodiscard]]
			std::optional< uint32_t > cellAt (const Base::Math::Vector< 3, float > & position) const noexcept;

			/**
			 * @brief Returns the row-major index of the cell of the grid the nearest a world point: its own, or the
			 * border cell beside it for a point off the grid.
			 * @param position The world point (its Y ignored), finite.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t nearestCell (const Base::Math::Vector< 3, float > & position) const noexcept;

			/**
			 * @brief Returns the open cell the nearest a cell, centre to centre (the first in row-major order on a tie):
			 * the cell itself when it is open.
			 * @param cell The cell's row-major index, in the grid.
			 * @return uint32_t NoCell when no cell is open.
			 */
			[[nodiscard]]
			uint32_t nearestOpenCell (uint32_t cell) const noexcept;

			/**
			 * @brief Returns the world position of a cell's centre, at Y = 0.
			 * @param cell The cell's row-major index, in the grid.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > cellCentre (uint32_t cell) const noexcept;

			/**
			 * @brief Returns whether a segment crosses open cells only; what lies off the grid is clear.
			 * @param from The start seen from above: {world X, world Z}.
			 * @param to The end, likewise.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSegmentClear (const Base::Math::Vector< 2, float > & from, const Base::Math::Vector< 2, float > & to) const noexcept;

			/**
			 * @brief Walks the distances again from a goal cell, unless they are that cell's already.
			 * @param goal The goal's row-major index, an OPEN cell of the grid.
			 */
			void solveFrom (uint32_t goal) const noexcept;

			/**
			 * @brief Returns the neighbour of a cell one step nearer the goal; among several, the one the most in the
			 * direction of the target.
			 * @param cell The cell's row-major index: open, reached, not the goal.
			 * @param to The target, in world space.
			 * @return uint32_t NoCell when none is nearer (never for a reached cell that is not the goal).
			 */
			[[nodiscard]]
			uint32_t stepDown (uint32_t cell, const Base::Math::Vector< 3, float > & to) const noexcept;

			/** @brief One byte per cell, row-major: 1 = open. */
			std::vector< uint8_t > m_open;
			/** @brief The cells of walk to m_goal, per cell (a cache of waypoint(): the last target cell's). */
			mutable std::vector< uint32_t > m_distances;
			/** @brief The wavefront's queue, sized once for every cell (solveFrom() allocates nothing). */
			mutable std::vector< uint32_t > m_queue;
			size_t m_openCellCount{0};
			/** @brief The cell m_distances were walked from, NoCell before the first target. */
			mutable uint32_t m_goal{NoCell};
			/** @brief The last target cell asked and the open cell found the nearest it (a cache of waypoint()). */
			mutable uint32_t m_targetCell{NoCell};
			mutable uint32_t m_targetGoal{NoCell};
			uint32_t m_columns{0};
			uint32_t m_rows{0};
			float m_tileSize{DefaultTileSize};
	};
}
