/*
 * src/Audio/Effects/VocalMorpher.hpp
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
	 * @brief Vocal Morpher effect.
	 * @warning  Support broken on OpenAL Software 1.19.1.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class VocalMorpher final : public Abstract
	{
		public:

			/** @brief The wave form type enumeration for vocal morpher effect. */
			enum class WaveForm
			{
				Sinusoid,
				Triangle,
				SawTooth
			};

			/**
			 * @brief The phoneme enumeration.
			 * @note Do not remove number.
			 */
			enum class Phoneme
			{
				A = 0, // AL_VOCAL_MORPHER_PHONEME_A
				E = 1, // AL_VOCAL_MORPHER_PHONEME_E,...
				I = 2,
				O = 3,
				U = 4,
				AA = 5,
				AE = 6,
				AH = 7,
				AO = 8,
				EH = 9,
				ER = 10,
				IH = 11,
				IY = 12,
				UH = 13,
				UW = 14,
				B = 15,
				D = 16,
				F = 17,
				G = 18,
				J = 19,
				K = 20,
				L = 21,
				M = 22,
				N = 23,
				P = 24,
				R = 25,
				S = 26,
				T = 27,
				V = 28,
				Z = 29
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VocalMorpher"};

			/**
			 * @brief Constructs a vocal morpher effect.
			 */
			VocalMorpher () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the phoneme A.
			 * @param value A phoneme type.
			 */
			void setPhonemeA (Phoneme value) noexcept;

			/**
			 * @brief Sets the phoneme A coarse tuning.
			 * @param value value A value between -24 and 24.
			 */
			void setPhonemeACoarseTuning (int value) noexcept;

			/**
			 * @brief Sets the phoneme B.
			 * @param value A phoneme type.
			 */
			void setPhonemeB (Phoneme value) noexcept;

			/**
			 * @brief Sets the phoneme B coarse tuning.
			 * @param value A value between -24 and 24.
			 */
			void setPhonemeBCoarseTuning (int value) noexcept;

			/**
			 * @brief Sets the wave form.
			 * @param value A wave form type.
			 */
			void setWaveForm (WaveForm value) noexcept;

			/**
			 * @brief Sets the rate.
			 * @param value A value between 0 and 10.
			 */
			void setRate (float value) noexcept;

			/**
			 * @brief Returns the phoneme A type.
			 * @return Phoneme
			 */
			[[nodiscard]]
			Phoneme phonemeA () const noexcept;

			/**
			 * @brief Returns the phoneme A coarse tune value.
			 * @return int
			 */
			[[nodiscard]]
			int phonemeACoarseTuning () const noexcept;

			/**
			 * @brief Returns the phoneme B type.
			 * @return Phoneme
			 */
			[[nodiscard]]
			Phoneme phonemeB () const noexcept;

			/**
			 * @brief Returns the phoneme B coarse tune value.
			 * @return int
			 */
			[[nodiscard]]
			int phonemeBCoarseTuning () const noexcept;

			/**
			 * @brief Returns the wave form type.
			 * @return WaveForm
			 */
			[[nodiscard]]
			WaveForm waveForm () const noexcept;

			/**
			 * @brief Returns the rate value.
			 * @return float
			 */
			[[nodiscard]]
			float rate () const noexcept;
	};
}
