/*
 * src/Scenes/Component/SoundEmitter.cpp
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

#include "SoundEmitter.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>

/* Local inclusions. */
#include "Animations/AnimationInterface.hpp"
#include "Audio/Manager.hpp"
#include "Audio/SoundResource.hpp"
#include "Audio/Types.hpp"
#include "BlobTrait.hpp"
#include "CoreTypes.hpp"
#include "ObservableTrait.hpp"
#include "Physics/MovableTrait.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"
#include "Variant.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Animations;
	using namespace Audio;

	void
	SoundEmitter::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		if ( m_source != nullptr )
		{
			this->updateSource(*m_source, worldCoordinates);
		}

		for ( const auto & voice : m_oneShotVoices )
		{
			if ( voice != nullptr )
			{
				this->updateSource(*voice, worldCoordinates);
			}
		}
	}

	void
	SoundEmitter::processLogics (const Scene & /*scene*/) noexcept
	{
		/* A one-shot voice whose sound ended goes back to the pool. */
		for ( auto & voice : m_oneShotVoices )
		{
			if ( voice != nullptr && voice->isFinished() )
			{
				voice.reset();
			}
		}

		if ( m_source == nullptr )
		{
			return;
		}

		/* Autorelease unused sound source. */
		if ( !this->isFlagEnabled(KeepInactiveSourceAlive) && m_source->isStopped() )
		{
			m_source->removeSound();
			m_source.reset();
		}
	}

	bool
	SoundEmitter::onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & /*data*/) noexcept
	{
		if ( observable->is(SoundResource::getClassUID()) )
		{
			switch ( notificationCode )
			{
				case Resources::ResourceTrait::LoadFinished :
					this->playAttachedSound();
					break;

				case Resources::ResourceTrait::LoadFailed :
					this->releaseAttachedSound();
					break;

				default:
					if constexpr ( ObserverDebugEnabled )
					{
						TraceDebug{ClassId} << "Event #" << notificationCode << " from a sound resource ignored.";
					}
					break;
			}

			return false;
		}

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	bool
	SoundEmitter::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case EmittingState :
				if ( value.asBool() )
				{
					this->resume();
				}
				else
				{
					this->pause();
				}
				return true;

			case Gain :
				this->setGain(value.asFloat());
				return true;

			default:
				return false;
		}
	}

	void
	SoundEmitter::updateSource (Source & source, const CartesianFrame< float > & worldCoordinates) const noexcept
	{
		/* Copy absolute position in world space coordinates. */
		source.setPosition(worldCoordinates.position());

		/* Sets the direction of the source. */
		source.setDirection(worldCoordinates.backwardVector());

		if ( this->velocityDistortionEnabled() )
		{
			/* Copy current velocity to a sound source for deforming effect. */
			const auto * movable = this->parentEntity().getMovableTrait();

			if ( movable != nullptr )
			{
				source.setVelocity(movable->getWorldVelocity());
			}
		}
	}

	void
	SoundEmitter::attachSound (const std::shared_ptr< SoundResource > & sound, float gain, bool loop) noexcept
	{
		this->setFlag(Loop, loop);

		m_attachedSound = sound;
		m_gain = gain;
	}

	void
	SoundEmitter::play (const std::shared_ptr< SoundResource > & sound, float gain, bool loop, bool replaceSound) noexcept
	{
		if ( !replaceSound && m_source != nullptr && m_source->isPlaying() )
		{
			return;
		}

		this->attachSound(sound, gain, loop);

		/* NOTE: Delay the sound play if not loaded. */
		if ( m_attachedSound->isLoaded() )
		{
			this->playAttachedSound();
		}
		else
		{
			this->observe(m_attachedSound.get());
		}
	}

	void
	SoundEmitter::playOneShot (const std::shared_ptr< SoundResource > & sound, float gain, float pitch) noexcept
	{
		if ( sound == nullptr )
		{
			TraceError{ClassId} << "playOneShot(), no sound given to the emitter '" << this->name() << "' !";

			return;
		}

		if ( !this->acceptsFinite("playOneShot", gain, pitch) )
		{
			return;
		}

		/* NOTE: A one-shot is an event of now: played once its loading finished, it would sound out of place. */
		if ( !sound->isLoaded() )
		{
			TraceWarning{ClassId} << "playOneShot(), the sound '" << sound->name() << "' is not loaded yet (preload it): not played by the emitter '" << this->name() << "'.";

			return;
		}

		auto * voice = this->acquireOneShotVoice();

		if ( voice == nullptr )
		{
			TraceDebug{ClassId} << "playOneShot(), no free source in the audio pool: '" << sound->name() << "' not played.";

			return;
		}

		auto & source = **voice;

		this->updateSource(source, this->getWorldCoordinates());

		source.setGain(std::abs(gain));
		source.setPitch(pitch);
		static_cast< void >(source.play(sound, PlayMode::Once));
	}

	size_t
	SoundEmitter::activeOneShotCount () const noexcept
	{
		return static_cast< size_t >(std::ranges::count_if(m_oneShotVoices, [] (const auto & voice) {
			return voice != nullptr && !voice->isFinished();
		}));
	}

	SourceRequest *
	SoundEmitter::acquireOneShotVoice () noexcept
	{
		/* A held voice whose sound ended is reused as it is. */
		for ( auto & voice : m_oneShotVoices )
		{
			if ( voice != nullptr && voice->isFinished() )
			{
				return &voice;
			}
		}

		/* An empty slot asks the pool. */
		for ( auto & voice : m_oneShotVoices )
		{
			if ( voice == nullptr )
			{
				voice = this->engineContext().audioManager.requestSource();

				return voice != nullptr ? &voice : nullptr;
			}
		}

		/* Every voice is busy: the next one in turn is cut for the new sound (the cursor stays < MaxOneShotVoices). */
		auto & voice = *std::next(m_oneShotVoices.begin(), static_cast< std::ptrdiff_t >(m_nextOneShotVoice));

		m_nextOneShotVoice = (m_nextOneShotVoice + 1) % MaxOneShotVoices;

		return &voice;
	}

	void
	SoundEmitter::releaseOneShotVoices () noexcept
	{
		/* NOTE: The pool stops and resets a source given back (Manager::releaseSource()). */
		for ( auto & voice : m_oneShotVoices )
		{
			voice.reset();
		}

		m_nextOneShotVoice = 0;
	}

	void
	SoundEmitter::pause () const noexcept
	{
		if ( m_source != nullptr )
		{
			m_source->pause();
		}

		for ( const auto & voice : m_oneShotVoices )
		{
			if ( voice != nullptr )
			{
				voice->pause();
			}
		}
	}

	void
	SoundEmitter::resume () const noexcept
	{
		if ( m_source != nullptr )
		{
			m_source->resume();
		}

		for ( const auto & voice : m_oneShotVoices )
		{
			if ( voice != nullptr )
			{
				voice->resume();
			}
		}
	}

	void
	SoundEmitter::replay () noexcept
	{
		/* No previous sound ... */
		if ( m_attachedSound == nullptr )
		{
			return;
		}

		/* If a source is present and is playing, we rewind the sound. */
		if ( m_source != nullptr && m_source->isPlaying() )
		{
			m_source->rewind();

			return;
		}

		this->playAttachedSound();
	}

	void
	SoundEmitter::stop () noexcept
	{
		this->releaseOneShotVoices();

		if ( m_source != nullptr )
		{
			m_source->stop();

			if ( !this->isFlagEnabled(KeepInactiveSourceAlive) )
			{
				m_source->removeSound();
				m_source.reset();
			}
		}
	}

	void
	SoundEmitter::onSuspend () noexcept
	{
		/* A one-shot is an event of the moment: it is not resumed at the wakeup. */
		this->releaseOneShotVoices();

		/* Only suspend if currently playing. */
		if ( m_source == nullptr || !m_source->isPlaying() )
		{
			return;
		}

		/* Remember we were playing. */
		this->enableFlag(WasPlayingBeforeSuspend);

		/* Release the source back to the pool. */
		m_source->stop();
		m_source->removeSound();
		m_source.reset();
	}

	void
	SoundEmitter::onWakeup () noexcept
	{
		/* Only wakeup if we were playing before suspend. */
		if ( !this->isFlagEnabled(WasPlayingBeforeSuspend) )
		{
			return;
		}

		this->disableFlag(WasPlayingBeforeSuspend);

		/* Reacquire a source and restart playback. */
		if ( m_attachedSound != nullptr && m_attachedSound->isLoaded() )
		{
			this->playAttachedSound();
		}
	}

	void
	SoundEmitter::playAttachedSound () noexcept
	{
		if ( m_source == nullptr )
		{
			m_source = this->engineContext().audioManager.requestSource();
		}

		if ( m_source != nullptr )
		{
			/* Check if useful. */
			this->updateSource(*m_source, this->getWorldCoordinates());

			m_source->setGain(m_gain);
			m_source->setPitch(m_pitch);
			m_source->play(m_attachedSound, this->isFlagEnabled(Loop) ? PlayMode::Loop : PlayMode::Once);
		}
	}

	void
	SoundEmitter::clear () noexcept
	{
		this->stop();

		this->releaseAttachedSound();

		if ( m_source != nullptr )
		{
			m_source->removeSound();
			m_source.reset();
		}

		m_gain = 8.0F;
		m_pitch = 1.0F;
	}
}
