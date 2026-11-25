/*
 * src/Scenes/SeaLight.hpp
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

namespace EmEn::Scenes
{
	/**
	 * @brief How a sea lets a directional light reach what lies under its level: the absorption along the light's own path
	 * and the caustics of the waves (engine item underwater-caustics-and-depth-light).
	 * @note Carried at the end of the light's block (seven vec4, Saphir::Keys::UniformBlock::Component::SeaWater and
	 * the six that follow) and read by the lit shaders (Saphir::LightGenerator, the PBR light pass).
	 */
	struct SeaLight final
	{
		/** @brief The bindless slot of a texture that is not carried. */
		static constexpr uint32_t NoTexture{0xFFFFFFFFU};

		/** @brief What the water takes from the light per metre of its path, per channel (1/m). */
		std::array< float, 3 > attenuation{};
		/** @brief The sea level, in metres (world Y). */
		float level{0.0F};
		/** @brief The tile side of each wave cascade, in metres. */
		std::array< float, 3 > cascadeSizes{1.0F, 1.0F, 1.0F};
		/** @brief How much of the caustics is drawn, 0 (none) to 1. */
		float strength{0.0F};
		/** @brief The shelter map's rectangle: its first corner's world X and Z, 1 / its side on X, 1 / its side on Z. */
		std::array< float, 4 > shelterRectangle{};
		/** @brief What a sheltered water keeps of each cascade's waves, 0 to 1. */
		std::array< float, 3 > shelteredShares{};
		/** @brief The low-pass of the waves' curvature, in metres per metre of depth. */
		float softening{0.0F};
		/** @brief The bindless 2D slot of each cascade's curvature. */
		std::array< uint32_t, 3 > curvatureSlots{NoTexture, NoTexture, NoTexture};
		/** @brief The bindless 2D slot of the shelter map. */
		uint32_t shelterSlot{NoTexture};
		/** @brief The bindless 2D slot of each cascade's displacement. */
		std::array< uint32_t, 3 > displacementSlots{NoTexture, NoTexture, NoTexture};
		/** @brief The bound of the surface's reach above and below the level, in metres. */
		float reach{0.0F};
		/** @brief The depth the caustics fade over (to 1/e), in metres. */
		float fadeDepth{1.0F};
		/** @brief How far a temporal pass gives up its history where the caustics are drawn in full, 0 to 1
		 * (Graphics::SeaCaustics::temporalResponse). */
		float temporalResponse{0.0F};
		/** @brief Whether a sea is carried at all. */
		bool enabled{false};

		[[nodiscard]]
		bool operator== (const SeaLight &) const noexcept = default;
	};
}
