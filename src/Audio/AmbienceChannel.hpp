/*
 * src/Audio/AmbienceChannel.hpp
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

/* Local inclusions. */
#include "Source.hpp"
#include "AmbienceSound.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The ambience channel class.
	 */
	class AmbienceChannel final
	{
		public:

			/**
			 * @brief Constructs an ambience channel.
			 * @param source A reference to an audio source smart pointer.
			 */
			explicit
			AmbienceChannel (SourceRequest && source) noexcept
				: m_source{std::move(source)}
			{

			}

			/**
			 * @brief Sets time before the next sound play from this channel.
			 * @note This will reset the current time to 0.
			 * @param time The delay in milliseconds.
			 * @return void
			 */
			void
			setTimeBeforeNextPlay (unsigned int time) noexcept
			{
				m_timeBeforeNextPlay = time;
				m_time = 0;
			}

			/**
			 * @brief Initializes the channel to play an ambience sound.
			 * @param sound A reference to an ambience sound.
			 * @param radius The radius of playing.
			 * @return unsigned int
			 */
			[[nodiscard]]
			unsigned int play (const AmbienceSound & sound, float radius) noexcept;

			/**
			 * @brief Stops the source.
			 * @param removeSound Remove the sound associated.
			 * @return void
			 */
			void
			stop (bool removeSound) const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->stop();

					if ( removeSound )
					{
						m_source->removeSound();
					}
				}
			}

			/**
			 * @brief Pauses the source using direct OpenAL control.
			 * @return void
			 * @version 0.8.35
			 */
			void
			pause () const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->pause();
				}
			}

			/**
			 * @brief Resumes the source using direct OpenAL control.
			 * @return void
			 * @version 0.8.35
			 */
			void
			resume () const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->resume();
				}
			}

			/** @copydoc EmEn::Audio::Source::setReferenceDistance() */
			void
			setReferenceDistance (float distance) const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->setReferenceDistance(distance);
				}
			}

			/** @copydoc EmEn::Audio::Source::setMaxDistance() */
			void
			setMaxDistance (float distance) const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->setMaxDistance(distance);
				}
			}

			/** @copydoc EmEn::Audio::Source::enableDirectFilter() */
			bool
			enableDirectFilter (const std::shared_ptr< Filters::Abstract > & filter) const noexcept
			{
				if ( m_source == nullptr )
				{
					return false;
				}

				return m_source->enableDirectFilter(filter);
			}

			/** @copydoc EmEn::Audio::Source::disableDirectFilter() */
			void
			disableDirectFilter () const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->disableDirectFilter();
				}
			}

			/**
			 * @brief Updates the current time.
			 * @param time The new current time in milliseconds.
			 * @return void
			 */
			void update (unsigned int time) noexcept;

			/**
			 * @brief Returns whether is time to play the channel or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTimeToPlay () const noexcept
			{
				return m_time > m_timeBeforeNextPlay;
			}

			/**
			 * @brief Sets the position.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void
			setPosition (const Libs::Math::Vector< 3, float > & position) noexcept
			{
				m_position = position;
			}

			/**
			 * @brief Sets a velocity vector to fake a movement.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void
			setVelocity (const Libs::Math::Vector< 3, float > & velocity) noexcept
			{
				m_velocity = velocity;
			}

			/**
			 * @brief Disable the channel velocity.
			 * @return void
			 */
			void
			disableVelocity () noexcept
			{
				m_velocity.reset();
			}

		private:

			SourceRequest m_source;
			Libs::Math::Vector< 3, float > m_position;
			Libs::Math::Vector< 3, float > m_velocity;
			unsigned int m_timeBeforeNextPlay{0U};
			unsigned int m_time{0U};
	};
}
