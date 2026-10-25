/*
 * src/Graphics/SkyStarField.hpp
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
#include <memory>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class StarCatalogueResource;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The stars of a frame, as the chain's star field draws them (Effects::Atmosphere::StarField, graphics doc 42
	 * § "The stars"): the physical sky's catalogue and where the celestial sphere stands.
	 * @note Recorded by the renderer with the atmosphere the frame submitted (Renderer::frameStarField()), one per frame
	 * in flight: the slot keeps the catalogue alive for the frame. Only a scene with a celestial clock has one — without
	 * the clock's equatorial frame the stars have no place.
	 */
	struct SkyStarField
	{
		/** @brief The most planets a frame carries (Scenes::SkyState::MaxPlanets). */
		static constexpr uint32_t MaxPlanets{8};

		/**
		 * @brief A point of the frame that is not in the catalogue: a planet.
		 */
		struct Point
		{
			/** @brief Unit world vector toward it. */
			Base::Math::Vector< 3, float > direction{0.0F, 1.0F, 0.0F};
			/** @brief Its illuminance above the atmosphere × its unit-luminance colour, in lux. */
			Base::Math::Vector< 3, float > illuminance{0.0F, 0.0F, 0.0F};
			/** @brief Its apparent angular radius, in radians (the footprint widens to its disc). */
			float angularRadius{0.0F};
		};

		/** @brief The loaded catalogue (immutable once loaded: any thread reads it). */
		std::shared_ptr< const StarCatalogueResource > catalogue;
		/** @brief The planet's equatorial axes in world vectors (Scenes::SkyState::equatorialFrame). */
		std::array< Base::Math::Vector< 3, float >, 3 > equatorialFrame{{{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F}}};
		/** @brief The planets (a calendar clock's), `planetCount` of them. */
		std::array< Point, MaxPlanets > planets{};
		/** @brief Number of planets in `planets`. */
		uint32_t planetCount{0};
	};
}
