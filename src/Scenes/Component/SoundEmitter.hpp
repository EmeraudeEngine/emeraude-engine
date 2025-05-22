/*
 * src/Scenes/Component/SoundEmitter.hpp
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
#include <any>
#include <cstddef>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Audio/SoundResource.hpp"
#include "Audio/Source.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a sound source emitter.
	 * @note You can virtually define infinite number of sound emitters, they are not strictly linked to hardware.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Libs::ObserverTrait This component observes sound loading events.
	 */
	class SoundEmitter final : public Abstract, public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundEmitter"};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				EmittingState,
				Gain
			};

			/**
			 * @brief Constructs a sound emitter.
			 * @param name The name of the renderable instance.
			 * @param parentEntity A reference to the parent entity.
			 * @param permanent Set the sound emitter regularly in use. Default false.
			 */
			SoundEmitter (const std::string & name, const AbstractEntity & parentEntity, bool permanent = false) noexcept;

			/**
			 * @brief Destroys the sound emitter. Takes care there is no loading sound or playing sound.
			 */
			~SoundEmitter () noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/**
			 * @brief Enables/Disables the sound distortion with entity velocity.
			 * @param state The state.
			 * @return void
			 */
			void
			enableVelocityDistortion (bool state) noexcept
			{
				this->setFlag(VelocityDistortionEnabled, state);
			}

			/**
			 * @brief Returns whether the sound distortion with entity velocity is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			velocityDistortionEnabled () const noexcept
			{
				return this->isFlagEnabled(VelocityDistortionEnabled);
			}

			/**
			 * @brief Changes the gain of the source emitter.
			 * @param gain An positive value.
			 * @return void
			 */
			void
			setGain (float gain) noexcept
			{
				m_gain = std::abs(gain);

				if ( this->isPlaying() )
				{
					m_source->setGain(m_gain);
				}
			}

			/**
			 * @brief Returns the last applied gain.
			 * @return float
			 */
			[[nodiscard]]
			float
			gain () const noexcept
			{
				return m_gain;
			}

			/**
			 * @brief Attaches a sound to the source for further playback.
			 * @param sound A reference to an audio buffer.
			 * @param gain Set the gain for playing the sound. Default 1.0.
			 * @param loop The loop play mode state. Default false.
			 * @return void
			 */
			void attachSound (const std::shared_ptr< Audio::SoundResource > & sound, float gain = 1.0F, bool loop = false) noexcept;

			/**
			 * @brief Sends a sound to play to the underlying source.
			 * @note This function will wait the sound to be fully loaded before playing it.
			 * @param sound A reference to an audio buffer.
			 * @param gain Set the gain for playing the sound. Default 1.0.
			 * @param loop The loop play mode state. Default false.
			 * @param replaceSound Replace sound if the source is playing. Default true.
			 * @return void
			 */
			void play (const std::shared_ptr< Audio::SoundResource > & sound, float gain = 1.0F, bool loop = false, bool replaceSound = true) noexcept;

			/**
			 * @brief Replays the previous sound if exists.
			 * @note The sound will be rewound if a source is playing.
			 * @return void
			 */
			void replay () noexcept;

			/**
			 * @brief Stops the playing.
			 * @return void
			 */
			void stop () const noexcept;

			/**
			 * @brief Pauses the playing.
			 * @return void
			 */
			void pause () const noexcept;

			/**
			 * @brief Resumes a paused sound.
			 * @return void
			 */
			void resume () const noexcept;

			/**
			 * @brief Rewinds the sound.
			 * @return void
			 */
			void rewind () const noexcept;

			/**
			 * @brief Returns whether the source is playing something or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool isPlaying () const noexcept;

			/**
			 * @brief Removes attached sound.
			 * @return void
			 */
			void clear () noexcept;

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Gets an available audio source from the manager.
			 * @return bool
			 */
			bool requestSource () noexcept;

			/**
			 * @brief Updates the source properties with the entity.
			 * @param worldCoordinates A reference to a coordinates.
			 * @return void
			 */
			void updateSource (const Libs::Math::CartesianFrame< float > & worldCoordinates) const noexcept;

			/**
			 * @brief Release an audio source.
			 * @return void
			 */
			void releaseSource () noexcept;

			/**
			 * @brief Plays the attached sound resource.
			 * @return void
			 */
			void playAttachedSound () noexcept;

			/**
			 * @brief Releases the attached sound resource.
			 * @return void
			 */
			void releaseAttachedSound () noexcept;

			/* Flag names. */
			static constexpr auto KeepInactiveSourceAlive{UnusedFlag + 0UL};
			static constexpr auto Loop{UnusedFlag + 1UL};
			static constexpr auto VelocityDistortionEnabled{UnusedFlag + 2UL};

			std::shared_ptr< Audio::Source > m_source;
			std::shared_ptr< Audio::SoundResource > m_attachedSound;
			float m_gain{8.0F};
	};
}
