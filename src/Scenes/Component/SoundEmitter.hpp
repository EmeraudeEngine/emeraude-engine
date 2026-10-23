/*
 * src/Scenes/Component/SoundEmitter.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Audio/SoundResource.hpp"
#include "Audio/Source.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a sound source emitter.
	 * @note You can virtually define an infinite number of sound emitters, they are not strictly linked to hardware.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Base::ObserverTrait This component observes sound loading events.
	 */
	class EMEN_API SoundEmitter final : public Abstract, public Base::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoundEmitter"};

			/** @brief The number of one-shot voices an emitter can layer over its own sound (playOneShot()). */
			static constexpr size_t MaxOneShotVoices{4};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				EmittingState,
				Gain
			};

			/**
			 * @brief Constructs a sound emitter.
			 * @note [OBS][SHARED-OBSERVER]
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param permanent Set the sound emitter regularly in use. Default false.
			 */
			SoundEmitter (const std::string & componentName, const AbstractEntity & parentEntity, bool permanent = false) noexcept
				: Abstract{componentName, parentEntity}
			{
				this->setFlag(KeepInactiveSourceAlive, permanent);
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SoundEmitter (const SoundEmitter & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SoundEmitter (SoundEmitter && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SoundEmitter &
			 */
			SoundEmitter & operator= (const SoundEmitter & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SoundEmitter &
			 */
			SoundEmitter & operator= (SoundEmitter && copy) noexcept = delete;

			/**
			 * @brief Destroys the sound emitter.
			 * @note Takes care there is no loading sound or playing sound.
			 */
			~SoundEmitter () noexcept override
			{
				this->stop();
			}

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
			void move (const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

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
			 */
			void
			setGain (float gain) noexcept
			{
				if ( !this->acceptsFinite("setGain", gain) )
				{
					return;
				}

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
			 * @brief Returns the sound attached for playback (replay() plays it), or nullptr.
			 * @return const std::shared_ptr< Audio::SoundResource > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Audio::SoundResource > &
			attachedSound () const noexcept
			{
				return m_attachedSound;
			}

			/**
			 * @brief Returns whether the attached sound plays in a loop.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLooping () const noexcept
			{
				return this->isFlagEnabled(Loop);
			}

			/**
			 * @brief Returns whether the emitter holds an audio source (a sound is playing or paused on it).
			 * @note pause(), resume() and rewind() act on that source and do nothing without one.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasSource () const noexcept
			{
				return m_source != nullptr;
			}

			/**
			 * @brief Attaches a sound to the source for further playback.
			 * @param sound A reference to an audio buffer.
			 * @param gain Set the gain for playing the sound. Default 1.0.
			 * @param loop The loop play mode state. Default false.
			 */
			void attachSound (const std::shared_ptr< Audio::SoundResource > & sound, float gain = 1.0F, bool loop = false) noexcept;

			/**
			 * @brief Sends a sound to play to the underlying source.
			 * @note This function will wait the sound to be fully loaded before playing it.
			 * @param sound A reference to an audio buffer.
			 * @param gain Set the gain for playing the sound. Default 1.0.
			 * @param loop The loop play mode state. Default false.
			 * @param replaceSound Replace sound if the source is playing. Default true.
			 */
			void play (const std::shared_ptr< Audio::SoundResource > & sound, float gain = 1.0F, bool loop = false, bool replaceSound = true) noexcept;

			/**
			 * @brief Plays a sound once on an extra voice, layered over the emitter's own sound (the model of Unity's
			 * AudioSource.PlayOneShot()).
			 * @note It neither stops nor replaces what play() or attachSound() started, and it is not that sound:
			 * isPlaying(), setGain(), replay(), rewind() and attachedSound() keep speaking of the emitter's own sound only.
			 * The voice follows the entity (move()), is paused, resumed and stopped with the emitter (pause(), resume(),
			 * stop(), clear(), a scene suspension) and goes back to the audio pool once its sound ended.
			 * @note At most MaxOneShotVoices voices per emitter: when all of them are busy, the next one in turn is cut for
			 * the new sound (the latest event is the one to hear).
			 * @note A one-shot is an event of NOW: a sound not loaded yet is NOT played later (that is play()'s behaviour),
			 * it is refused with a warning — preload it.
			 * @pre Called on the logic thread, the emitter's own (move(), processLogics()).
			 * @param sound A reference to a sound resource smart pointer. A null pointer is refused (traced).
			 * @param gain The gain of this play, its absolute value is used. Default 1. A non-finite value is refused.
			 * @param pitch The pitch of this play, clamped by the source to [0.5, 2]. Default 1. A non-finite value is
			 * refused.
			 * @post Nothing plays when the sound is null or not loaded, a value is not finite (all traced), or the audio
			 * pool has no free source (a saturation, traced in Debug).
			 */
			void playOneShot (const std::shared_ptr< Audio::SoundResource > & sound, float gain = 1.0F, float pitch = 1.0F) noexcept;

			/**
			 * @brief Returns how many one-shot voices are still playing (or paused).
			 * @return size_t In [0, MaxOneShotVoices].
			 */
			[[nodiscard]]
			size_t activeOneShotCount () const noexcept;

			/**
			 * @brief Replays the previous sound if exists.
			 * @note The sound will be rewound if a source is playing.
			 */
			void replay () noexcept;

			/**
			 * @brief Stops the playing, the emitter's own sound and every one-shot voice (released to the pool).
			 */
			void stop () noexcept;

			/**
			 * @brief Pauses the playing, the emitter's own sound and every one-shot voice.
			 */
			void pause () const noexcept;

			/**
			 * @brief Resumes a paused sound, the emitter's own sound and every one-shot voice.
			 */
			void resume () const noexcept;

			/**
			 * @brief Rewinds the sound.
			 */
			void
			rewind () const noexcept
			{
				if ( m_source != nullptr )
				{
					m_source->rewind();
				}
			}

			/**
			 * @brief Returns whether the source is playing something or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPlaying () const noexcept
			{
				if ( m_source == nullptr )
				{
					return false;
				}

				return m_source->isPlaying();
			}

			/**
			 * @brief Removes attached sound.
			 */
			void clear () noexcept;

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override;

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Base::ObservableTrait * observable, int notificationCode, const Base::Any & data) noexcept override;

			/**
			 * @brief Updates a source's position, direction (and velocity, if enabled) with the entity.
			 * @param source A reference to the source, the emitter's own or a one-shot voice.
			 * @param worldCoordinates A reference to a coordinate.
			 */
			void updateSource (Audio::Source & source, const Base::Math::CartesianFrame< float > & worldCoordinates) const noexcept;

			/**
			 * @brief Returns the voice a one-shot plays on: a held voice whose sound ended, else a new one from the pool,
			 * else the next busy one in turn (cut).
			 * @return Audio::SourceRequest * Never nullptr unless the pool has no free source and a slot is still empty.
			 */
			[[nodiscard]]
			Audio::SourceRequest * acquireOneShotVoice () noexcept;

			/**
			 * @brief Stops every one-shot voice and gives it back to the pool.
			 */
			void releaseOneShotVoices () noexcept;

			/**
			 * @brief Plays the attached sound resource.
			 */
			void playAttachedSound () noexcept;

			/**
			 * @brief Releases the attached sound resource.
			 */
			void
			releaseAttachedSound () noexcept
			{
				this->disableFlag(Loop);

				m_attachedSound.reset();
			}

			/* Flag names. */
			static constexpr auto KeepInactiveSourceAlive{UnusedFlag + 0UL};
			static constexpr auto Loop{UnusedFlag + 1UL};
			static constexpr auto VelocityDistortionEnabled{UnusedFlag + 2UL};
			static constexpr auto WasPlayingBeforeSuspend{UnusedFlag + 3UL};

			Audio::SourceRequest m_source;
			std::array< Audio::SourceRequest, MaxOneShotVoices > m_oneShotVoices{};
			size_t m_nextOneShotVoice{0};
			std::shared_ptr< Audio::SoundResource > m_attachedSound;
			float m_gain{1.0F};
	};
}
