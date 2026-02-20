/*
 * src/Libs/VertexFactory/CapUVMapping.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>

namespace EmEn::Libs::VertexFactory
{
	/**
	 * @brief UV mapping mode for cap surfaces (disk caps on cylinders, cones, tubes, etc.).
	 */
	enum class CapUVMapping : uint8_t
	{
		/** @brief No cap generated. */
		None = 0,
		/** @brief Whole cap uses a single planar UV projection (texture laid flat). */
		Planar = 1,
		/** @brief Each segment gets its own [0,1]x[0,1] UV space (tileable pattern). */
		PerSegment = 2
	};
}
