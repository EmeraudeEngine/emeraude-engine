/*
 * src/Graphics/SeaShelterMap.hpp
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
#include <vector>

namespace EmEn::Graphics
{
	/**
	 * @brief How OPEN the water of a sea is over a square of the world: the open sea, or a water land shelters — a
	 * basin enclosed in an island, behind a passage too narrow for the swell (owner decisions 2026-11-24, engine item
	 * ocean-sheltered-water).
	 * @note Built from the scene's ground when the scene presents it to its sea
	 * (Scenes::SeaLevelInterface::adaptToGround(), base VertexFactory::buildShelterMap()), handed to the sea's geometry
	 * (Geometry::OceanSurfaceResource::setShelterMap()), which uploads it: every read of the wave cascades weighs
	 * them by it (Geometry::OceanSurface, THE SHELTER MAP).
	 */
	struct SeaShelterMap final
	{
		/** @brief The openness of each texel, row after row: a row runs along +X, the rows follow +Z. 255 the open
		 * sea, 0 a sheltered water, in between along a fade. side × side values. */
		std::vector< uint8_t > texels;
		/** @brief The number of texels a side. */
		uint32_t side{0};
		/** @brief The world X of the square's first corner, in metres. */
		float originX{0.0F};
		/** @brief The world Z of the square's first corner, in metres. */
		float originZ{0.0F};
		/** @brief The side of the square, in metres. */
		float size{0.0F};
	};
}
