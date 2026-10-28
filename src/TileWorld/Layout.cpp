/*
 * src/TileWorld/Layout.cpp
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

#include "Layout.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstddef>
#include <utility>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "TileWorld/TileInterface.hpp"
#include "TileWorld/Types.hpp"
#include "Tracer.hpp"

namespace EmEn::TileWorld
{
	namespace
	{
		constexpr auto TracerTag{"TileWorld::Layout"};

		/**
		 * @brief Checks one cell (by its row-major index): it holds a tile, its east and south sides agree with their
		 * neighbours, its west / north side on the border connects to nothing. Every problem is logged.
		 */
		[[nodiscard]]
		bool
		validateCell (const Layout & layout, size_t index) noexcept
		{
			const auto column = static_cast< uint32_t >(index % layout.columns());
			const auto row = static_cast< uint32_t >(index / layout.columns());

			if ( layout.cell(column, row)->tile == nullptr )
			{
				TraceError{TracerTag} << "validate(): the cell (" << column << ", " << row << ") holds no tile !";

				return false;
			}

			bool valid = true;

			/* Each shared edge once (east and south); the border sides must connect to nothing. */
			const auto east = layout.connections(column, row, Side::East);
			const auto eastNeighbour = column + 1 < layout.columns() ? layout.connections(column + 1, row, Side::West) : NetworkMask{0};
			const auto south = layout.connections(column, row, Side::South);
			const auto southNeighbour = row + 1 < layout.rows() ? layout.connections(column, row + 1, Side::North) : NetworkMask{0};

			if ( east != eastNeighbour || south != southNeighbour )
			{
				TraceError{TracerTag} << "validate(): the cell (" << column << ", " << row << ") carries " << static_cast< uint32_t >(east) << " east / " << static_cast< uint32_t >(south) << " south, its neighbours " << static_cast< uint32_t >(eastNeighbour) << " / " << static_cast< uint32_t >(southNeighbour) << " !";

				valid = false;
			}

			if ( (column == 0 && layout.connections(column, row, Side::West) != 0) || (row == 0 && layout.connections(column, row, Side::North) != 0) )
			{
				TraceError{TracerTag} << "validate(): the cell (" << column << ", " << row << ") connects through the west or north border !";

				valid = false;
			}

			return valid;
		}
	}

	bool
	Layout::resize (uint32_t columns, uint32_t rows, float tileSize) noexcept
	{
		if ( columns == 0 || rows == 0 || columns > MaxSide || rows > MaxSide || !std::isfinite(tileSize) || tileSize <= 0.0F )
		{
			TraceError{TracerTag} << "resize(" << columns << ", " << rows << ", " << tileSize << "): refused (1 to " << MaxSide << " cells per side, a finite tile size > 0) !";

			return false;
		}

		m_cells.assign(static_cast< size_t >(columns) * rows, Cell{});
		m_columns = columns;
		m_rows = rows;
		m_tileSize = tileSize;

		return true;
	}

	bool
	Layout::place (uint32_t column, uint32_t row, std::shared_ptr< const TileInterface > tile, Rotation rotation) noexcept
	{
		if ( column >= m_columns || row >= m_rows || tile == nullptr )
		{
			TraceError{TracerTag} << "place(" << column << ", " << row << "): refused (out of the " << m_columns << " × " << m_rows << " grid, or no tile) !";

			return false;
		}

		auto & target = m_cells[(static_cast< size_t >(row) * m_columns) + column];
		target.tile = std::move(tile);
		target.rotation = rotation;

		return true;
	}

	const Cell *
	Layout::cell (uint32_t column, uint32_t row) const noexcept
	{
		if ( column >= m_columns || row >= m_rows )
		{
			return nullptr;
		}

		return &m_cells[(static_cast< size_t >(row) * m_columns) + column];
	}

	NetworkMask
	Layout::connections (uint32_t column, uint32_t row, Side worldSide) const noexcept
	{
		const auto * target = this->cell(column, row);

		if ( target == nullptr || target->tile == nullptr )
		{
			return 0;
		}

		/* The tile's own side that the rotation turned onto this world side: the inverse rotation. */
		const auto ownSide = static_cast< Side >((static_cast< uint8_t >(worldSide) + 4U - static_cast< uint8_t >(target->rotation)) % 4U);

		return target->tile->connections(ownSide);
	}

	bool
	Layout::validate () const noexcept
	{
		if ( m_cells.empty() )
		{
			TraceError{TracerTag} << "validate(): the layout is empty !";

			return false;
		}

		bool valid = true;

		for ( size_t index = 0; index < m_cells.size(); ++index )
		{
			valid = validateCell(*this, index) && valid;
		}

		return valid;
	}

	Base::Math::Vector< 3, float >
	Layout::cellCentre (uint32_t column, uint32_t row) const noexcept
	{
		const auto x = (static_cast< float >(column) + 0.5F - (static_cast< float >(m_columns) * 0.5F)) * m_tileSize;
		const auto z = (static_cast< float >(row) + 0.5F - (static_cast< float >(m_rows) * 0.5F)) * m_tileSize;

		return {x, 0.0F, z};
	}
}
