/*
 * src/Graphics/SeaCaustics.hpp
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
#include <memory>

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class TextureInterface;
}

namespace EmEn::Graphics
{
	/**
	 * @brief What a light needs to draw the caustics of a sea whose surface moves: how its waves focus the light on
	 * what lies under them.
	 * @note Answered by a sea whose surface moves (Scenes::SeaLevelInterface::caustics(): the FFT ocean); a sea at
	 * rest answers none — its surface focuses nothing, the light under it is only absorbed. Read by the scene, which
	 * files the textures in its bindless table and makes its sun carry the rest
	 * (Scenes::SeaLight; engine item underwater-caustics-and-depth-light).
	 * @note The curvature is the Hessian of each wave cascade's height (Graphics::OceanWaves), one tiling texture a
	 * cascade, with its mip chain; the shelter map says where the land calms the waves, so their caustics.
	 * @warning Valid for the frame it was asked in: the textures belong to the sea's geometry.
	 */
	struct SeaCaustics final
	{
		/** @brief How many wave cascades a sea's caustics are made of. */
		static constexpr std::size_t CascadeCount{3};

		/** @brief The curvature of each cascade: (∂²h/∂x², ∂²h/∂z², ∂²h/∂x∂z) in 1/m, tiling, with mips. */
		std::array< std::shared_ptr< Vulkan::TextureInterface >, CascadeCount > curvature;
		/** @brief The displacement of each cascade: (λ Dx, h, λ Dz) in metres, tiling — where the surface really stands
		 * over a point near the level (a beach a wave has just left is in the air). */
		std::array< std::shared_ptr< Vulkan::TextureInterface >, CascadeCount > displacement;
		/** @brief The shelter map (one channel): 1 the open sea, 0 a sheltered water. */
		std::shared_ptr< Vulkan::TextureInterface > shelter;
		/** @brief The tile side of each cascade, in metres. */
		std::array< float, CascadeCount > cascadeSizes{};
		/** @brief The shelter map's rectangle: its first corner's world X and Z, 1 / its side on X, 1 / its side on Z. */
		std::array< float, 4 > shelterRectangle{};
		/** @brief What a sheltered water keeps of each cascade's waves, 0 to 1. */
		std::array< float, CascadeCount > shelteredShares{};
		/** @brief The bound of the surface's reach above and below the level, in metres. */
		float reach{0.0F};
		/** @brief How much of the caustics is drawn, 0 (none) to 1. */
		float strength{1.0F};
		/** @brief The low-pass of the curvature, in metres per metre of depth: the waves shorter than this share of the
		 * depth have crossed their focus above the receiver. */
		float softening{0.15F};
		/** @brief The depth the caustics fade over (to 1/e), in metres. */
		float fadeDepth{8.0F};
		/** @brief How far a temporal pass gives up its history where the caustics are drawn in full, 0 (not at all) to 1
		 * (the most a GRADED reactive mask asks): the pattern moves over a still receiver, no motion vector follows it,
		 * and an untold history averages it — temporal anti-aliasing halved its contrast (graphics doc 45). The sun's
		 * light pass writes it into the reactive mask, weighted by how much of the caustics the pixel receives. */
		float temporalResponse{1.0F};
		/** @brief How much of the sun's shafts is drawn in the water, 0 (none: the water scatters the sun's light evenly)
		 * to 1: the same caustics, met by the light on its way down, seen in the volume
		 * (Effects::Atmosphere::WaterVolume). */
		float shaftStrength{1.0F};
		/** @brief The anisotropy of the scattering of the sun's light toward the eye (Henyey-Greenstein g), 0 to 0.95. */
		float shaftAnisotropy{0.6F};
	};
}
