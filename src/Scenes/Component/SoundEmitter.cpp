/*
 * src/Scenes/Component/SoundEmitter.cpp
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

#include "SoundEmitter.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <any>
#include <memory>
#include <string>

/* Local inclusions. */
#include "Audio/Manager.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Animations;
	using namespace Audio;

	void
	SoundEmitter::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		if ( m_source != nullptr )
		{
			this->updateSource(worldCoordinates);
		}
	}

	void
	SoundEmitter::processLogics (const Scene & /*scene*/) noexcept
	{
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
	SoundEmitter::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
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
	SoundEmitter::updateSource (const CartesianFrame< float > & worldCoordinates) const noexcept
	{
		/* Copy absolute position in world space coordinates. */
		m_source->setPosition(worldCoordinates.position());

		/* Sets the direction of the source. */
		m_source->setDirection(worldCoordinates.backwardVector());

		if ( this->velocityDistortionEnabled() )
		{
			/* Copy current velocity to a sound source for deforming effect. */
			const auto * movable = this->parentEntity().getMovableTrait();

			if ( movable != nullptr )
			{
				m_source->setVelocity(movable->getWorldVelocity());
			}
		}
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
			this->updateSource(this->getWorldCoordinates());

			m_source->setGain(m_gain);
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
	}
}
