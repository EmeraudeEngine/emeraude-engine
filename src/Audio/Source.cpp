/*
 * src/Audio/Source.cpp
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

#include "Source.hpp"

/* STL inclusions. */
#include <cstddef>
#include <vector>

/* Local inclusions. */
#include "Manager.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	Source::Source () noexcept
	{
		/* NOTE: Fake a source ID to silent all methods. */
		if ( !Manager::instance()->usable() )
		{
			*(this->identifierPointer()) = 1;

			return;
		}

		alFlushErrors();

		alGenSources(1, this->identifierPointer());

		if ( alGetErrors("alGenSources()", __FILE__, __LINE__) || this->identifier() <= 0 )
		{
			Tracer::error(ClassId, "Unable to generate a source !");
		}
	}

	Source::~Source ()
	{
		if ( this->isCreated() )
		{
			alDeleteSources(1, this->identifierPointer());
		}

		this->resetIdentifier();
	}

	bool
	Source::prepareEffectSlot (int channel) noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return false;
		}

		if ( !EFX::isAvailable() )
		{
			Tracer::warning(ClassId, "OpenAL EFX is not supported !");

			return false;
		}

		if ( channel >= EFX::getMaxAuxiliarySends() )
		{
			TraceWarning{ClassId} << "There is only max " << EFX::getMaxAuxiliarySends() << " effect slots per source !";

			return false;
		}

		/* Checks if effect slot is not already created. */
		const auto effectIt = m_effectSlots.find(channel);

		if ( effectIt != m_effectSlots.cend() )
		{
			return true;
		}

		/* Creates a new effect slot */
		auto effectSlot = std::make_shared< EffectSlot >();

		if ( !effectSlot->isCreated() )
		{
			return false;
		}

		return m_effectSlots.emplace(channel, effectSlot).second;
	}

	void
	Source::setPosition (const Math::Vector< 3, float > & position) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_POSITION, position.data());
	}

	void
	Source::setPosition (const Math::Vector< 4, float > & position) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_POSITION, position.data());
	}

	void
	Source::setPosition (float x, float y, float z) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSource3f(this->identifier(), AL_POSITION, x, y ,z);
	}

	Math::Vector< 3, float >
	Source::position () const noexcept
	{
		Math::Vector< 3, float > position{};

		if ( Manager::instance()->usable() )
		{
			if ( this->isCreated() )
			{
				alGetSourcefv(this->identifier(), AL_POSITION, position.data());
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get position from an unloaded audio source !");
			}
		}

		return position;
	}

	void
	Source::setDirection (const Math::Vector< 3, float > & direction) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_DIRECTION, direction.data());
	}

	void
	Source::setDirection (const Math::Vector< 4, float > & direction) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_DIRECTION, direction.data());
	}

	void
	Source::setDirection (float x, float y, float z) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSource3f(this->identifier(), AL_DIRECTION, x, y ,z);
	}

	Math::Vector< 3, float >
	Source::direction () const noexcept
	{
		Math::Vector< 3, float > direction;

		if ( Manager::instance()->usable() )
		{
			if ( this->isCreated() )
			{
				alGetSourcefv(this->identifier(), AL_DIRECTION, direction.data());
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get direction from an unloaded audio source !");
			}
		}

		return direction;
	}

	void
	Source::setVelocity (const Math::Vector< 3, float > & velocity) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_VELOCITY, velocity.data());
	}

	void
	Source::setVelocity (const Math::Vector< 4, float > & velocity) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcefv(this->identifier(), AL_VELOCITY, velocity.data());
	}

	void
	Source::setVelocity (float x, float y, float z) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSource3f(this->identifier(), AL_VELOCITY, x, y ,z);
	}

	Math::Vector< 3, float >
	Source::velocity () const noexcept
	{
		Math::Vector< 3, float > velocity;

		if ( Manager::instance()->usable() )
		{
			if ( this->isCreated() )
			{
				alGetSourcefv(this->identifier(), AL_VELOCITY, velocity.data());
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get velocity from an unloaded audio source !");
			}
		}

		return velocity;
	}

	void
	Source::setGain (float gain) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		if ( gain < 0.0F )
		{
			Tracer::warning(ClassId, "Gain must zero or positive !");

			gain = 0.0F;
		}

		alSourcef(this->identifier(), AL_GAIN, gain);
	}

	void
	Source::boundsGain (float minGain, float maxGain) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcef(this->identifier(), AL_MIN_GAIN, Math::clampToUnit(minGain));
		alSourcef(this->identifier(), AL_MAX_GAIN, Math::clampToUnit(maxGain));
	}

	void
	Source::setReferenceDistance (float distance) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		/* The distance in units that no attenuation occurs. At 0.0, no distance
		 * attenuation ever occurs on non-linear attenuation models. (default:1, range:0-INF+). */
		if ( distance < 0.0F )
		{
			Tracer::warning(ClassId, "Reference distance must be zero or positive !");

			return;
		}

		alSourcef(this->identifier(), AL_REFERENCE_DISTANCE, distance);
	}

	void
	Source::setRolloffFactor (float factor) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		/* Multiplier to exaggerate or diminish distance attenuation. (default:1, range:0-INF+). */
		if ( factor < 0.0F )
		{
			Tracer::warning(ClassId, "Rolloff factor must be zero or positive !");

			return;
		}

		alSourcef(this->identifier(), AL_ROLLOFF_FACTOR, factor);
	}

	void
	Source::setMaxDistance (float distance) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		/* The distance above which the source is not attenuated any further with a
		 * clamped distance model, or where attenuation reaches 0.0 gain for linear
		 * distance models with a default rolloff factor. (default:INF+, range:0-INF+). */
		if ( distance < 0.0F )
		{
			Tracer::warning(ClassId, "Max distance must be zero or positive !");

			return;
		}

		alSourcef(this->identifier(), AL_MAX_DISTANCE, distance);
	}

	void
	Source::setCone (float innerAngle, float outerAngle, float outerGain, float gainFacingAway) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		/* The angle covered by the inner cone, where the source will not attenuate. (default 360) */
		alSourcef(this->identifier(), AL_CONE_INNER_ANGLE, std::clamp(innerAngle, 0.0F, FullRevolution< float >));

		/* The angle covered by the outer cone, where the source will be fully. (default 360) */
		alSourcef(this->identifier(), AL_CONE_OUTER_ANGLE, std::clamp(outerAngle, 0.0F, FullRevolution< float >));

		/* The gain attenuation applied when the listener is outside the source's outer cone. Default 0 */
		alSourcef(this->identifier(), AL_CONE_OUTER_GAIN, Math::clampToUnit(outerGain));

		/* the gain when outside the oriented cone. Default 1. */
		if ( EFX::isAvailable() )
		{
			alSourcef(this->identifier(), AL_CONE_OUTER_GAINHF, Math::clampToUnit(gainFacingAway));
		}
	}

	void
	Source::setPitch (float pitch) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcef(this->identifier(), AL_PITCH, std::clamp(pitch, 0.5F, 2.0F));
	}

	void
	Source::setAirAbsorption (int factor) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		if ( !EFX::isAvailable() )
		{
			Tracer::warning(ClassId, "OpenAL EFX is not supported !");

			return;
		}

		const ALint value = std::clamp(factor, 0, 10);

		alSourcei(this->identifier(), AL_AIR_ABSORPTION_FACTOR, value);
	}

	int
	Source::airAbsorption () const noexcept
	{
		if ( !EFX::isAvailable() )
		{
			Tracer::warning(ClassId, "OpenAL EFX is not supported !");

			return 0;
		}

		return this->getIntValue(AL_AIR_ABSORPTION_FACTOR, 0);
	}

	float
	Source::setRandomPitch (float minPitch, float maxPitch) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return 1.0F;
		}

		/* Clamp min and max to avoid unpleasant sound distortion. */
		const auto min = std::max(0.5F, minPitch);
		const auto max = std::min(2.0F, maxPitch);

		auto rand = Utility::quickRandom(min, max);

		alSourcef(this->identifier(), AL_PITCH, rand);

		return rand;
	}

	void
	Source::setMuteState (bool state) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		if ( state )
		{
			/* Save old gain. */
			alGetSourcef(this->identifier(), AL_GAIN, &m_previousGain);

			/* Then set it to zero. */
			alSourcef(this->identifier(), AL_GAIN, 0.0F);
		}
		else
		{
			/* Gets back old gain. */
			alSourcef(this->identifier(), AL_GAIN, m_previousGain);
		}
	}

	void
	Source::setRelativeState (bool state) noexcept
	{
		if ( !Manager::instance()->usable() || !this->isCreated() )
		{
			return;
		}

		alSourcei(this->identifier(), AL_SOURCE_RELATIVE, state ? AL_TRUE : AL_FALSE);
	}

	bool
	Source::addEffect (const std::shared_ptr< Effects::Abstract > & effect, int channel) noexcept
	{
		if ( !this->prepareEffectSlot(channel) )
		{
			return false;
		}

		auto & effectSlot = m_effectSlots[channel];

		if ( !effectSlot->setEffect(effect) )
		{
			return false;
		}

		return effectSlot->enable(this);
	}

	bool
	Source::addEffect (const std::shared_ptr< Filters::Abstract > & filter, int channel) noexcept
	{
		if ( !this->prepareEffectSlot(channel) )
		{
			return false;
		}

		auto & effectSlot = m_effectSlots[channel];

		if ( !effectSlot->setFilter(filter) )
		{
			return false;
		}

		return effectSlot->enable(this);
	}

	bool
	Source::addEffect (const std::shared_ptr< Effects::Abstract > & effect, const std::shared_ptr< Filters::Abstract > & filter, int channel) noexcept
	{
		if ( !this->prepareEffectSlot(channel) )
		{
			return false;
		}

		auto & effectSlot = m_effectSlots[channel];

		if ( !effectSlot->setEffect(effect) )
		{
			return false;
		}

		if ( !effectSlot->setFilter(filter) )
		{
			return false;
		}

		return effectSlot->enable(this);
	}

	void
	Source::enableEffectSlot (int channel) const noexcept
	{
		const auto effectIt = m_effectSlots.find(channel);

		if ( effectIt == m_effectSlots.cend() )
		{
			TraceWarning{ClassId} << "There is no effect slot #" << channel << " with this source !";

			return;
		}

		effectIt->second->enable(this);
	}

	void
	Source::disableEffectSlot (int channel) const noexcept
	{
		const auto effectIt = m_effectSlots.find(channel);

		if ( effectIt == m_effectSlots.cend() )
		{
			TraceWarning{ClassId} << "There is no effect slot #" << channel << " with this source !";

			return;
		}

		effectIt->second->disable(this);
	}

	bool
	Source::enableDirectFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept
	{
		if ( !Manager::instance()->usable() || !EFX::isAvailable() )
		{
			return false;
		}

		if ( filter == nullptr )
		{
			Tracer::error(ClassId, "Filter is a null pointer !");

			return false;
		}

		m_directFilter = filter;

		alSourcei(this->identifier(), AL_DIRECT_FILTER, static_cast< ALint >(m_directFilter->identifier()));

		return true;
	}

	void
	Source::disableDirectFilter () noexcept
	{
		if ( !Manager::instance()->usable() || !EFX::isAvailable() )
		{
			return;
		}

		alSourcei(this->identifier(), AL_DIRECT_FILTER, AL_FILTER_NULL);

		m_directFilter.reset();
	}

	bool
	Source::play (const std::shared_ptr< PlayableInterface > & playableInterface, PlayMode mode) noexcept
	{
		if ( !Manager::instance()->isAudioEnabled() )
		{
			return false;
		}

		alFlushErrors();

		/* Check the audio source. */
		if ( !this->isCreated() )
		{
			Tracer::error(ClassId, "Source is not on audio memory !");

			return false;
		}

		if ( playableInterface != m_currentPlayableInterface )
		{
			if ( playableInterface == nullptr )
			{
				Tracer::error(ClassId, "Trying to play a null pointer !");

				return false;
			}

			m_currentPlayableInterface = playableInterface;
		}

		/* Checks if the source is audible. */
		if ( this->isMuted() )
		{
			TraceDebug{ClassId} << "Trying to play a sound on a muted source.";

			return false;
		}

		/* We stop the source if playing something already. */
		ALint state = 0;

		/* NOTE: Can be AL_INITIAL, AL_PLAYING, AL_PAUSED or AL_STOPPED */
		alGetSourcei(this->identifier(), AL_SOURCE_STATE, &state);

		switch ( state )
		{
			case AL_PLAYING :
			case AL_PAUSED :
				this->stop();

				if ( this->getSourceType() == SourceType::Streaming )
				{
					this->clearStream();
				}
				break;

			default:
				break;
		}

		/* 3. Playback. */
		{
			/* Configuring the looping state. */
			alSourcei(this->identifier(), AL_LOOPING, mode == PlayMode::Loop ? AL_TRUE : AL_FALSE);

			/* Checks if the sample is streamable or not. */
			const auto bufferCount = m_currentPlayableInterface->streamable();

			if ( bufferCount > 0 )
			{
				std::vector< ALuint > identifiers{};

				/* Packing identifier. */
				identifiers.reserve(bufferCount);

				for ( size_t index = 0; index < bufferCount; index++ )
				{
					identifiers.push_back(m_currentPlayableInterface->buffer(index)->identifier());
				}

				alSourceQueueBuffers(this->identifier(), static_cast< ALsizei >(bufferCount), identifiers.data());
			}
			else
			{
				alSourcei(this->identifier(), AL_BUFFER, static_cast< ALint >(m_currentPlayableInterface->buffer()->identifier()));
			}

			/* Let's play the source. */
			alSourcePlay(this->identifier());
		}

		return !alGetErrors(__PRETTY_FUNCTION__, static_cast< const char * >(__FILE__), __LINE__);
	}

	void
	Source::pause () noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		if ( this->getIntValue(AL_SOURCE_STATE, AL_INITIAL) != AL_PLAYING )
		{
			TraceDebug{ClassId} << "Trying to pause a non-playing audio source !";

			return;
		}

		alSourcePause(this->identifier());
	}

	void
	Source::resume () noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		if ( this->getIntValue(AL_SOURCE_STATE, AL_INITIAL) != AL_PAUSED )
		{
			TraceDebug{ClassId} << "Trying to resume a non paused audio source !";

			return;
		}

		alSourcePlay(this->identifier());
	}

	void
	Source::rewind () noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		switch ( this->getIntValue(AL_SOURCE_STATE, AL_INITIAL) )
		{
			case AL_PLAYING :
			case AL_PAUSED :
			case AL_STOPPED :
				alSourceRewind(this->identifier());
				break;

			default:
				TraceDebug{ClassId} << "Trying to rewind an empty audio source !";
				break;
		}
	}

	void
	Source::stop () noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		switch ( this->getIntValue(AL_SOURCE_STATE, AL_INITIAL) )
		{
			case AL_PLAYING :
			case AL_PAUSED :
				alSourceStop(this->identifier());
				break;

			default:
				//Tracer::warning(ClassId, "Trying to stop a non-playing or empty audio source !");
				break;
		}
	}

	void
	Source::removeSound () noexcept
	{
		if ( m_currentPlayableInterface == nullptr )
		{
			return;
		}

		if ( m_currentPlayableInterface->streamable() > 0 )
		{
			this->clearStream();
		}

		m_currentPlayableInterface.reset();
	}

	void
	Source::clearStream () const noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		alFlushErrors();

		/* NOTE: Be sure looping state is off when removing buffers. */
		alSourcei(this->identifier(), AL_LOOPING, AL_FALSE);

		/* Gets the number of buffer currently queued in the source.
		 * NOTE: No the processed, all of them. */
		ALint bufferCount = 0;

		alGetSourcei(this->identifier(), AL_BUFFERS_QUEUED, &bufferCount);

		/* Removing buffers and store removed identifiers. */
		std::vector< ALuint > removedIdentifiers(static_cast< size_t >(bufferCount));

		alSourceUnqueueBuffers(this->identifier(), bufferCount, removedIdentifiers.data());

		if ( !alGetErrors(__PRETTY_FUNCTION__, __FILE__, __LINE__) )
		{
			Tracer::warning(ClassId, "Something goes wrong with OpenAL when clearing streams !");
		}
	}

	void
	Source::setFloatValue (ALenum pName, ALfloat value) noexcept
	{
		alSourcef(this->identifier(), pName, value);
	}

	ALfloat
	Source::getFloatValue (ALenum pName, ALfloat defaultValue) const noexcept
	{
		auto value = defaultValue;

		if ( Manager::instance()->usable() )
		{
			if ( this->isCreated() )
			{
				alGetSourcef(this->identifier(), pName, &value);
			}
			else
			{
				TraceWarning{ClassId} << "Trying to get data (pName: " << pName << ") from an unloaded audio source !";
			}
		}

		return value;
	}

	void
	Source::setIntValue (ALenum pName, ALint value) noexcept
	{
		alSourcei(this->identifier(), pName, value);
	}

	ALint
	Source::getIntValue (ALenum pName, ALint defaultValue) const noexcept
	{
		auto value = defaultValue;

		if ( Manager::instance()->usable() )
		{
			if ( this->isCreated() )
			{
				alGetSourcei(this->identifier(), pName, &value);
			}
			else
			{
				TraceWarning{ClassId} << "Trying to get data (pName: " << pName << ") from an unloaded audio source !";
			}
		}

		return value;
	}

	std::ostream &
	operator<< (std::ostream & out, const Source & obj)
	{
		return out <<
			"Audio source #" << obj.identifier() << "\n"
			"Position : " << obj.position() << "\n"
			"Direction : " << obj.direction() << "\n"
			"Velocity : " << obj.velocity() << "\n"
			"Gain (minimum/maximum) : " << obj.gain() << " (" << obj.minimumGain() << '/' << obj.maximumGain() << ')' << "\n"
			"Reference distance : " << obj.referenceDistance() << "\n"
			"Roll-off factor : " << obj.rolloffFactor() << "\n"
			"Maximum distance : " << obj.maxDistance() << "\n"
			"Air absorption (EFX) : " << obj.airAbsorption() << "\n"
			"Cone inner angle : " << obj.coneInnerAngle() << "\n"
			"Cone outer angle : " << obj.coneOuterAngle() << "\n"
			"Cone outer gain : " << obj.coneOuterGain() << "\n"
			"Cone gain facing away (EFX) : " << obj.coneGainFacingAway() << "\n"
			"Pitch : " << obj.pitch() << "\n"
			"Is muted : " << ( obj.isMuted() ? "true" : "false" ) << "\n"
			"Is relative to listener : " << ( obj.isRelative() ? "true" : "false" ) << "\n"
			"Is looping : " << ( obj.isLooping() ? "true" : "false" ) << '\n';
	}

	std::string
	to_string (const Source & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
