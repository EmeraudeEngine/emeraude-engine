/*
 * src/Graphics/MDI/PerDrawData.hpp
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

/* Local inclusions for usages. */
#include "Libs/Math/Matrix.hpp"

namespace EmEn::Graphics::MDI
{
	/**
	 * @brief Per-draw data structure for Multi-Draw Indirect rendering (std430 layout).
	 *
	 * Stored in an SSBO and accessed by the vertex shader via Buffer Device Address (BDA).
	 * Each draw in an MDI batch has one entry indexed by gl_DrawID.
	 *
	 * Memory layout (std430):
	 * - modelMatrix: 64 bytes (mat4)
	 * - frameIndex:   4 bytes (uint)
	 * - padding:     12 bytes (alignment to 16 bytes)
	 * Total: 80 bytes per draw
	 */
	struct PerDrawData final
	{
		/** @brief Model matrix for this draw (world transform). */
		Libs::Math::Matrix< 4, float > modelMatrix;
		/** @brief Animation frame index for materials with animated textures. */
		uint32_t frameIndex{0};
		/** @brief Padding to maintain 16-byte alignment (std430). */
		uint32_t _padding[3]{0, 0, 0};
	};

	static_assert(sizeof(PerDrawData) == 80, "PerDrawData must be 80 bytes for std430 alignment");
	static_assert(alignof(PerDrawData) <= 16, "PerDrawData must be 16-byte aligned or less");

	/** @brief Maximum number of draws in a single MDI batch. */
	constexpr uint32_t MaxMDIDraws{4096};
}
