/*
 * src/Graphics/SeaImmersion.hpp
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

namespace EmEn::Graphics
{
	/**
	 * @brief Where the NEAR plane of a frame's view stands against the scene's sea: the engine's one answer, a frame,
	 * to "is the eye under water?".
	 * @note Decided ONCE a frame, on the CPU, where the effect chain's context is built
	 * (PostProcessor::executeIndirectPostProcessEffects(), IndirectPostProcessEffect::FrameContext::seaImmersion) from
	 * the near plane's lowest and highest points against the sea level and how far the surface can stand from it
	 * (SeaSurfaceWaves::reach, zero for a surface at rest). Every effect reads this state; none decides on its own
	 * (owner decision 2026-11-24, engine item underwater-view).
	 * @note Only a sea that declares its water counts (Scenes::SeaLevelInterface::waterMedium()): without one the state
	 * is Clear.
	 */
	enum class SeaImmersion : uint8_t
	{
		/** @brief The near plane stands clear of the highest the surface can be, or the scene has no sea that declares
		 * its water: no pixel's ray starts in the water. */
		Clear,
		/** @brief The surface can cross the near plane: a pixel's ray may start in the water or in the air, and only a
		 * test per pixel tells (Effects::Atmosphere::WaterVolume stands each pixel against the drawn surface).
		 * ⚠️ A bound, not a sighting: with a surface that reaches 8 m, an eye 5 m above a calm sea is Astride. */
		Astride,
		/** @brief The near plane stands under the lowest the surface can be: every pixel's ray starts in the water. */
		Submerged
	};

	/**
	 * @brief Returns the name of a sea immersion state, for traces.
	 * @param value The state.
	 * @return const char *
	 */
	[[nodiscard]]
	constexpr
	const char *
	to_cstring (SeaImmersion value) noexcept
	{
		switch ( value )
		{
			case SeaImmersion::Clear : return "Clear";
			case SeaImmersion::Astride : return "Astride";
			case SeaImmersion::Submerged : return "Submerged";
		}

		return "Unknown";
	}
}
