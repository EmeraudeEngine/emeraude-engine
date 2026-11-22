/*
 * src/TileWorld/Navigation.cpp
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

#include "Navigation.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "TileWorld/Layout.hpp"
#include "Tracer.hpp"

namespace EmEn::TileWorld
{
	using Base::Math::Vector;
	using Base::Math::X;
	using Base::Math::Y;
	using Base::Math::Z;

	namespace
	{
		constexpr auto TracerTag{"TileWorld::Navigation"};

		/** @brief Under this length, in metres, a segment has no direction: it is its start point. */
		constexpr float PointLength{1e-4F};

		/** @brief Whether a world point can be placed on the grid (its height is never read). */
		[[nodiscard]]
		bool
		isOnTheMap (const Vector< 3, float > & position) noexcept
		{
			return std::isfinite(position[X]) && std::isfinite(position[Z]);
		}

		/** @brief The cell index along one axis of a fractional grid coordinate, kept inside the grid. */
		[[nodiscard]]
		int32_t
		cellAlong (float coordinate, uint32_t count) noexcept
		{
			return static_cast< int32_t >(std::clamp(std::floor(coordinate), 0.0F, static_cast< float >(count - 1U)));
		}

		/** @brief The part of a segment still kept by a clip, as parameters along it: [0, 1] is the whole segment. */
		struct KeptSpan final
		{
			float enter{0.0F};
			float leave{1.0F};
		};

		/** @brief A step from a cell to one of its four neighbours: North is −Z, east +X, south +Z, west −X (Types.hpp). */
		struct NeighbourStep final
		{
			int32_t column{0};
			int32_t row{0};
		};

		constexpr std::array< NeighbourStep, 4 > NeighbourSteps{{{.column = 0, .row = -1}, {.column = 1, .row = 0}, {.column = 0, .row = 1}, {.column = -1, .row = 0}}};

		/**
		 * @brief One edge of the Liang-Barsky clip (Y.-D. Liang, B. A. Barsky, "A New Concept and Method for Line
		 * Clipping", ACM Transactions on Graphics 3(1), 1984): keeps the part of the span where p × t ≤ q.
		 * @return bool False when nothing is left.
		 */
		[[nodiscard]]
		bool
		clipEdge (float p, float q, KeptSpan & kept) noexcept
		{
			/* Parallel to the edge: inside it or not, whatever t. */
			if ( std::abs(p) < std::numeric_limits< float >::min() )
			{
				return q >= 0.0F;
			}

			const auto t = q / p;

			if ( p < 0.0F )
			{
				if ( t > kept.leave )
				{
					return false;
				}

				kept.enter = std::max(kept.enter, t);
			}
			else
			{
				if ( t < kept.enter )
				{
					return false;
				}

				kept.leave = std::min(kept.leave, t);
			}

			return true;
		}
	}

	bool
	Navigation::build (const Layout & layout, const std::function< bool (uint32_t column, uint32_t row) > & isOpen) noexcept
	{
		m_open.clear();
		m_distances.clear();
		m_queue.clear();
		m_openCellCount = 0;
		m_goal = NoCell;
		m_targetCell = NoCell;
		m_targetGoal = NoCell;
		m_columns = 0;
		m_rows = 0;

		if ( !isOpen || layout.columns() == 0 || layout.rows() == 0 )
		{
			TraceError{TracerTag} << "build(): " << (isOpen ? "the layout is empty" : "no function says which cell is open") << " !";

			return false;
		}

		/* NOTE: A layout holds Layout::MaxSide² cells at most: a row-major index fits 32 bits, NoCell is none of them. */
		m_columns = layout.columns();
		m_rows = layout.rows();
		m_tileSize = layout.tileSize();

		const auto cells = static_cast< size_t >(m_columns) * m_rows;

		m_open.resize(cells, 0);

		for ( uint32_t row = 0; row < m_rows; ++row )
		{
			for ( uint32_t column = 0; column < m_columns; ++column )
			{
				if ( isOpen(column, row) )
				{
					m_open[(static_cast< size_t >(row) * m_columns) + column] = 1;
					++m_openCellCount;
				}
			}
		}

		m_distances.assign(cells, Unreached);
		m_queue.resize(cells);

		return true;
	}

	bool
	Navigation::isOpen (uint32_t column, uint32_t row) const noexcept
	{
		if ( column >= m_columns || row >= m_rows )
		{
			return false;
		}

		return m_open[(static_cast< size_t >(row) * m_columns) + column] != 0;
	}

	uint32_t
	Navigation::distance (uint32_t column, uint32_t row) const noexcept
	{
		if ( column >= m_columns || row >= m_rows )
		{
			return Unreached;
		}

		return m_distances[(static_cast< size_t >(row) * m_columns) + column];
	}

	Vector< 2, float >
	Navigation::gridCoordinates (const Vector< 3, float > & position) const noexcept
	{
		return {
			(position[X] / m_tileSize) + (static_cast< float >(m_columns) * 0.5F),
			(position[Z] / m_tileSize) + (static_cast< float >(m_rows) * 0.5F)
		};
	}

	std::optional< uint32_t >
	Navigation::cellAt (const Vector< 3, float > & position) const noexcept
	{
		const auto coordinates = this->gridCoordinates(position);

		if ( coordinates[X] < 0.0F || coordinates[Y] < 0.0F || coordinates[X] >= static_cast< float >(m_columns) || coordinates[Y] >= static_cast< float >(m_rows) )
		{
			return std::nullopt;
		}

		return (static_cast< uint32_t >(coordinates[Y]) * m_columns) + static_cast< uint32_t >(coordinates[X]);
	}

	uint32_t
	Navigation::nearestCell (const Vector< 3, float > & position) const noexcept
	{
		const auto coordinates = this->gridCoordinates(position);

		return (static_cast< uint32_t >(cellAlong(coordinates[Y], m_rows)) * m_columns) + static_cast< uint32_t >(cellAlong(coordinates[X], m_columns));
	}

	uint32_t
	Navigation::nearestOpenCell (uint32_t cell) const noexcept
	{
		if ( m_open[cell] != 0 )
		{
			return cell;
		}

		if ( m_openCellCount == 0 )
		{
			return NoCell;
		}

		const auto column = static_cast< int64_t >(cell % m_columns);
		const auto row = static_cast< int64_t >(cell / m_columns);
		const auto lastRing = static_cast< int64_t >(std::max(m_columns, m_rows));
		auto best = NoCell;
		auto bestSquared = std::numeric_limits< int64_t >::max();

		const auto consider = [this, column, row, &best, &bestSquared] (int64_t columnOffset, int64_t rowOffset) noexcept {
			const auto otherColumn = column + columnOffset;
			const auto otherRow = row + rowOffset;

			if ( otherColumn < 0 || otherRow < 0 || std::cmp_greater_equal(otherColumn, m_columns) || std::cmp_greater_equal(otherRow, m_rows) )
			{
				return;
			}

			const auto other = static_cast< uint32_t >((otherRow * static_cast< int64_t >(m_columns)) + otherColumn);

			if ( m_open[other] == 0 )
			{
				return;
			}

			const auto squared = (columnOffset * columnOffset) + (rowOffset * rowOffset);

			if ( squared < bestSquared || (squared == bestSquared && other < best) )
			{
				best = other;
				bestSquared = squared;
			}
		};

		/* The square rings round the cell, nearest first. The nearest cell of ring r is r cells away: once a cell is
		 * found, the rings farther than it cannot hold a nearer one. */
		for ( int64_t ring = 1; ring <= lastRing; ++ring )
		{
			if ( best != NoCell && (ring * ring) > bestSquared )
			{
				break;
			}

			for ( int64_t offset = -ring; offset <= ring; ++offset )
			{
				consider(offset, -ring);
				consider(offset, ring);
			}

			for ( int64_t offset = 1 - ring; offset < ring; ++offset )
			{
				consider(-ring, offset);
				consider(ring, offset);
			}
		}

		return best;
	}

	Vector< 3, float >
	Navigation::cellCentre (uint32_t cell) const noexcept
	{
		const auto column = cell % m_columns;
		const auto row = cell / m_columns;

		return {
			(static_cast< float >(column) + 0.5F - (static_cast< float >(m_columns) * 0.5F)) * m_tileSize,
			0.0F,
			(static_cast< float >(row) + 0.5F - (static_cast< float >(m_rows) * 0.5F)) * m_tileSize
		};
	}

	bool
	Navigation::isSegmentClear (const Vector< 2, float > & from, const Vector< 2, float > & to) const noexcept
	{
		/* In grid units: a cell is the square [column, column + 1] × [row, row + 1]. */
		const auto startColumn = (from[X] / m_tileSize) + (static_cast< float >(m_columns) * 0.5F);
		const auto startRow = (from[Y] / m_tileSize) + (static_cast< float >(m_rows) * 0.5F);
		const auto spanColumns = (to[X] - from[X]) / m_tileSize;
		const auto spanRows = (to[Y] - from[Y]) / m_tileSize;

		/* The part of the segment over the grid: nothing stands off it. */
		KeptSpan kept;

		if ( !clipEdge(-spanColumns, startColumn, kept)
			|| !clipEdge(spanColumns, static_cast< float >(m_columns) - startColumn, kept)
			|| !clipEdge(-spanRows, startRow, kept)
			|| !clipEdge(spanRows, static_cast< float >(m_rows) - startRow, kept) )
		{
			return true;
		}

		const auto firstColumn = startColumn + (kept.enter * spanColumns);
		const auto firstRow = startRow + (kept.enter * spanRows);
		const auto alongColumns = (kept.leave - kept.enter) * spanColumns;
		const auto alongRows = (kept.leave - kept.enter) * spanRows;

		auto column = cellAlong(firstColumn, m_columns);
		auto row = cellAlong(firstRow, m_rows);
		const auto lastColumn = cellAlong(firstColumn + alongColumns, m_columns);
		const auto lastRow = cellAlong(firstRow + alongRows, m_rows);

		/* The cells the segment crosses, one border at a time (Amanatides & Woo): `next*` is the parameter at which it
		 * crosses its next border on an axis, `between*` the parameter from a border to the next. The steps left on
		 * each axis bound the walk: it stays in the grid whatever the rounding. */
		const int32_t columnStep = lastColumn > column ? 1 : -1;
		const int32_t rowStep = lastRow > row ? 1 : -1;
		auto columnStepsLeft = std::abs(lastColumn - column);
		auto rowStepsLeft = std::abs(lastRow - row);
		constexpr auto Never = std::numeric_limits< float >::infinity();
		const auto betweenColumns = columnStepsLeft > 0 ? std::abs(1.0F / alongColumns) : Never;
		const auto betweenRows = rowStepsLeft > 0 ? std::abs(1.0F / alongRows) : Never;
		auto nextColumn = columnStepsLeft > 0 ? (static_cast< float >(columnStep > 0 ? column + 1 : column) - firstColumn) / alongColumns : Never;
		auto nextRow = rowStepsLeft > 0 ? (static_cast< float >(rowStep > 0 ? row + 1 : row) - firstRow) / alongRows : Never;

		for ( ;; )
		{
			if ( m_open[(static_cast< size_t >(row) * m_columns) + static_cast< size_t >(column)] == 0 )
			{
				return false;
			}

			if ( columnStepsLeft == 0 && rowStepsLeft == 0 )
			{
				return true;
			}

			if ( rowStepsLeft == 0 || (columnStepsLeft > 0 && nextColumn < nextRow) )
			{
				column += columnStep;
				--columnStepsLeft;
				nextColumn += betweenColumns;
			}
			else
			{
				row += rowStep;
				--rowStepsLeft;
				nextRow += betweenRows;
			}
		}
	}

	bool
	Navigation::isClear (const Vector< 3, float > & from, const Vector< 3, float > & to, float clearance) const noexcept
	{
		if ( !isOnTheMap(from) || !isOnTheMap(to) || !std::isfinite(clearance) || clearance < 0.0F )
		{
			return false;
		}

		if ( !this->isBuilt() )
		{
			return true;
		}

		/* Seen from above: X and Z. */
		const Vector< 2, float > start{from[X], from[Z]};
		const Vector< 2, float > end{to[X], to[Z]};

		if ( !this->isSegmentClear(start, end) )
		{
			return false;
		}

		const auto alongX = end[X] - start[X];
		const auto alongZ = end[Y] - start[Y];
		const auto length = std::sqrt((alongX * alongX) + (alongZ * alongZ));

		if ( clearance <= 0.0F || length < PointLength )
		{
			return true;
		}

		/* The two parallels a clearance away. Beyond half a tile a parallel could step over a closed cell: a wider body
		 * is tested as half a tile wide on each side. */
		const auto width = std::min(clearance, 0.5F * m_tileSize);
		const Vector< 2, float > aside{(-alongZ / length) * width, (alongX / length) * width};

		return this->isSegmentClear(start + aside, end + aside) && this->isSegmentClear(start - aside, end - aside);
	}

	void
	Navigation::solveFrom (uint32_t goal) const noexcept
	{
		if ( goal == m_goal )
		{
			return;
		}

		std::ranges::fill(m_distances, Unreached);

		/* The wavefront: every open cell enters the queue once at most, the queue holds every cell. */
		size_t head = 0;
		size_t tail = 0;

		m_distances[goal] = 0;
		m_queue[tail++] = goal;

		while ( head < tail )
		{
			const auto cell = m_queue[head++];
			const auto next = m_distances[cell] + 1U;
			const auto column = cell % m_columns;
			const auto row = cell / m_columns;

			const auto reach = [this, next, &tail] (uint32_t neighbour) noexcept {
				if ( m_open[neighbour] != 0 && m_distances[neighbour] == Unreached )
				{
					m_distances[neighbour] = next;
					m_queue[tail++] = neighbour;
				}
			};

			if ( row > 0 )
			{
				reach(cell - m_columns);
			}

			if ( column + 1U < m_columns )
			{
				reach(cell + 1U);
			}

			if ( row + 1U < m_rows )
			{
				reach(cell + m_columns);
			}

			if ( column > 0 )
			{
				reach(cell - 1U);
			}
		}

		m_goal = goal;
	}

	uint32_t
	Navigation::stepDown (uint32_t cell, const Vector< 3, float > & to) const noexcept
	{
		const auto here = m_distances[cell];

		if ( here == 0 || here == Unreached )
		{
			return NoCell;
		}

		const auto column = static_cast< int64_t >(cell % m_columns);
		const auto row = static_cast< int64_t >(cell / m_columns);
		const auto centre = this->cellCentre(cell);
		const auto towardX = to[X] - centre[X];
		const auto towardZ = to[Z] - centre[Z];
		auto best = NoCell;
		auto bestScore = 0.0F;

		for ( const auto & step : NeighbourSteps )
		{
			const auto otherColumn = column + step.column;
			const auto otherRow = row + step.row;

			if ( otherColumn < 0 || otherRow < 0 || std::cmp_greater_equal(otherColumn, m_columns) || std::cmp_greater_equal(otherRow, m_rows) )
			{
				continue;
			}

			const auto neighbour = static_cast< uint32_t >((otherRow * static_cast< int64_t >(m_columns)) + otherColumn);

			if ( m_distances[neighbour] != here - 1U )
			{
				continue;
			}

			/* The step the most in the direction of the target. */
			const auto score = (static_cast< float >(step.column) * towardX) + (static_cast< float >(step.row) * towardZ);

			if ( best == NoCell || score > bestScore )
			{
				best = neighbour;
				bestScore = score;
			}
		}

		return best;
	}

	Vector< 3, float >
	Navigation::waypoint (const Vector< 3, float > & from, const Vector< 3, float > & to, float clearance) const noexcept
	{
		if ( !this->isBuilt() || !isOnTheMap(from) || !isOnTheMap(to) )
		{
			return to;
		}

		/* A clearance that makes no sense is a line: the way is still wanted. */
		const auto width = std::isfinite(clearance) && clearance > 0.0F ? clearance : 0.0F;

		if ( this->isClear(from, to, width) )
		{
			return to;
		}

		const auto fromCell = this->cellAt(from);

		if ( !fromCell.has_value() )
		{
			return to;
		}

		const auto height = to[Y];

		/* A body in a closed cell (pushed against a wall, standing on a lot's edge) first gets back to open ground. */
		if ( m_open[fromCell.value()] == 0 )
		{
			const auto nearest = this->nearestOpenCell(fromCell.value());

			if ( nearest == NoCell )
			{
				return to;
			}

			auto centre = this->cellCentre(nearest);
			centre[Y] = height;

			return centre;
		}

		/* The goal: the open cell the nearest the target's own (the target may stand in a closed one, or off the grid). */
		if ( const auto targetCell = this->nearestCell(to); targetCell != m_targetCell )
		{
			m_targetCell = targetCell;
			m_targetGoal = this->nearestOpenCell(targetCell);
		}

		if ( m_targetGoal == NoCell )
		{
			return to;
		}

		this->solveFrom(m_targetGoal);

		auto cell = fromCell.value();

		if ( cell == m_goal || m_distances[cell] == Unreached )
		{
			return to;
		}

		/* Down the way: the next cell always (the straight way to a neighbour's centre never leaves the two cells), then
		 * as far as the body can head straight for. */
		auto heading = to;

		for ( uint32_t step = 0; step < LookAhead; ++step )
		{
			const auto next = this->stepDown(cell, to);

			if ( next == NoCell )
			{
				break;
			}

			auto centre = this->cellCentre(next);
			centre[Y] = height;

			if ( step > 0 && !this->isClear(from, centre, width) )
			{
				break;
			}

			heading = centre;
			cell = next;

			if ( cell == m_goal )
			{
				break;
			}
		}

		return heading;
	}
}
