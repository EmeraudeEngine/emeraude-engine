/*
 * src/Audio/Effects/RingModulator.hpp
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
	 * @brief Ring Modulator effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class RingModulator final : public Abstract
	{
		public:

			/** @brief The wave form enumerations for ring modulator. */
			enum class WaveForm
			{
				Sinusoid,
				SawTooth,
				Square
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RingModulator"};

			/**
			 * @brief Constructs a ring modulator effect.
			 */
			RingModulator () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the frequency.
			 * @param value A value between 0 and 8000.
			 */
			void setFrequency (float value) noexcept;

			/**
			 * @brief Sets the high-pass cut-off.
			 * @param value A value between 0 and 24000.
			 */
			void setHighPassCutOff (float value) noexcept;

			/**
			 * @brief Sets the wave form.
			 * @param value A wave form type.
			 */
			void setWaveForm (WaveForm value) noexcept;

			/**
			 * @brief Returns the frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float frequency () const noexcept;

			/**
			 * @brief Returns the high-pass cut-off value.
			 * @return float
			 */
			[[nodiscard]]
			float highPassCutOff () const noexcept;

			/**
			 * @brief Returns the wave form type.
			 * @return WaveForm
			 */
			[[nodiscard]]
			WaveForm waveForm () const noexcept;
	};
}
