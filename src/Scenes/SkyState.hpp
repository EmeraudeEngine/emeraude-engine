/*
 * src/Scenes/SkyState.hpp
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

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief A body of the sky as the renderer sees it: where it stands and what it pours ABOVE the atmosphere.
	 */
	struct SkyBody
	{
		/** @brief Unit world vector toward the body. */
		Base::Math::Vector< 3, float > direction{0.0F, 1.0F, 0.0F};
		/** @brief Illuminance on a surface facing the body above the atmosphere, in lux (the sky scatters it). */
		float illuminance{0.0F};
		/** @brief Colour of its light above the atmosphere, a unit-luminance chromaticity (linear RGB). */
		Base::Math::Vector< 3, float > color{1.0F, 1.0F, 1.0F};
		/** @brief Apparent angular diameter of its disc, in degrees. */
		float angularDiameter{0.53F};
		/** @brief Illuminated fraction of its disc, in [0, 1] (1 for a star). */
		float illuminatedFraction{1.0F};
		/** @brief A moon: unit world vector along its orbit's pole, the axis its face is oriented by. */
		Base::Math::Vector< 3, float > pole{0.0F, 1.0F, 0.0F};
		/** @brief A moon: its illuminance when FULL above the atmosphere, in lux — the calibration of its disc (the
		 * disc-integrated radiance at full equals it). */
		float fullIlluminance{0.0F};
		/** @brief A moon: the planet's illuminance on it relative to the star's at this phase (the earthshine, gain
		 * included). */
		float earthshine{0.0F};
	};

	/**
	 * @brief The state of the sky's bodies for one rendered frame: published by the scene's
	 * Component::CelestialClock, read by the sky the renderer draws (Graphics::Renderable::DynamicSkyResource).
	 * @note The sky resource is SHARED and stateless: the time, hence the sun, belongs to the scene (owner decision
	 * 2026-10-24, the model of the ParticipatingMedium). A scene without a clock has an INVALID state, and a dynamic
	 * sky then keeps its own default sun.
	 */
	struct SkyState
	{
		/** @brief The most moons the state carries. */
		static constexpr uint32_t MaxMoons{4};

		/** @brief The star the atmosphere scatters (one in lot 1 of the dynamic sky, owner decision 2026-10-24). */
		SkyBody sun{};
		/** @brief The moons, `moonCount` of them. */
		std::array< SkyBody, MaxMoons > moons{};
		/** @brief Unit world vector toward the northern celestial pole: the axis the stars turn around. */
		Base::Math::Vector< 3, float > celestialPole{0.0F, 1.0F, 0.0F};
		/** @brief The planet's EQUATORIAL frame in world vectors, the stars' frame (graphics doc 42 § "The stars"): [0] toward
		 * right ascension 0 on the equator (the vernal equinox), [1] toward right ascension 90°, [2] toward the northern
		 * celestial pole. A catalogue direction (x, y, z) is the world vector x [0] + y [1] + z [2].
		 * @note ⚠️ The clock's equator of DATE is taken for the catalogue's J2000 one: the precession turns them about
		 * 0.36° apart by 2026, under the clock's own model (no nutation, no aberration). */
		std::array< Base::Math::Vector< 3, float >, 3 > equatorialFrame{{{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F}}};
		/** @brief Local sidereal angle, in radians: the rotation of the celestial sphere around the pole. */
		float siderealAngle{0.0F};
		/** @brief Number of moons in `moons`. */
		uint32_t moonCount{0};
		/** @brief False when no clock publishes. */
		bool valid{false};
	};
}
