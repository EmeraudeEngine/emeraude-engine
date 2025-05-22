/*
 * src/Audio/Effects/Equalizer.hpp
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
	 * @brief Equalizer effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class Equalizer final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Equalizer"};

			/**
			 * @brief Constructs an equalizer effect.
			 */
			Equalizer () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the low gain.
			 * @param value A value between 0.126 and 7.943.
			 */
			void setLowGain (float value) noexcept;

			/**
			 * @brief Sets the low cut-off.
			 * @param value A value between 50 and 800.
			 */
			void setLowCutOff (float value) noexcept;

			/**
			 * @brief Sets the middle 1 gain.
			 * @param value A value between 0.126 and 7.943.
			 */
			void setMid1Gain (float value) noexcept;

			/**
			 * @brief Sets the middle 1 center.
			 * @param value A value between 200 and 3000.
			 */
			void setMid1Center (float value) noexcept;

			/**
			 * @brief Sets the middle 1 width.
			 * @param value A value between 0.01 and 1.
			 */
			void setMid1Width (float value) noexcept;

			/**
			 * @brief Sets the middle 2 gain.
			 * @param value A value between 0.126 and 7.943.
			 */
			void setMid2Gain (float value) noexcept;

			/**
			 * @brief Sets the middle 2 center.
			 * @param value A value between 1000 and 8000.
			 */
			void setMid2Center (float value) noexcept;

			/**
			 * @brief Sets the middle 2 width.
			 * @param value A value between 0.01 and 1.
			 */
			void setMid2Width (float value) noexcept;

			/**
			 * @brief Sets the high gain.
			 * @param value A value between 0.126 and 7.943.
			 */
			void setHighGain (float value) noexcept;

			/**
			 * @brief Sets the high cut-off.
			 * @param value A value between 4000 and 16000.
			 */
			void setHighCutOff (float value) noexcept;

			/**
			 * @brief Returns the low gain value.
			 * @return float
			 */
			[[nodiscard]]
			float lowGain () const noexcept;

			/**
			 * @brief Returns the low cut-off value.
			 * @return float
			 */
			[[nodiscard]]
			float lowCutOff () const noexcept;

			/**
			 * @brief Returns the middle 1 gain value.
			 * @return float
			 */
			[[nodiscard]]
			float mid1Gain () const noexcept;

			/**
			 * @brief Returns the middle 1 center value.
			 * @return float
			 */
			[[nodiscard]]
			float mid1Center () const noexcept;

			/**
			 * @brief Returns the middle 1 width value.
			 * @return float
			 */
			[[nodiscard]]
			float mid1Width () const noexcept;

			/**
			 * @brief Returns the middle 2 gain value.
			 * @return float
			 */
			[[nodiscard]]
			float mid2Gain () const noexcept;

			/**
			 * @brief Returns the middle 2 center value.
			 * @return float
			 */
			[[nodiscard]]
			float mid2Center () const noexcept;

			/**
			 * @brief Returns the middle 2 width value.
			 * @return float
			 */
			[[nodiscard]]
			float mid2Width () const noexcept;

			/**
			 * @brief Returns the high gain value.
			 * @return float
			 */
			[[nodiscard]]
			float highGain () const noexcept;

			/**
			 * @brief Returns the high cut-off value.
			 * @return float
			 */
			[[nodiscard]]
			float highCutOff () const noexcept;
	};
}
