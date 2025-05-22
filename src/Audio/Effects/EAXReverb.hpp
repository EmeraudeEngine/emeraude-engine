/*
 * src/Audio/Effects/EAXReverb.hpp
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
	 * @brief Reverb effect (EAX version).
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class EAXReverb final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"EAXReverb"};

			/**
			 * @brief Constructs a EAX reverb effect.
			 */
			EAXReverb () noexcept;

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
			 * @brief Sets the gain at low frequency.
			 * @param value A value between 0 and 1.
			 */
			void setGainLF (float value) noexcept;

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
			 * @brief Sets the decay at low frequency ratio.
			 * @param value A value between 0.1 and 2.
			 */
			void setDecayLFRatio (float value) noexcept;

			/**
			 * @brief Sets the reflections gain.
			 * @param value A value between 0 and 3.16.
			 */
			void setReflectionsGain (float value) noexcept;

			/**
			 * @brief Sets the reflections delay.
			 * @param value A value between 0 and 0.3.
			 */
			void setReflectionsDelay (float value) noexcept;

			/**
			 * @brief Sets the reflections pan.
			 * @param value A free value.
			 */
			void setReflectionsPan (float value) noexcept;

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
			 * @brief Sets the late pan.
			 * @param value A free value.
			 */
			void setLatePan (float value) noexcept;

			/**
			 * @brief Sets the echo time.
			 * @param value A value between 0.075 and 0.25.
			 */
			void setEchoTime (float value) noexcept;

			/**
			 * @brief Sets the echo depth.
			 * @param value A value between 0 and 1.
			 */
			void setEchoDepth (float value) noexcept;

			/**
			 * @brief Sets the modulation time.
			 * @param value A value between 0.04 and 4.
			 */
			void setModulationTime (float value) noexcept;

			/**
			 * @brief Sets the modulation depth.
			 * @param value A value between 0 and 1.
			 */
			void setModulationDepth (float value) noexcept;

			/**
			 * @brief Sets the air absorption gain at high frequency.
			 * @param value A value between 0.892 and 1.
			 */
			void setAirAbsorptionGainHF (float value) noexcept;

			/**
			 * @brief Sets the high frequency reference.
			 * @param value A value between 1000 and 20000.
			 */
			void setHFReference (float value) noexcept;

			/**
			 * @brief Sets the low frequency reference.
			 * @param value A value between 20 and 1000.
			 */
			void setLFReference (float value) noexcept;

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
			 * @brief Returns the gain at low frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float gainLF () const noexcept;

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
			 * @brief Returns the decay at low frequency ratio value.
			 * @return float
			 */
			[[nodiscard]]
			float decayLFRatio () const noexcept;

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
			 * @brief Returns the reflections pan value.
			 * @return float
			 */
			[[nodiscard]]
			float reflectionsPan () const noexcept;

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
			 * @brief Returns the late pan value.
			 * @return float
			 */
			[[nodiscard]]
			float latePan () const noexcept;

			/**
			 * @brief Returns the echo time value.
			 * @return float
			 */
			[[nodiscard]]
			float echoTime () const noexcept;

			/**
			 * @brief Returns the echo depth value.
			 * @return float
			 */
			[[nodiscard]]
			float echoDepth () const noexcept;

			/**
			 * @brief Returns the modulation time value.
			 * @return float
			 */
			[[nodiscard]]
			float modulationTime () const noexcept;

			/**
			 * @brief Returns the modulation depth value.
			 * @return float
			 */
			[[nodiscard]]
			float modulationDepth () const noexcept;

			/**
			 * @brief Returns the air absorption gain at high frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float airAbsorptionGainHF () const noexcept;

			/**
			 * @brief Returns the high frequency reference value.
			 * @return float
			 */
			[[nodiscard]]
			float HFReference () const noexcept;

			/**
			 * @brief Returns the low frequency reference value.
			 * @return float
			 */
			[[nodiscard]]
			float LFReference () const noexcept;

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
