/*
 * src/TileWorld/Layout.hpp
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
#include <memory>
#include <vector>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::TileWorld
{
	class TileInterface;
}

namespace EmEn::TileWorld
{
	/** @brief One cell of the layout: the tile kind it holds and how it is turned. */
	struct Cell
	{
		std::shared_ptr< const TileInterface > tile;
		Rotation rotation{Rotation::R0};
	};

	/**
	 * @brief The grid: N columns × M rows of cells, each holding a tile kind and its rotation.
	 * @note A trust boundary for the sizes (they may come from a console, a demo option, a file): refused above
	 * MaxSide, never allocated blindly.
	 * @note Thread: built and read on one thread; read-only (shareable) once handed to the builder.
	 */
	class EMEN_API Layout final
	{
		public:

			/** @brief The most cells per side (512 × 16 m = 8 km): bounds the relief solve, O(cells × sweeps). */
			static constexpr uint32_t MaxSide{512};

			/**
			 * @brief Constructs an EMPTY layout (0 × 0): resize() gives it its size.
			 */
			Layout () noexcept = default;

			/**
			 * @brief Sizes the grid, every cell empty.
			 * @param columns Cells along X, 1 to MaxSide.
			 * @param rows Cells along Z, 1 to MaxSide.
			 * @param tileSize The tile size in metres, finite and > 0.
			 * @return bool False (and nothing changed) when a value is out of range.
			 */
			[[nodiscard]]
			bool resize (uint32_t columns, uint32_t rows, float tileSize = DefaultTileSize) noexcept;

			/**
			 * @brief Puts a tile kind in a cell.
			 * @return bool False when the cell is out of the grid or the tile is null.
			 */
			[[nodiscard]]
			bool place (uint32_t column, uint32_t row, std::shared_ptr< const TileInterface > tile, Rotation rotation = Rotation::R0) noexcept;

			/** @brief The cell, or null outside the grid. */
			[[nodiscard]]
			const Cell * cell (uint32_t column, uint32_t row) const noexcept;

			/**
			 * @brief The networks a cell connects on a WORLD side (its tile's rotation applied); 0 for an empty cell or
			 * outside the grid.
			 */
			[[nodiscard]]
			NetworkMask connections (uint32_t column, uint32_t row, Side worldSide) const noexcept;

			/**
			 * @brief Checks the whole grid: every cell holds a tile, and every pair of sides sharing an edge carries the
			 * same networks. The outer border connects to nothing.
			 * @return bool False on the first problem class found; EVERY mismatch is logged (cell, side, both masks).
			 */
			[[nodiscard]]
			bool validate () const noexcept;

			/** @brief The cells along X. */
			[[nodiscard]]
			uint32_t
			columns () const noexcept
			{
				return m_columns;
			}

			/** @brief The cells along Z. */
			[[nodiscard]]
			uint32_t
			rows () const noexcept
			{
				return m_rows;
			}

			/** @brief The tile size, in metres. */
			[[nodiscard]]
			float
			tileSize () const noexcept
			{
				return m_tileSize;
			}

			/** @brief The world position (Y = 0) of a cell's centre; the grid is centred on the origin. */
			[[nodiscard]]
			Base::Math::Vector< 3, float > cellCentre (uint32_t column, uint32_t row) const noexcept;

		private:

			std::vector< Cell > m_cells;
			uint32_t m_columns{0};
			uint32_t m_rows{0};
			float m_tileSize{DefaultTileSize};
	};
}
