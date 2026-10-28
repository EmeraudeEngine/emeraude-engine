/*
 * src/TileWorld/TileInterface.hpp
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
#include <span>
#include <string_view>
#include <utility>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "VertexFactory/Shape.hpp"
#include "Types.hpp"

namespace EmEn::TileWorld
{
	class TileInterface;

	/**
	 * @brief A cell next to the one being built, as the tile sees it (owner decision 2026-10-28: a straight road marks its
	 * approach to a junction itself).
	 * @note A tile reading its neighbours makes its geometry depend on them: an edit rebuilds the edited cell's region
	 * AND the regions of its four neighbours (Builder::buildRegion()).
	 */
	struct Neighbour
	{
		const TileInterface * tile{nullptr}; /**< Null at the world's border or for an empty cell. Non-owning: valid during build() only. */
		Rotation rotation{Rotation::R0}; /**< The neighbour's own rotation in the layout (its sides are in ITS frame). */
	};

	/**
	 * @brief What a tile knows when it builds: its size and the heights of its corners, in its OWN frame (the rotation
	 * already applied: CornerHeights[NorthWest] is the corner at the tile's own north-west).
	 */
	struct TileContext
	{
		CornerHeights heights{};
		Base::Math::Vector< 3, float > centre; /**< The cell's centre, world space (Y = 0). */
		float tileSize{DefaultTileSize};
		uint32_t column{0};
		uint32_t row{0};
		uint32_t variationSeed{0}; /**< Derived from the world seed and the cell, well mixed (any bits may be used): a tile may vary its look with it. */
		Rotation rotation{Rotation::R0}; /**< How the layout turns the tile (for world-aligned texture coordinates). */
		std::array< Neighbour, 4 > neighbours{}; /**< Indexed by the tile's OWN side (Side): the cell across that side. */
	};

	/**
	 * @brief The world X and Z of a point given in a tile's own frame: what a tile uses for world-aligned texture
	 * coordinates (continuous across tiles, whatever their rotations).
	 * @param context The cell's context.
	 * @param x The point's X in the tile's own frame.
	 * @param z The point's Z in the tile's own frame.
	 * @return std::pair< float, float > (world X, world Z).
	 */
	[[nodiscard]]
	constexpr
	std::pair< float, float >
	worldPlanar (const TileContext & context, float x, float z) noexcept
	{
		/* The builder's placement: x' = c·x − s·z, z' = s·x + c·z, with (c, s) of the quarter turns. */
		switch ( context.rotation )
		{
			case Rotation::R0 :
				return {context.centre[Base::Math::X] + x, context.centre[Base::Math::Z] + z};

			case Rotation::R90 :
				return {context.centre[Base::Math::X] - z, context.centre[Base::Math::Z] + x};

			case Rotation::R180 :
				return {context.centre[Base::Math::X] - x, context.centre[Base::Math::Z] - z};

			case Rotation::R270 :
				return {context.centre[Base::Math::X] + z, context.centre[Base::Math::Z] - x};
		}

		return {context.centre[Base::Math::X] + x, context.centre[Base::Math::Z] + z};
	}

	/**
	 * @brief Where a tile writes its geometry: one shape per SURFACE slot (asphalt, concrete, grass…). The consumer
	 * names the slots and gives each its material and its contact properties (TileWorld::Builder).
	 * @note A tile writes in its own frame; the builder rotates and places it, then merges it into its region.
	 */
	class SurfaceSink
	{
		public:

			/** @brief Destructs the sink. */
			virtual ~SurfaceSink () = default;

			SurfaceSink (const SurfaceSink &) = delete;
			SurfaceSink (SurfaceSink &&) = delete;
			SurfaceSink & operator= (const SurfaceSink &) = delete;
			SurfaceSink & operator= (SurfaceSink &&) = delete;

			/**
			 * @brief The shape of a surface slot, to merge the tile's triangles into.
			 * @param slot The consumer's slot index.
			 * @return Base::VertexFactory::Shape< float > * Null when the slot is not declared (the tile then skips it).
			 */
			[[nodiscard]]
			virtual Base::VertexFactory::Shape< float > * surface (uint32_t slot) noexcept = 0;

		protected:

			/** @brief Constructs a sink. */
			SurfaceSink () noexcept = default;
	};

	/**
	 * @brief A kind of tile: what it connects, how steep it accepts to be, where things may stand on it, and its geometry.
	 * @note One instance per KIND, shared by every cell holding it (the layout keeps a shared pointer): a tile holds no
	 * per-cell state. Everything is in the tile's own frame; the layout's rotation turns it.
	 * @note Thread: build() may be called from several threads at once (the builder parallelizes the regions): it must
	 * not mutate the tile.
	 */
	class TileInterface
	{
		public:

			/** @brief Destructs the tile. */
			virtual ~TileInterface () = default;

			TileInterface (const TileInterface &) = delete;
			TileInterface (TileInterface &&) = delete;
			TileInterface & operator= (const TileInterface &) = delete;
			TileInterface & operator= (TileInterface &&) = delete;

			/** @brief The tile's name, for the logs. */
			[[nodiscard]]
			virtual std::string_view name () const noexcept = 0;

			/**
			 * @brief The networks the given side of the tile (own frame) connects to.
			 * @param side The side.
			 * @return NetworkMask 0 when nothing crosses that side.
			 */
			[[nodiscard]]
			virtual NetworkMask connections (Side side) const noexcept = 0;

			/** @brief How steep the tile accepts to be. */
			[[nodiscard]]
			virtual SlopeRule slopeRule () const noexcept = 0;

			/** @brief The tile's zones (own frame). The span lives as long as the tile. */
			[[nodiscard]]
			virtual std::span< const Zone > zones () const noexcept = 0;

			/**
			 * @brief Writes the tile's geometry, in its own frame, over its corner heights.
			 * @param context The cell's context (heights in the tile's own frame).
			 * @param sink Where to write, slot by slot.
			 * @return bool False when the tile could not build (the builder logs it and refuses the world).
			 */
			[[nodiscard]]
			virtual bool build (const TileContext & context, SurfaceSink & sink) const noexcept = 0;

		protected:

			/** @brief Constructs a tile. */
			TileInterface () noexcept = default;
	};
}
