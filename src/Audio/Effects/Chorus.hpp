/*
 * src/Audio/Effects/Chorus.hpp
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
	 * @brief Chorus effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class Chorus final : public Abstract
	{
		public:

			/** @brief The wave form enumerations. */
			enum class WaveForm
			{
				Sinusoid,
				Triangle
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Chorus"};

			/**
			 * @brief Constructs a chorus effect.
			 */
			Chorus () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the wave form.
			 * @param value The type of waveform.
			 */
			void setWaveForm (WaveForm value) noexcept;

			/**
			 * @brief Sets the phase.
			 * @param value A value between -180 and 180.
			 */
			void setPhase (int value) noexcept;

			/**
			 * @brief Sets the rate.
			 * @param value A value between 0 and 10.
			 */
			void setRate (float value) noexcept;

			/**
			 * @brief Sets the depth.
			 * @param value A value between 0 and 1.
			 */
			void setDepth (float value) noexcept;

			/**
			 * @brief Sets the feed back.
			 * @param value A value between -1 and 1.
			 */
			void setFeedBack (float value) noexcept;

			/**
			 * @brief Sets the delay.
			 * @param value A value between 0 and 0.016.
			 */
			void setDelay (float value) noexcept;

			/**
			 * @brief Returns the wave form type.
			 * @return WaveForm
			 */
			[[nodiscard]]
			WaveForm waveForm () const noexcept;

			/**
			 * @brief Returns the phase value.
			 * @return int
			 */
			[[nodiscard]]
			int phase () const noexcept;

			/**
			 * @brief Returns the rate value.
			 * @return float
			 */
			[[nodiscard]]
			float rate () const noexcept;

			/**
			 * @brief Returns the depth value.
			 * @return float
			 */
			[[nodiscard]]
			float depth () const noexcept;

			/**
			 * @brief Returns the feed back value.
			 * @return float
			 */
			[[nodiscard]]
			float feedBack () const noexcept;

			/**
			 * @brief Returns the delay value.
			 * @return float
			 */
			[[nodiscard]]
			float delay () const noexcept;
	};
}
