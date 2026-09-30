/*
 * src/Graphics/LTCTables.hpp
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
#include <cstddef>
#include <cstdint>

/*
 * The fitted LTC tables of the GGX BRDF (Heitz, Dupuy, Hill, Neubelt, "Real-Time Polygonal-Light Shading with Linearly
 * Transformed Cosines", SIGGRAPH 2016; license and provenance in LTCTables.cpp). Consumed by Graphics::LTCTexture, the
 * line light (Scenes::Component::LineLight) samples them — Heitz & Hill, "Real-Time Line- and Disk-Light Shading with
 * Linearly Transformed Cosines", SIGGRAPH 2017 course.
 */
namespace EmEn::Graphics::LTC
{
	/** @brief The side of a table, in texels: x = roughness (perceptual, √α), y = √(1 − cos θ_view). */
	constexpr uint32_t TableSize{64};

	/** @brief Half floats per table: RGBA per texel. */
	constexpr size_t TableValueCount{static_cast< size_t >(TableSize) * TableSize * 4};

	/** @brief The inverse LTC matrix M⁻¹ per texel, RGBA = (m00, m02, m20, m22), the rest of M⁻¹ being (0, 1, 0) (half floats). */
	extern const std::array< uint16_t, TableValueCount > MatrixTable;

	/** @brief The BRDF magnitude and its Fresnel term per texel, RGBA = (magnitude, Fresnel, 0, sphere) (half floats). */
	extern const std::array< uint16_t, TableValueCount > AmplitudeTable;
}
