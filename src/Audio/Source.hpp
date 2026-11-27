/*
 * src/Audio/Source.hpp
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
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <span>
#include <string>

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/efx.h"

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages. */
#include "Math/Base.hpp"
#include "Math/Vector.hpp"
#include "Mutex.hpp"
#include "OpenALExtensions.hpp"
#include "ThreadSafety.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn::Audio
{
	namespace Effects
	{
		class Abstract;
	}

	namespace Filters
	{
		class Abstract;
		class BandPass;
	}

	class EffectSlot;
	class SoundResource;
	struct ListenerMedium;
	struct SoundPath;
}

namespace EmEn::Audio
{
	/**
	 * @brief The Source class
	 * @note It owns its OpenAL name: neither copyable nor movable (a copy or a moved-from object would delete the name
	 * twice).
	 * @extends EmEn::Audio::AbstractObject
	 */
	class EMEN_API Source final : public AbstractObject
	{
		public:

			static constexpr auto ClassId{"AudioSource"};

			/**
			 * @brief Constructs an audio source.
			 */
			Source () noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Source (const Source & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Source (Source && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Source &
			 */
			Source & operator= (const Source & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Source &
			 */
			Source & operator= (Source && copy) noexcept = delete;

			/**
			 * @brief Destructs the audio source.
			 */
			~Source () override;

			/** @copydoc EmEn::Audio::AbstractObject::isCreated() */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				/* NOTE: Relaxed method for some OpenAL version. */
				/*if ( this->identifier() <= 0 )
				{
					return false;
				}

				return alIsSource(this->identifier()) == AL_TRUE;*/

				return this->identifier() > 0;
			}

			/**
			 * @brief Sets the source position.
			 * @param position A reference to a vector.
			 */
			void setPosition (const Base::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief Sets the source position.
			 * @param position A reference to a vector.
			 */
			void setPosition (const Base::Math::Vector< 4, float > & position) noexcept;

			/**
			 * @brief Sets the source position.
			 * @param x The X-axis position value.
			 * @param y The Y-axis position value.
			 * @param z The Z-axis position value.
			 */
			void setPosition (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source position.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > position () const noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param direction A reference to a vector.
			 */
			void setDirection (const Base::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param direction A reference to a vector.
			 */
			void setDirection (const  Base::Math::Vector< 4, float > & direction) noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param x The X-axis direction value.
			 * @param y The Y-axis direction value.
			 * @param z The Z-axis direction value.
			 */
			void setDirection (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source direction.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > direction () const noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param velocity A reference to a vector.
			 */
			void setVelocity (const Base::Math::Vector< 3, float > & velocity) noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param velocity A reference to a vector.
			 */
			void setVelocity (const Base::Math::Vector< 4, float > & velocity) noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param x The X-axis velocity value.
			 * @param y The Y-axis velocity value.
			 * @param z The Z-axis velocity value.
			 */
			void setVelocity (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source velocity.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > velocity () const noexcept;

			/**
			 * @brief Sets the source gain.
			 * @param gain A float from 0.0 to INF+.
			 */
			void setGain (float gain) noexcept;

			/**
			 * @brief Returns the source gain.
			 * @return float
			 */
			[[nodiscard]]
			float
			gain () const noexcept
			{
				return this->getFloatValue(AL_GAIN, 0.0F);
			}

			/**
			 * @brief Sets a minimal and a maximal possible gain for this source.
			 * @param minGain A minimum value.
			 * @param maxGain A maximum value.
			 */
			void boundsGain (float minGain, float maxGain) noexcept;

			/**
			 * @brief Returns the minimal gain allowed by this source.
			 * @return float
			 */
			[[nodiscard]]
			float
			minimumGain () const noexcept
			{
				return this->getFloatValue(AL_MIN_GAIN, 0.0F);
			}

			/**
			 * @brief Returns the maximal gain allowed by this source.
			 * @return float
			 */
			[[nodiscard]]
			float
			maximumGain () const noexcept
			{
				return this->getFloatValue(AL_MAX_GAIN, 0.0F);
			}

			/**
			 * @brief Sets how far the sound will execute to drop.
			 * @param distance A distance in the world units.
			 */
			void setReferenceDistance (float distance) noexcept;

			/**
			 * @brief Returns how far the sound will execute to drop.
			 * @return float
			 */
			[[nodiscard]]
			float
			referenceDistance () const noexcept
			{
				return this->getFloatValue(AL_REFERENCE_DISTANCE, 0.0F);
			}

			/**
			 * @brief Sets how quickly (higher number) the sound will lower or not.
			 * @param factor A value bigger than 0.0. The initial value is 1.0.
			 */
			void setRolloffFactor (float factor) noexcept;

			/**
			 * @brief Returns how quickly (higher number) the sound will lower or not.
			 * @return float
			 */
			[[nodiscard]]
			float
			rolloffFactor () const noexcept
			{
				return this->getFloatValue(AL_ROLLOFF_FACTOR, 0.0F);
			}

			/**
			 * @brief Sets the maximum distance after which the sound will no longer be heard.
			 * @param distance A distance in the world units.
			 */
			void setMaxDistance (float distance) noexcept;

			/**
			 * @brief Returns the maximum distance after which the sound will no longer be heard.
			 * @return float
			 */
			[[nodiscard]]
			float
			maxDistance () const noexcept
			{
				return this->getFloatValue(AL_MAX_DISTANCE, 0.0F);
			}

			/**
			 * @brief Higher method to set the attenuation parameters of the source.
			 * @param rolloffFactor Set how quickly (higher number) the sound will lower or not. The default is 1.0F.
			 * @param referenceDistance Set from what distance the sound will execute to be lowered.
			 * @param maxDistance Set the distance after which the sound will no longer be heard.
			 */
			void
			setAttenuationParameters (float rolloffFactor, float referenceDistance, float maxDistance) noexcept
			{
				this->setRolloffFactor(rolloffFactor);

				this->setReferenceDistance(referenceDistance);

				this->setMaxDistance(maxDistance);
			}

			/**
			 * @brief Sets properties of emission cone.
			 * @warning The parameter "gainFacingAway" is only available with EFX extension.
			 * @param innerAngle The inner angle of the cone where the sound is louder.
			 * @param outerAngle The outer angle of the cone where the sound is fading.
			 * @param outerGain The attenuation gain outside the gain.
			 * @param gainFacingAway The attenuation gain when facing away the cone.
			 */
			void setCone (float innerAngle, float outerAngle, float outerGain, float gainFacingAway) noexcept;

			/**
			 * @brief Returns the cone inner angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			coneInnerAngle () const noexcept
			{
				return this->getFloatValue(AL_CONE_INNER_ANGLE, Base::Math::FullRevolution< float >);
			}

			/**
			 * @brief Returns the cone outer angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			coneOuterAngle () const noexcept
			{
				return this->getFloatValue(AL_CONE_OUTER_ANGLE, Base::Math::FullRevolution< float >);
			}

			/**
			 * @brief Returns the cone gain outside.
			 * @return float
			 */
			[[nodiscard]]
			float
			coneOuterGain () const noexcept
			{
				return this->getFloatValue(AL_CONE_OUTER_GAIN, 0.0F);
			}

			/**
			 * @brief Returns the cone gain when facing away.
			 * @return float
			 */
			[[nodiscard]]
			float
			coneGainFacingAway () const noexcept
			{
				return OpenAL::isEFXAvailable() ? this->getFloatValue(AL_CONE_OUTER_GAINHF, 1.0F) : 1.0F;
			}

			/**
			 * @brief Sets the sound pitch.
			 * @param pitch The value of the pitch to apply to the sound.
			 */
			void setPitch (float pitch) noexcept;

			/**
			 * @brief Returns the sound pitch value.
			 * @return float
			 */
			[[nodiscard]]
			float
			pitch () const noexcept
			{
				return this->getFloatValue(AL_PITCH, 1.0F);
			}

			/**
			 * @brief Sets the air absorption property.
			 * @warning This method only works with the EFX extension.
			 * @param factor The attenuation factor from air absorption.
			 */
			void setAirAbsorption (int factor) noexcept;

			/**
			 * @brief Returns the air absorption property.
			 * @return int
			 */
			[[nodiscard]]
			int airAbsorption () const noexcept;

			/**
			 * @brief Sets a random pitch value.
			 * @param minPitch The minimal value for the pitch.
			 * @param maxPitch The maximum value for the pitch.
			 * @return float The selected pitch value.
			 */
			float setRandomPitch (float minPitch, float maxPitch) noexcept;

			/**
			 * @brief Sets the mute state.
			 * @param state The state.
			 */
			void setMuteState (bool state) noexcept;

			/**
			 * @brief Toggles the mute state.
			 * @return bool
			 */
			bool
			toggleMuteState () noexcept
			{
				if ( this->isMuted() )
				{
					this->setMuteState(false);

					return false;
				}

				this->setMuteState(true);

				return true;
			}

			/**
			 * @brief Sets the relative state to the listener.
			 * @param state The state.
			 */
			void setRelativeState (bool state) noexcept;

			/**
			 * @brief Returns whether the source is muted or not (gain=0.0).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMuted () const noexcept
			{
				return this->gain() <= 0.0F;
			}

			/**
			 * @brief Returns whether the source is playing or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPlaying () const noexcept
			{
				return this->getIntValue(AL_SOURCE_STATE, AL_PLAYING) == AL_PLAYING;
			}

			/**
			 * @brief Returns whether the source is paused or not.
			 * @note This state keeps the sound reading cursor.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPaused () const noexcept
			{
				return this->getIntValue(AL_SOURCE_STATE, AL_PLAYING) == AL_PAUSED;
			}

			/**
			 * @brief Returns whether the source is stopped or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isStopped () const noexcept
			{
				return this->getIntValue(AL_SOURCE_STATE, AL_PLAYING) == AL_STOPPED;
			}

			/**
			 * @brief Returns whether a once-played sound is over: neither playing nor paused (stopped, or still initial
			 * when its play() failed — such a source never reads "stopped").
			 * @note The test a holder of fire-and-forget voices releases them on (SoundEmitter one-shots, the scene's
			 * world sounds).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFinished () const noexcept
			{
				return !this->isPlaying() && !this->isPaused();
			}

			/**
			 * @brief Returns whether the loop mode is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLooping () const noexcept
			{
				return this->getIntValue(AL_LOOPING, AL_FALSE) == AL_TRUE;
			}

			/**
			 * @brief Sets the loop mode.
			 * @param state True to enable looping, false to disable.
			 */
			void
			setLooping (bool state) noexcept
			{
				alSourcei(this->identifier(), AL_LOOPING, state ? AL_TRUE : AL_FALSE);
			}

			/**
			 * @brief Returns the current playback position in seconds.
			 * @return float
			 */
			[[nodiscard]]
			float
			playbackPosition () const noexcept
			{
				return this->getFloatValue(AL_SEC_OFFSET, 0.0F);
			}

			/**
			 * @brief Sets the playback position in seconds.
			 * @param position The position in seconds.
			 */
			void
			setPlaybackPosition (float position) noexcept
			{
				alSourcef(this->identifier(), AL_SEC_OFFSET, position);
			}

			/**
			 * @brief Returns whether the source position is relative to the listener.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRelative () const noexcept
			{
				return this->getIntValue(AL_SOURCE_RELATIVE, AL_FALSE) == AL_TRUE;
			}

			/**
			 * @brief Adds an effect on the source.
			 * @warning This method only works with the EFX extension.
			 * @param effect A reference to an effect smart pointer.
			 * @param channel The effect channel, which is the index of the source's auxiliary send: in
			 * [0, OpenAL::userAuxiliarySendCount()), refused beyond (the last send is the listener's medium's). Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Effects::Abstract > & effect, int channel = 0) noexcept;

			/**
			 * @brief Adds a filter on the source.
			 * @warning This method only works with the EFX extension.
			 * @param filter A reference to a filter smart pointer.
			 * @param channel The effect channel, which is the index of the source's auxiliary send: in
			 * [0, OpenAL::userAuxiliarySendCount()), refused beyond (the last send is the listener's medium's). Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Filters::Abstract > & filter, int channel = 0) noexcept;

			/**
			 * @brief Adds effects and a filter on the source.
			 * @warning This method only works with the EFX extension.
			 * @param effect A reference to an effect smart pointer.
			 * @param filter A reference to a filter smart pointer.
			 * @param channel The effect channel, which is the index of the source's auxiliary send: in
			 * [0, OpenAL::userAuxiliarySendCount()), refused beyond (the last send is the listener's medium's). Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Effects::Abstract > & effect, const std::shared_ptr< Filters::Abstract > & filter, int channel = 0) noexcept;

			/**
			 * @brief Enables an effect/filter slot.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 */
			void enableEffectSlot (int channel) const noexcept;

			/**
			 * @brief Disables an effect/filter slot.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 */
			void disableEffectSlot (int channel) const noexcept;

			/**
			 * @brief Enables a direct filter.
			 * @warning This method only works with the EFX extension.
			 * @param filter A reference to a filter smart pointer.
			 * @return bool
			 */
			bool enableDirectFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept;

			/**
			 * @brief Disables the direct filter.
			 * @warning This method only works with the EFX extension.
			 */
			void disableDirectFilter () noexcept;

			/**
			 * @brief Applies what the medium around the listener does to this source: its direct path's gains, and
			 * the reverberation it is sent to.
			 * @note The audio manager's call, on every source of its pool (Manager::setListenerMedium()). OpenAL gives a
			 * source ONE direct filter: the medium's gains are multiplied with the response of the direct filter a user
			 * enabled (enableDirectFilter()), in a filter this source owns. The reverberation takes the LAST auxiliary
			 * send of the source. Both outlive resetToDefaults(): they are the listener's, not a user's.
			 * @note ANY THREAD (the filter state has its own lock).
			 * @warning This method only works with the EFX extension.
			 * @param medium The medium: its two gains of the direct path are read here, clamped to [0, 1]; a non-finite
			 * one is refused (traced).
			 * @param reverbSlot The OpenAL name of the auxiliary effect slot the source is sent to, or 0 for none.
			 */
			void setListenerMedium (const ListenerMedium & medium, ALuint reverbSlot) noexcept;

			/**
			 * @brief Takes this source out of the listener's medium, or puts it back: an exempt source is neither
			 * filtered by the medium nor sent to its reverberation.
			 * @note For a sound that already IS what the medium sounds like (a recording made under water, played
			 * while the listener is under a sea — owner decision 2026-11-26): treating it again would muffle it twice.
			 * The medium's values are still recorded: a source put back carries the current medium at once. The sound
			 * path (setSoundPath()) and a user's direct filter still apply.
			 * @note ANY THREAD (the filter state has its own lock). The exemption is the user's: resetToDefaults() ends it.
			 * @warning This method only works with the EFX extension.
			 * @param state True to exempt the source.
			 */
			void ignoreListenerMedium (bool state) noexcept;

			/**
			 * @brief States what lies between this source and the listener beyond the listener's own medium: the gains
			 * its sound loses on the way.
			 * @note The call of whoever places the source, each time the path changes (a scene's sound emitter and a
			 * scene's world sound, each logic cycle: the same path costs a comparison). The gains multiply the direct
			 * filter (the user's response and the listener's medium, refreshDirectFilter()) and filter what the source
			 * sends to the listener's reverberation. A user's own effect sends are left as they are.
			 * @note ANY THREAD (the filter state has its own lock). The path is the user's: resetToDefaults() clears it.
			 * @warning This method only works with the EFX extension.
			 * @param path The path. An invalid one (SoundPath::isValid()) is refused (traced).
			 */
			void setSoundPath (const SoundPath & path) noexcept;

			/**
			 * @brief Stops the source, removes its sound and puts every property back to its OpenAL default.
			 * @note The source pool calls it when a source comes back (Manager::releaseSource()): a pooled source is
			 * shared by every user in turn, and each one configures only what it relies on. Without it a source left
			 * relative, pitched, filtered or with short distances by one user (an ambience channel) reached the next one
			 * (a scene sound emitter) that way.
			 * @post Pitch 1, gain 1 (gain bounds 0 / 1), position, velocity and direction 0, absolute (not relative),
			 * reference distance 1, rolloff factor 1, maximum distance FLT_MAX (OpenAL's own default), cone 360° / 360° / outer gain 0, no direct
			 * filter, no effect slot; with EFX, air absorption 0 and the cone's outer HF gain 1.
			 * @note What the listener's medium does to the source stays (setListenerMedium()): it is not a user's. The
			 * sound path (setSoundPath()) and the exemption from the medium (ignoreListenerMedium()) are a user's:
			 * cleared.
			 */
			void resetToDefaults () noexcept;

			/**
			 * @brief Plays a sound on the source.
			 * @note If something is playing or paused. It will be stopped and/or removed.
			 * @param sound A reference to a sound resource smart pointer.
			 * @param mode The playing mode. Default once.
			 * @return bool
			 */
			bool play (const std::shared_ptr< SoundResource > & sound, PlayMode mode = PlayMode::Once) noexcept;

			/**
			 * @brief Adds buffers at the end of the source's queue: the source streams what its owner feeds it.
			 * @note The streaming service of a source (the track mixer's music): the OWNER of the queue owns the
			 * buffers, refills the processed ones and detaches them all (detachBuffers()) before it deletes them. A
			 * source that plays a sound (play()) has no queue. OpenAL's own looping is switched off: a queue loops
			 * through its feeder.
			 * @param buffers The OpenAL names of filled buffers, all of one format.
			 * @return bool False when OpenAL refused them (traced).
			 */
			[[nodiscard]]
			bool queueBuffers (std::span< const ALuint > buffers) noexcept;

			/**
			 * @brief Takes the buffers the source has finished playing out of its queue.
			 * @param buffers Where their OpenAL names are written, oldest first.
			 * @return size_t The number of names written, at most buffers.size().
			 */
			[[nodiscard]]
			size_t unqueueProcessedBuffers (std::span< ALuint > buffers) noexcept;

			/**
			 * @brief Starts (or restarts) the source on what is attached to it.
			 * @return bool False when the audio is disabled or OpenAL refused (traced).
			 */
			bool playQueue () noexcept;

			/**
			 * @brief Returns the number of buffers in the source's queue, the processed ones included.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			queuedBufferCount () const noexcept
			{
				const auto count = this->getIntValue(AL_BUFFERS_QUEUED, 0);

				return count > 0 ? static_cast< size_t >(count) : 0;
			}

			/**
			 * @brief Returns the playback cursor in sample frames, counted from the first buffer still attached.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			sampleOffset () const noexcept
			{
				const auto offset = this->getIntValue(AL_SAMPLE_OFFSET, 0);

				return offset > 0 ? static_cast< size_t >(offset) : 0;
			}

			/**
			 * @brief Moves the playback cursor, in sample frames counted from the first buffer attached.
			 * @param frames The offset; OpenAL refuses one beyond what is attached.
			 */
			void
			setSampleOffset (size_t frames) noexcept
			{
				this->setIntValue(AL_SAMPLE_OFFSET, static_cast< ALint >(frames));
			}

			/**
			 * @brief Stops the source and detaches every buffer from it, a static one and a queue alike.
			 * @note The source holds its sound's resource (m_currentSound) as long as that sound's buffer is attached:
			 * OpenAL refuses to delete an attached buffer (AL_INVALID_OPERATION, "deleting in-use buffer"), which then
			 * stays allocated until the context dies. Called BEFORE the resource is let go; the owner of a queue calls
			 * it before it refills or deletes its buffers.
			 */
			void detachBuffers () const noexcept;

			/**
			 * @brief Pauses the source only if it's playing.
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes a previously paused source.
			 */
			void resume () noexcept;

			/**
			 * @brief Gets the playback cursor at the beginning.
			 */
			void rewind () noexcept;

			/**
			 * @brief Stops the audio source.
			 */
			void stop () noexcept;

			/**
			 * @brief Releases the sound.
			 * @post The source is stopped and no buffer of that sound is attached to it any more (detachBuffers()): the
			 * sound's resource may then delete its buffers.
			 */
			void removeSound () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const Source & obj);

		private:

			/**
			 * @brief Sets a float audio parameter to the audio source.
			 * @param pName The audio source attribute.
			 * @param value The float value.
			 */
			void setFloatValue (ALenum pName, ALfloat value) noexcept;

			/**
			 * @brief Returns a float audio parameter from the audio source.
			 * @param pName The audio source attribute.
			 * @param defaultValue The default value. Default 0.0.
			 * @return ALfloat
			 */
			[[nodiscard]]
			ALfloat getFloatValue (ALenum pName, ALfloat defaultValue = 0.0F) const noexcept;

			/**
			 * @brief Sets an integer audio parameter to the audio source.
			 * @param pName The audio source attribute.
			 * @param value The float value.
			 */
			void setIntValue (ALenum pName, ALint value) noexcept;

			/**
			 * @brief Returns an integer audio parameter from the audio source.
			 * @param pName The audio source attribute.
			 * @param defaultValue The default value. Default 0.
			 * @return ALfloat
			 */
			[[nodiscard]]
			ALint getIntValue (ALenum pName, ALint defaultValue = 0) const noexcept;

			/**
			 * @brief Prepares the effect slot channel.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 * @return bool
			 */
			bool prepareEffectSlot (int channel) noexcept;

			/**
			 * @brief Gives the source its direct filter: none, the user's, or the user's response multiplied by the
			 * listener's medium and by the sound path.
			 */
			void refreshDirectFilter () noexcept EMEN_REQUIRES(m_filterAccess);

			/**
			 * @brief Connects the last auxiliary send of the source to the listener's reverberation — through a filter
			 * carrying the sound path when it is not clear — or frees it.
			 */
			void refreshListenerSend () noexcept EMEN_REQUIRES(m_filterAccess);

			std::map< int, std::shared_ptr< EffectSlot > > m_effectSlots;
			/** @brief Guards the direct filter's state: a user sets its filter while the manager applies the medium. */
			mutable Base::Mutex m_filterAccess;
			std::shared_ptr< const Filters::Abstract > m_directFilter EMEN_GUARDED_BY(m_filterAccess);
			/** @brief The user's response multiplied by the listener's medium. Created the first time it is needed. */
			std::unique_ptr< Filters::BandPass > m_composedFilter EMEN_GUARDED_BY(m_filterAccess);
			/** @brief The sound path's gains on what goes to the listener's reverberation. Created the first time it is needed. */
			std::unique_ptr< Filters::BandPass > m_pathFilter EMEN_GUARDED_BY(m_filterAccess);
			float m_listenerGain EMEN_GUARDED_BY(m_filterAccess){1.0F};
			float m_listenerGainHF EMEN_GUARDED_BY(m_filterAccess){1.0F};
			float m_pathGain EMEN_GUARDED_BY(m_filterAccess){1.0F};
			float m_pathGainHF EMEN_GUARDED_BY(m_filterAccess){1.0F};
			ALuint m_listenerReverbSlot EMEN_GUARDED_BY(m_filterAccess){0};
			/** @brief True while the source is out of the listener's medium (ignoreListenerMedium()). */
			bool m_ignoresListenerMedium EMEN_GUARDED_BY(m_filterAccess){false};
			std::shared_ptr< SoundResource > m_currentSound;
			float m_previousGain{1.0F};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	EMEN_API std::string to_string (const Source & obj) noexcept;

	using SourceRequest = std::unique_ptr< Source, std::function< void( Source * ) > >;
}
