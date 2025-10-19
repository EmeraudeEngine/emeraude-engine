/*
 * src/Audio/Source.hpp
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
#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <memory>

/* Third-party inclusions. */
#include "OpenALExtensions.hpp"

/* Local inclusions for inheritances. */
#include "AbstractObject.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Base.hpp"
#include "Libs/Math/Vector.hpp"
#include "EffectSlot.hpp"
#include "PlayableInterface.hpp"
#include "Types.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The Source class
	 * @extends EmEn::Audio::AbstractObject
	 */
	class Source final : public AbstractObject
	{
		public:

			/** @brief The source type enumerations. */
			enum class SourceType : uint8_t
			{
				Undetermined,
				Static,
				Streaming
			};

			static constexpr auto ClassId{"AudioSource"};

			/**
			 * @brief Constructs an audio source.
			 */
			Source () noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Source (const Source & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Source (Source && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Source &
			 */
			Source & operator= (const Source & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Source &
			 */
			Source & operator= (Source && copy) noexcept = default;

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
			 * @return void
			 */
			void setPosition (const Libs::Math::Vector< 3, float > & position) noexcept;

			/**
			 * @brief Sets the source position.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void setPosition (const Libs::Math::Vector< 4, float > & position) noexcept;

			/**
			 * @brief Sets the source position.
			 * @param x The X-axis position value.
			 * @param y The Y-axis position value.
			 * @param z The Z-axis position value.
			 * @return void
			 */
			void setPosition (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source position.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > position () const noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param direction A reference to a vector.
			 * @return void
			 */
			void setDirection (const Libs::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param direction A reference to a vector.
			 * @return void
			 */
			void setDirection (const  Libs::Math::Vector< 4, float > & direction) noexcept;

			/**
			 * @brief Sets the direction of the sound emission source.
			 * @param x The X-axis direction value.
			 * @param y The Y-axis direction value.
			 * @param z The Z-axis direction value.
			 * @return void
			 */
			void setDirection (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source direction.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > direction () const noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void setVelocity (const Libs::Math::Vector< 3, float > & velocity) noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void setVelocity (const Libs::Math::Vector< 4, float > & velocity) noexcept;

			/**
			 * @brief Sets the source velocity to simulate doppler effect.
			 * @param x The X-axis velocity value.
			 * @param y The Y-axis velocity value.
			 * @param z The Z-axis velocity value.
			 * @return void
			 */
			void setVelocity (float x, float y, float z) noexcept;

			/**
			 * @brief Returns the source velocity.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > velocity () const noexcept;

			/**
			 * @brief Sets the source gain.
			 * @param gain A float from 0.0 to INF+.
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
				return this->getFloatValue(AL_CONE_INNER_ANGLE, Libs::Math::FullRevolution< float >);
			}

			/**
			 * @brief Returns the cone outer angle.
			 * @return float
			 */
			[[nodiscard]]
			float
			coneOuterAngle () const noexcept
			{
				return this->getFloatValue(AL_CONE_OUTER_ANGLE, Libs::Math::FullRevolution< float >);
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @return void
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
			 * @brief isRelative
			 * @return
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
			 * @param channel The index of the slot. Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Effects::Abstract > & effect, int channel = 0) noexcept;

			/**
			 * @brief Adds a filter on the source.
			 * @warning This method only works with the EFX extension.
			 * @param filter A reference to a filter smart pointer.
			 * @param channel The index of the slot. Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Filters::Abstract > & filter, int channel = 0) noexcept;

			/**
			 * @brief Adds effects and a filter on the source.
			 * @warning This method only works with the EFX extension.
			 * @param effect A reference to an effect smart pointer.
			 * @param filter A reference to a filter smart pointer.
			 * @param channel The index of the slot. Default 0.
			 * @return bool
			 */
			bool addEffect (const std::shared_ptr< Effects::Abstract > & effect, const std::shared_ptr< Filters::Abstract > & filter, int channel = 0) noexcept;

			/**
			 * @brief Enables an effect/filter slot.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 * @return void
			 */
			void enableEffectSlot (int channel) const noexcept;

			/**
			 * @brief Disables an effect/filter slot.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 * @return void
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
			 * @return void
			 */
			void disableDirectFilter () noexcept;

			/**
			 * @brief Plays a sound or music (streamed) to the source.
			 * @note If something is playing or paused. It will be stopped and/or removed.
			 * @param playableInterface A reference to a playable interface smart pointer.
			 * @param mode The playing mode. Default once.
			 * @return bool
			 */
			bool play (const std::shared_ptr< PlayableInterface > & playableInterface, PlayMode mode = PlayMode::Once) noexcept;

			/**
			 * @brief Pauses the source only if it's playing.
			 * @return void
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes a previously paused source.
			 * @return void
			 */
			void resume () noexcept;

			/**
			 * @brief Gets the playback cursor at the beginning.
			 * @return void
			 */
			void rewind () noexcept;

			/**
			 * @brief Stops the audio source.
			 * @return void
			 */
			void stop () noexcept;

			/**
			 * @brief Releases the playable interface.
			 * @return void
			 */
			void removeSound () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Source & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Source & obj) noexcept;

		private:

			/**
			 * @brief Sets a float audio parameter to the audio source.
			 * @param pName The audio source attribute.
			 * @param value The float value.
			 * @return void
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
			 * @return void
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
			 * @brief Clears streaming queue.
			 * @return void
			 */
			void clearStream () const noexcept;

			/**
			 * @brief Prepares the effect slot channel.
			 * @warning This method only works with the EFX extension.
			 * @param channel The channel number.
			 * @return bool
			 */
			bool prepareEffectSlot (int channel) noexcept;

			/**
			 * @brief Returns the amount of buffer queued to the audio source.
			 * @return int
			 */
			[[nodiscard]]
			int
			getBuffersQueuedCount () const noexcept
			{
				return this->getIntValue(AL_BUFFERS_QUEUED, 0);
			}

			/**
			 * @brief Returns the amount of buffer already played by the audio source.
			 * @return int
			 */
			[[nodiscard]]
			int
			getBuffersProcessedCount () const noexcept
			{
				return this->getIntValue(AL_BUFFERS_PROCESSED, 0);
			}

			/**
			 * @brief Returns the source type.
			 * @return SourceType
			 */
			[[nodiscard]]
			SourceType
			getSourceType () const noexcept
			{
				switch ( this->getIntValue(AL_SOURCE_TYPE, 0) )
				{
					case AL_STATIC :
						return SourceType::Static;

					case AL_STREAMING :
						return SourceType::Streaming;

					case AL_UNDETERMINED :
					default:
						return SourceType::Undetermined;
				}
			}

			std::map< int, std::shared_ptr< EffectSlot > > m_effectSlots;
			std::shared_ptr< const Filters::Abstract > m_directFilter;
			std::shared_ptr< PlayableInterface > m_currentPlayableInterface;
			float m_previousGain{1.0F};
	};

	using SourceRequest = std::unique_ptr< Source, std::function< void( Source * ) > >;
}
