/*
 * src/Audio/ListenerMedium.hpp
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
#include <cmath>

namespace EmEn::Audio
{
	/**
	 * @brief What the medium around the LISTENER does to every sound of the source pool: the engine's one statement
	 * of "the listener hears through something else than the open air" (under a sea, today).
	 * @note Already blended by whoever decides it (Scenes::Scene, from the listener's depth under its sea): the audio
	 * manager applies these values as they are, it knows no water (Manager::setListenerMedium()). The default is the
	 * open air: nothing changed.
	 * @note Applied to the pooled sources only (the scene's emitters, its world sounds, its ambience). The music
	 * (TrackMixer) and the manager's default source (interface sounds) are not of the world: untouched (owner
	 * decision 2026-11-26, engine item underwater-sound).
	 */
	struct ListenerMedium final
	{
		/** @brief The gain of the direct path at every frequency, 0 to 1. */
		float gain{1.0F};
		/** @brief The gain of the direct path at high frequencies, 0 to 1: OpenAL's low-pass, a high shelf at 5 kHz. */
		float gainHF{1.0F};
		/** @brief How much of the medium's reverberation is heard, 0 (none, the sources are not sent to it) to 1. */
		float reverbGain{0.0F};
		/** @brief The speed of sound around the listener, in m/s (the Doppler effect). Finite and above zero. */
		float speedOfSound{343.3F};

		/**
		 * @brief Tells whether every value is one the audio manager takes.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			return std::isfinite(gain) && gain >= 0.0F && gain <= 1.0F &&
				std::isfinite(gainHF) && gainHF >= 0.0F && gainHF <= 1.0F &&
				std::isfinite(reverbGain) && reverbGain >= 0.0F && reverbGain <= 1.0F &&
				std::isfinite(speedOfSound) && speedOfSound > 0.0F;
		}

		/**
		 * @brief Tells whether the direct path is left as it is.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		leavesDirectPath () const noexcept
		{
			return gain >= 1.0F && gainHF >= 1.0F;
		}

		/** @brief Member-wise comparison: the manager skips a medium it already applied. */
		[[nodiscard]]
		bool operator== (const ListenerMedium &) const noexcept = default;
	};
}
