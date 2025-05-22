/*
 * src/Audio/Effects/AutoWah.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::Audio::Effects
{
	/**
	 * @brief AutoWah effect.
	 * @warning  Support broken on OpenAL Software 1.19.1.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class AutoWah final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AutoWah"};

			/**
			 * @brief Constructs an auto-wah effect.
			 */
			AutoWah () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the attack time.
			 * @param value A value between 0.0001 and 1.
			 */
			void setAttackTime (float value) noexcept;

			/**
			 * @brief Sets the release time.
			 * @param value A value between 0.0001 and 1.
			 */
			void setReleaseTime (float value) noexcept;

			/**
			 * @brief Sets the resonance.
			 * @param value A value between 2 and 1000.
			 */
			void setResonance (float value) noexcept;

			/**
			 * @brief Sets the peak gain.
			 * @param value A value between 0.00003 and 31621.
			 */
			void setPeakGain (float value) noexcept;

			/**
			 * @brief Returns the attack time value.
			 * @return float
			 */
			[[nodiscard]]
			float attackTime () const noexcept;

			/**
			 * @brief Returns the release time value.
			 * @return float
			 */
			[[nodiscard]]
			float releaseTime () const noexcept;

			/**
			 * @brief Returns the resonance value.
			 * @return float
			 */
			[[nodiscard]]
			float resonance () const noexcept;

			/**
			 * @brief Returns the peak gain value.
			 * @return float
			 */
			[[nodiscard]]
			float peakGain () const noexcept;
	};
}
