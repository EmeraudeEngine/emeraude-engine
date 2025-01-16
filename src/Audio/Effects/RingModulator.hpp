/*
 * src/Audio/Effects/RingModulator.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for ineritances. */
#include "Abstract.hpp"

namespace Emeraude::Audio::Effects
{
	/**
	 * @brief Ring Modulator effect.
	 * @extends Emeraude::Audio::Effects::Abstract This is an effect.
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

			/** @copydoc Emeraude::Audio::Effects::Abstract::resetProperties() */
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
