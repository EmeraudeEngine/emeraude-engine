/*
 * src/Audio/Effects/Reverb.hpp
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
	 * @brief Reverb effect.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class Reverb final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Reverb"};

			/**
			 * @brief Constructs a reverb effect.
			 */
			Reverb () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the density.
			 * @param value A value between 0 and 1.
			 */
			void setDensity (float value) noexcept;

			/**
			 * @brief Sets the diffusion.
			 * @param value A value between 0 and 1.
			 */
			void setDiffusion (float value) noexcept;

			/**
			 * @brief Sets the gain.
			 * @param value A value between 0 and 1.
			 */
			void setGain (float value) noexcept;

			/**
			 * @brief Sets the gain at high frequency.
			 * @param value A value between 0 and 1.
			 */
			void setGainHF (float value) noexcept;

			/**
			 * @brief Sets the decay time.
			 * @param value A value between 0.1 and 20.
			 */
			void setDecayTime (float value) noexcept;

			/**
			 * @brief Sets the decay at high frequency ratio.
			 * @param value A value between 0.1 and 2.
			 */
			void setDecayHFRatio (float value) noexcept;

			/**
			 * @brief Sets the reflections gain.
			 * @param value A value between 0.0 and 3.16.
			 */
			void setReflectionsGain (float value) noexcept;

			/**
			 * @brief Sets the reflections delay.
			 * @param value A value between 0 and 0.3.
			 */
			void setReflectionsDelay (float value) noexcept;

			/**
			 * @brief Sets the late gain.
			 * @param value A value between 0 and 10.
			 */
			void setLateGain (float value) noexcept;

			/**
			 * @brief Sets the late delay.
			 * @param value A value between 0 and 0.1.
			 */
			void setLateDelay (float value) noexcept;

			/**
			 * @brief Sets the air absorption gain at high frequency.
			 * @param value A value between 0.892 and 1.
			 */
			void setAirAbsorptionGainHF (float value) noexcept;

			/**
			 * @brief Sets the room roll-off factor.
			 * @param value A value between 0 and 10.
			 */
			void setRoomRollOffFactor (float value) noexcept;

			/**
			 * @brief Enables the decay high frequency limit.
			 * @param state The state.
			 */
			void setDecayHFLimit (bool state) noexcept;

			/**
			 * @brief Returns the density value.
			 * @return float
			 */
			[[nodiscard]]
			float density () const noexcept;

			/**
			 * @brief Returns the diffusion value.
			 * @return float
			 */
			[[nodiscard]]
			float diffusion () const noexcept;

			/**
			 * @brief Returns the gain value.
			 * @return float
			 */
			[[nodiscard]]
			float gain () const noexcept;

			/**
			 * @brief Returns the gain at high frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float gainHF () const noexcept;

			/**
			 * @brief Returns the decay time value.
			 * @return float
			 */
			[[nodiscard]]
			float decayTime () const noexcept;

			/**
			 * @brief Returns the decay at high frequency ratio value.
			 * @return float
			 */
			[[nodiscard]]
			float decayHFRatio () const noexcept;

			/**
			 * @brief Returns the reflections gain value.
			 * @return float
			 */
			[[nodiscard]]
			float reflectionsGain () const noexcept;

			/**
			 * @brief Returns the reflections delay value.
			 * @return float
			 */
			[[nodiscard]]
			float reflectionsDelay () const noexcept;

			/**
			 * @brief Returns the late gain value.
			 * @return float
			 */
			[[nodiscard]]
			float lateGain () const noexcept;

			/**
			 * @brief Returns the late delay value.
			 * @return float
			 */
			[[nodiscard]]
			float lateDelay () const noexcept;

			/**
			 * @brief Returns the air absorption gain at high frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float airAbsorptionGainHF () const noexcept;

			/**
			 * @brief Returns the room roll-off factor value.
			 * @return float
			 */
			[[nodiscard]]
			float roomRollOffFactor () const noexcept;

			/**
			 * @brief Returns whether the decay at high frequency limit enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool decayHFLimit () const noexcept;
	};
}
