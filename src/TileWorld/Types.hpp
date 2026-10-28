/*
 * src/TileWorld/Types.hpp
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

/* STL inclusions. */
#include <array>
#include <cstdint>

/**
 * @brief TileWorld — an OPTIONAL way to build a scene: square tiles that assemble on a grid, over a slope-bounded
 * relief, merged into static solid meshes. Nothing in the engine depends on it.
 * @note Frames: a column runs along +X, a row along +Z; the grid is centred on the world origin; Y is up. A tile's
 * own frame has its origin at its centre, its sides at ±tileSize/2. North is −Z, east +X, south +Z, west −X.
 */
namespace EmEn::TileWorld
{
	/** @brief The default tile size, in metres (owner decision 2026-10-28: 16 m, 256 m²). */
	constexpr float DefaultTileSize{16.0F};

	/**
	 * @brief The sub-cells per tile side: the sub-grid zones and props are placed on (0.5 m for a 16 m tile; owner
	 * 2026-10-28: a 7 m carriageway between two 4.5 m sidewalks puts the kerbs on half metres).
	 */
	constexpr uint8_t SubCellsPerSide{32};

	/** @brief A side of a tile, clockwise seen from above. */
	enum class Side : uint8_t
	{
		North = 0, /**< −Z. */
		East = 1, /**< +X. */
		South = 2, /**< +Z. */
		West = 3 /**< −X. */
	};

	/** @brief The four sides, in their clockwise order. */
	constexpr std::array< Side, 4 > Sides{Side::North, Side::East, Side::South, Side::West};

	/** @brief A quarter-turn rotation of a tile about +Y, clockwise seen from above. */
	enum class Rotation : uint8_t
	{
		R0 = 0,
		R90 = 1,
		R180 = 2,
		R270 = 3
	};

	/**
	 * @brief The side of the WORLD a side of the tile faces once the tile is rotated.
	 * @param side The side in the tile's own frame.
	 * @param rotation The tile's rotation.
	 * @return Side
	 */
	[[nodiscard]]
	constexpr
	Side
	rotated (Side side, Rotation rotation) noexcept
	{
		return static_cast< Side >((static_cast< uint8_t >(side) + static_cast< uint8_t >(rotation)) % 4U);
	}

	/** @brief The side facing the given one across a shared edge. */
	[[nodiscard]]
	constexpr
	Side
	opposite (Side side) noexcept
	{
		return static_cast< Side >((static_cast< uint8_t >(side) + 2U) % 4U);
	}

	/**
	 * @brief The networks a side connects to, one bit per network. The CONSUMER names the bits (a road, a rail, a
	 * river…); the engine only checks that two sides sharing an edge carry the same mask.
	 */
	using NetworkMask = uint8_t;

	/** @brief A tile's four corners, clockwise from the north-west one: NW, NE, SE, SW. */
	enum class Corner : uint8_t
	{
		NorthWest = 0,
		NorthEast = 1,
		SouthEast = 2,
		SouthWest = 3
	};

	/** @brief A height per corner, in metres, indexed by Corner. */
	using CornerHeights = std::array< float, 4 >;

	/** @brief The tile axis a slope rule follows. */
	enum class SlopeAxis : uint8_t
	{
		None, /**< Every edge bounded alike. */
		NorthSouth, /**< Slopes along Z; the north and south edges stay level within crossGrade. */
		EastWest /**< Slopes along X; the east and west edges stay level within crossGrade. */
	};

	/**
	 * @brief How steep a tile accepts to be, as grades (rise / run: 0.08 = 8 %).
	 * @note Every edge rises at most maxGrade × tileSize. With an axis, the two edges ACROSS it (parallel to it) rise at
	 * most crossGrade × tileSize: a straight road slopes along its own axis only (OpenTTD's rule).
	 */
	struct SlopeRule
	{
		float maxGrade{0.0F};
		float crossGrade{0.0F};
		SlopeAxis axis{SlopeAxis::None};
	};

	/** @brief A rectangle of sub-cells, in the tile's own frame (x along +X from the west side, z along +Z from the north). */
	struct SubRect
	{
		uint8_t x{0};
		uint8_t z{0};
		uint8_t width{0};
		uint8_t depth{0};
	};

	/**
	 * @brief An area of a tile with a meaning: where something may stand (a building lot, a strip for props…).
	 * @note `kind` is the CONSUMER's (the engine never interprets it); `facing` is the side the area turns to (a lot
	 * faces its street, a lamp post its carriageway).
	 */
	struct Zone
	{
		SubRect area;
		uint32_t kind{0};
		Side facing{Side::North};
	};
}
