/*
 * src/Audio/SoundEnvironmentProperties.hpp
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

/* STL inclusions. */

/* Local inclusions for usages. */
#include "Physics/Physics.hpp"
#include "Types.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The sound environment properties class to define how the sound will be played somewhere.
	 */
	class SoundEnvironmentProperties final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundEnvironmentProperties"};

			/** 
			 * @brief Constructs a default sound environment properties.
			 */
			SoundEnvironmentProperties () noexcept = default;

			/**
			 * @brief Sets the doppler effect factor.
			 * @param dopplerFactor
			 */
			void
			setDopplerFactor (float dopplerFactor) noexcept
			{
				if ( dopplerFactor >= 0.0F )
				{
					m_dopplerFactor = dopplerFactor;
				}
			}

			/**
			 * @brief Returns the current doppler effect factor.
			 * @return float
			 */
			[[nodiscard]]
			float
			dopplerFactor () const noexcept
			{
				return m_dopplerFactor;
			}

			/**
			 * @brief Sets the speed of sound.
			 * @param speed The value in unit per second.
			 */
			void
			setSpeedOfSound (float speed) noexcept
			{
				if ( speed > 0.0F )
				{
					m_speedOfSound = speed;
				}
			}

			/**
			 * @brief Returns the current speed of sound.
			 * @return float
			 */
			[[nodiscard]]
			float
			speedOfSound () const noexcept
			{
				return m_speedOfSound;
			}

			/**
			 * @brief Sets the distance model for the sound attenuation.
			 * @param model One of the DistanceModel enum values.
			 */
			void
			setDistanceModel (DistanceModel model) noexcept
			{
				m_distanceModel = model;
			}

			/**
			 * @brief Returns the current distance model in use for the sound attenuation.
			 * @return DistanceModel
			 */
			[[nodiscard]]
			DistanceModel
			distanceModel () const noexcept
			{
				return m_distanceModel;
			}

		private:

			float m_dopplerFactor{1.0F};
			float m_speedOfSound{Physics::SpeedOfSound::Air< float >};
			DistanceModel m_distanceModel{DistanceModel::Exponent};
	};
}
