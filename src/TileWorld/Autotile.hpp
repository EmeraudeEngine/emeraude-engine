/*
 * src/TileWorld/Autotile.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::TileWorld
{
	/**
	 * @brief The shape a network takes on a tile, from the sides it leaves by (a 4-bit autotile: 16 cases, 6 shapes).
	 * @note Canonical orientation (Rotation::R0): DeadEnd leaves north; Straight north and south; Corner north and
	 * east; Tee north, east and south (closed west); Cross every side. Reference: the 4-bit bitmask autotile,
	 * https://www.redblobgames.com/articles/autotile/claude/
	 */
	enum class Topology : uint8_t
	{
		Isolated, /**< No side. */
		DeadEnd,
		Straight,
		Corner,
		Tee,
		Cross
	};

	/** @brief A topology and the rotation that turns its canonical orientation onto the requested sides. */
	struct Autotile
	{
		Topology topology{Topology::Isolated};
		Rotation rotation{Rotation::R0};
	};

	/** @brief The bit of a side in a side mask (bit 0 north, 1 east, 2 south, 3 west). */
	[[nodiscard]]
	constexpr
	uint8_t
	sideBit (Side side) noexcept
	{
		return static_cast< uint8_t >(1U << static_cast< uint8_t >(side));
	}

	/** @brief A side mask turned by a rotation (each side moves clockwise by the rotation's quarter turns). */
	[[nodiscard]]
	constexpr
	uint8_t
	rotatedSides (uint8_t sides, Rotation rotation) noexcept
	{
		const auto turns = static_cast< uint32_t >(rotation);
		const auto mask = static_cast< uint32_t >(sides) & 0x0FU;

		return static_cast< uint8_t >(((mask << turns) | (mask >> ((4U - turns) % 4U))) & 0x0FU);
	}

	/** @brief The sides a topology leaves by in its canonical orientation. */
	[[nodiscard]]
	constexpr
	uint8_t
	canonicalSides (Topology topology) noexcept
	{
		switch ( topology )
		{
			case Topology::Isolated :
				return 0x0U;

			case Topology::DeadEnd :
				return sideBit(Side::North);

			case Topology::Straight :
				return sideBit(Side::North) | sideBit(Side::South);

			case Topology::Corner :
				return sideBit(Side::North) | sideBit(Side::East);

			case Topology::Tee :
				return sideBit(Side::North) | sideBit(Side::East) | sideBit(Side::South);

			case Topology::Cross :
				return 0x0FU;
		}

		return 0x0U;
	}

	/**
	 * @brief The topology and the rotation of a tile whose network leaves by the given sides.
	 * @param sides A side mask (sideBit()); the bits above 3 are ignored.
	 * @return Autotile The smallest rotation that matches (Straight: R0 or R90; Cross / Isolated: R0).
	 */
	[[nodiscard]]
	constexpr
	Autotile
	autotile (uint8_t sides) noexcept
	{
		const auto mask = static_cast< uint8_t >(sides & 0x0FU);
		const auto count = ((mask >> 0U) & 1U) + ((mask >> 1U) & 1U) + ((mask >> 2U) & 1U) + ((mask >> 3U) & 1U);

		Topology topology = Topology::Isolated;

		switch ( count )
		{
			case 1 : topology = Topology::DeadEnd; break;
			case 2 : topology = (mask == 0x05U || mask == 0x0AU) ? Topology::Straight : Topology::Corner; break;
			case 3 : topology = Topology::Tee; break;
			case 4 : topology = Topology::Cross; break;
			default : break;
		}

		for ( uint8_t turns = 0; turns < 4; ++turns )
		{
			const auto rotation = static_cast< Rotation >(turns);

			if ( rotatedSides(canonicalSides(topology), rotation) == mask )
			{
				return {.topology = topology, .rotation = rotation};
			}
		}

		return {.topology = topology, .rotation = Rotation::R0};
	}

	static_assert(autotile(0x00U).topology == Topology::Isolated);
	static_assert(autotile(0x0FU).topology == Topology::Cross);
	static_assert(autotile(0x0AU).topology == Topology::Straight && autotile(0x0AU).rotation == Rotation::R90);
	static_assert(autotile(0x09U).topology == Topology::Corner && autotile(0x09U).rotation == Rotation::R270);
	static_assert(autotile(0x0EU).topology == Topology::Tee && autotile(0x0EU).rotation == Rotation::R90);
	static_assert(autotile(0x04U).topology == Topology::DeadEnd && autotile(0x04U).rotation == Rotation::R180);
}
