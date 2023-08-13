/*
 * Emeraude/Audio/AmbienceSound.hpp
 * This file is part of Emeraude
 *
 * Copyright (C) 2012-2023 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude
 * 
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* C/C++ standard libraries. */
#include <memory>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "SoundResource.hpp"

namespace Emeraude::Audio
{
	/**
	 * @brief The ambience sound class.
	 */
	class AmbienceSound final
	{
		public:

			/**
			 * @brief Constructs an ambience sound effect.
			 * @param soundEffect A reference to a sound smart pointer.
			 */
			explicit AmbienceSound (const std::shared_ptr< SoundResource > & soundEffect) noexcept;

			/**
			 * @brief Returns the sound effect resource smart pointer.
			 * @return std::shared_ptr< SoundResource >
			 */
			[[nodiscard]]
			std::shared_ptr< SoundResource > soundResource () const noexcept;

			/**
			 * @brief Sets the channel gain for the sound effect.
			 * @param gain The gain.
			 * @return void
			 */
			void setChannelGain (float gain) noexcept;

			/**
			 * @brief Gets the channel gain for the sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float channelGain () const noexcept;

			/**
			 * @brief Sets the random pitch range values.
			 * @param pitchA A pitch value.
			 * @param pitchB An other pitch value.
			 * @return void
			 */
			void setRandomPitchRange (float pitchA, float pitchB) noexcept;

			/**
			 * @brief Returns the minimum pitch for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float minimumPitch () const noexcept;

			/**
			 * @brief Returns the maximum pitch for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float maximumPitch () const noexcept;

			/**
			 * @brief Returns a random pitch value.
			 * @return float
			 */
			[[nodiscard]]
			float getRandomPitch () const noexcept;

			/**
			 * @brief Sets a radial velocity from which a vector will be created randomly.
			 * @param velocity The velocity value.
			 * @return void
			 */
			void setRadialVelocity (float velocity) noexcept;

			/**
			 * @brief Returns the radial velocity for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float radialVelocity () const noexcept;

			/**
			 * @brief Returns a random velocity vector.
			 * @return float
			 */
			[[nodiscard]]
			Libraries::Math::Vector< 3, float > getRandomVelocity () const noexcept;

			/**
			 * @brief Sets whether the sound is relative or not to the listener.
			 * @param state The state.
			 * @return void
			 */
			void setRelativeState (bool state) noexcept;

			/**
			 * @brief Returns whether the sound is relative or not to the listener.
			 * @return bool
			 */
			[[nodiscard]]
			bool isRelative () const noexcept;

			/**
			 * @brief Returns whether the sound is using pitch randomization.
			 * @return bool
			 */
			[[nodiscard]]
			bool usePitchRandomization () const noexcept;

			/**
			 * @brief Returns whether the sound is using velocity randomization.
			 * @return bool
			 */
			[[nodiscard]]
			bool useVelocityRandomization () const noexcept;

		private:

			std::shared_ptr< SoundResource > m_soundEffect{};
			float m_channelGain{0.75F};
			float m_minimumPitch{1.0F};
			float m_maximumPitch{1.0F};
			float m_radialVelocity{0.0F};
			bool m_relative{false};
	};
}
