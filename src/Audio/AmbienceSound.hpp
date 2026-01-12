/*
 * src/Audio/AmbienceSound.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/Utility.hpp"
#include "SoundResource.hpp"

namespace EmEn::Audio
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
			std::shared_ptr< SoundResource >
			soundResource () const noexcept
			{
				return m_soundEffect;
			}

			/**
			 * @brief Sets the channel gain for the sound effect.
			 * @param gain The gain.
			 * @return void
			 */
			void
			setChannelGain (float gain) noexcept
			{
				if ( gain >= 0.0F )
				{
					m_channelGain = gain;
				}
			}

			/**
			 * @brief Gets the channel gain for the sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float
			channelGain () const noexcept
			{
				return m_channelGain;
			}

			/**
			 * @brief Sets the random pitch range values.
			 * @param pitchA A pitch value.
			 * @param pitchB Another pitch value.
			 * @return void
			 */
			void
			setRandomPitchRange (float pitchA, float pitchB) noexcept
			{
				m_minimumPitch = Libs::Utility::ifZero(std::min(pitchA, pitchB), 1.0F);
				m_maximumPitch = Libs::Utility::ifZero(std::max(pitchA, pitchB), 1.0F);
			}

			/**
			 * @brief Returns the minimum pitch for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float
			minimumPitch () const noexcept
			{
				return m_minimumPitch;
			}

			/**
			 * @brief Returns the maximum pitch for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float
			maximumPitch () const noexcept
			{
				return m_maximumPitch;
			}

			/**
			 * @brief Returns a random pitch value.
			 * @return float
			 */
			[[nodiscard]]
			float
			getRandomPitch () const noexcept
			{
				if ( m_minimumPitch == m_maximumPitch )
				{
					return m_minimumPitch;
				}

				return Libs::Utility::quickRandom(m_minimumPitch, m_maximumPitch);
			}

			/**
			 * @brief Sets a radial velocity from which a vector will be created randomly.
			 * @param velocity The velocity value.
			 * @return void
			 */
			void
			setRadialVelocity (float velocity) noexcept
			{
				m_radialVelocity = std::abs(velocity);
			}

			/**
			 * @brief Returns the radial velocity for this sound effect.
			 * @return float
			 */
			[[nodiscard]]
			float
			radialVelocity () const noexcept
			{
				return m_radialVelocity;
			}

			/**
			 * @brief Returns a random velocity vector.
			 * @return float
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getRandomVelocity () const noexcept
			{
				if ( !this->useVelocityRandomization() )
				{
					return {};
				}

				return Libs::Math::Vector< 3, float >::quickRandom(-m_radialVelocity, -m_radialVelocity);
			}

			/**
			 * @brief Sets whether the sound is relative or not to the listener.
			 * @param state The state.
			 * @return void
			 */
			void
			setRelativeState (bool state) noexcept
			{
				m_relative = state;
			}

			/**
			 * @brief Returns whether the sound is relative or not to the listener.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRelative () const noexcept
			{
				return m_relative;
			}

			/**
			 * @brief Returns whether the sound is using pitch randomization.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usePitchRandomization () const noexcept
			{
				return m_minimumPitch != 1.0F && m_maximumPitch != 1.0F;
			}

			/**
			 * @brief Returns whether the sound is using velocity randomization.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useVelocityRandomization () const noexcept
			{
				return m_radialVelocity > 0.0F;
			}

		private:

			std::shared_ptr< SoundResource > m_soundEffect;
			float m_channelGain{0.75F};
			float m_minimumPitch{1.0F};
			float m_maximumPitch{1.0F};
			float m_radialVelocity{0.0F};
			bool m_relative{false};
	};
}
