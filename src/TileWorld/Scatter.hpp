/*
 * src/TileWorld/Scatter.hpp
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
#include <optional>
#include <vector>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::TileWorld
{
	class Layout;
	class Relief;
}

namespace EmEn::TileWorld
{
	/** @brief Where a scatter put something. */
	struct Placement
	{
		Base::Math::Vector< 3, float > position; /**< World, on the relief's surface (Relief::surfaceHeight()). */
		Base::Math::Vector< 3, float > facing; /**< World, horizontal, unit: a rectangle zone's side, an arc's centre or away from it. */
		uint32_t seed{0}; /**< The point's own seed: a consumer draws its variety from it (a size, a yaw, a model). */
		uint32_t column{0}; /**< The cell it lies in. */
		uint32_t row{0};
	};

	/** @brief Which zone shapes a scatter fills. */
	enum class ScatterShapes : uint8_t
	{
		All, /**< Rectangles and arcs. */
		Rectangles, /**< ZoneShape::Rectangle only. */
		Arcs /**< ZoneShape::Arc only. */
	};

	/** @brief How to scatter. */
	struct ScatterParameters
	{
		uint32_t zoneKind{0}; /**< The consumer's zone kind to fill. */
		float spacing{8.0F}; /**< The distance between neighbours, metres (≥ MinSpacing). */
		float jitter{0.25F}; /**< How far a point may move from its place, a fraction of its step (0 to 0.5). */
		float margin{0.0F}; /**< Kept free along a zone's edges, metres (≥ 0). */
		uint32_t seed{0}; /**< Mixed with each cell (cellSeed()): an edit changes the points of its cell only. */
		uint32_t minimumPerZone{1}; /**< The fewest points along a usable zone's LENGTH (a rectangle's longer side, an arc) (≤ MaxMinimumPerZone): 1 fills even a short zone, 0 leaves a zone shorter than half a spacing empty (lamps along short strips). Across a rectangle (its shorter side) one row at least. */
		ScatterShapes shapes{ScatterShapes::All};
	};

	/** @brief The smallest spacing a scatter accepts, metres: it bounds the points of a zone. */
	constexpr float MinSpacing{0.25F};

	/** @brief The largest minimumPerZone a scatter accepts. */
	constexpr uint32_t MaxMinimumPerZone{64};

	/** @brief The most points one scatter returns (refused beyond: a wrong spacing, not a city). */
	constexpr size_t MaxPlacements{1U << 20U};

	/**
	 * @brief Scatters points over every zone of a kind in a layout: a JITTERED GRID on a rectangle (stratified sampling,
	 * R. L. Cook, "Stochastic Sampling in Computer Graphics", ACM Transactions on Graphics 5(1), 1986) — evenly spread
	 * without a grid's regularity, two points never closer than step × (1 − 2 × jitter) inside a zone; on an arc, points
	 * along its middle radius every `spacing` of arc length, jittered along it.
	 * @note A zone's usable size (less the margins) holds round(size / spacing) steps a side: along its length at least
	 * minimumPerZone (1 by default), across it at least one (a 1 m strip holds a single file): a 16 m tile at 8 m holds
	 * 2 × 2 points; with a minimum of 0 a zone shorter than half a spacing holds none. A huge spacing with a minimum of 1
	 * puts exactly one point in the middle of each zone. Deterministic on the three OS (std::mt19937 through Base::PortableRandom).
	 * @param layout A validated layout.
	 * @param relief The relief sized for it.
	 * @param parameters How to scatter.
	 * @return std::optional< std::vector< Placement > > Nothing when a parameter is out of range, the relief does not
	 * match the layout, or the points would exceed MaxPlacements (logged).
	 */
	[[nodiscard]]
	EMEN_API
	std::optional< std::vector< Placement > > scatter (const Layout & layout, const Relief & relief, const ScatterParameters & parameters) noexcept;
}
