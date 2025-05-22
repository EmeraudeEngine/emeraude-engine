/*
 * src/Audio/Ambience.cpp
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

#include "Ambience.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Resources/Manager.hpp"
#include "Audio/Manager.hpp"
#include "Constants.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	Ambience::~Ambience ()
	{
		this->reset();
	}

	bool
	Ambience::allocateSources () noexcept
	{
		if ( this->requestLoopChannel() )
		{
			m_loopedSource = Manager::instance()->requestSource();

			if ( m_loopedSource == nullptr )
			{
				return false;
			}

			m_loopedSource->setRelativeState(true);
			m_loopedSource->setGain(m_loopedChannelGain);

			if ( m_directFilter != nullptr )
			{
				m_loopedSource->enableDirectFilter(m_directFilter);
			}
		}

		if ( this->requestSoundEffectChannels() )
		{
			if ( m_channelCount == 0 )
			{
				Tracer::warning(ClassId, "Channel count parameter must a least be 1 !");

				return false;
			}

			if ( m_channelCount != m_channels.size() )
			{
				m_channels.clear();

				const auto * audioManager = Manager::instance();

				for ( size_t channelIndex = 0; channelIndex < m_channelCount; channelIndex++ )
				{
					auto source = audioManager->requestSource();

					if ( source == nullptr )
					{
						return !m_channels.empty();
					}

					source->setRelativeState(false);
					source->setGain(1.0F);
					source->setReferenceDistance(m_radius * Half< float >);
					source->setMaxDistance(m_radius * Double< float >);

					if ( m_directFilter != nullptr )
					{
						source->enableDirectFilter(m_directFilter);
					}

					m_channels.emplace_back(source);
				}
			}
		}

		return true;
	}

	void
	Ambience::releaseSources () noexcept
	{
		if ( m_loopedSource != nullptr )
		{
			m_loopedSource->stop();
			m_loopedSource->removeSound();

			m_loopedSource.reset();
		}

		if ( !m_channels.empty() )
		{
			for ( const auto & channel : m_channels )
			{
				channel.getSource()->stop();
				channel.getSource()->removeSound();
			}

			m_channels.clear();
		}
	}

	void
	Ambience::setRadius (float radius) noexcept
	{
		m_radius = std::max(1.0F, std::abs(radius));

		for ( const auto & channel : m_channels )
		{
			auto source = channel.getSource();

			source->setReferenceDistance(m_radius * Half< float >);
			source->setMaxDistance(m_radius * Double< float >);
		}
	}

	bool
	Ambience::setLoopSound (const std::shared_ptr< SoundResource > & soundResource, float gain) noexcept
	{
		if ( soundResource == nullptr )
		{
			Tracer::error(ClassId, "The sound resource is null !");

			return false;
		}

		m_loopSound = soundResource;
		m_loopedChannelGain = gain;

		return true;
	}

	bool
	Ambience::addSound (const AmbienceSound & sound) noexcept
	{
		if ( sound.soundResource() == nullptr )
		{
			Tracer::error(ClassId, "The sound resource is null !");

			return false;
		}

		m_soundEffects.emplace_back(sound);

		return true;
	}

	bool
	Ambience::addSound (const std::shared_ptr< SoundResource > & sound, float channelGain, bool relative, float minPitch, float maxPitch, float velocity) noexcept
	{
		if ( sound == nullptr )
		{
			Tracer::error(ClassId, "The sound resource is null !");

			return false;
		}

		auto & soundEffect = m_soundEffects.emplace_back(sound);
		soundEffect.setChannelGain(channelGain);
		soundEffect.setRelativeState(relative);
		soundEffect.setRandomPitchRange(minPitch, maxPitch);
		soundEffect.setRadialVelocity(velocity);

		return true;
	}

	bool
	Ambience::enableDirectFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept
	{
		m_directFilter = filter;

		if ( m_loopedSource != nullptr )
		{
			if ( !m_loopedSource->enableDirectFilter(filter) )
			{
				return false;
			}
		}

		return !std::ranges::any_of(std::as_const(m_channels), [filter] (auto & channel) {
			return static_cast< bool >(!channel.getSource()->enableDirectFilter(filter));
		});
	}

	void
	Ambience::disableDirectFilter () noexcept
	{
		if ( m_loopedSource != nullptr )
		{
			m_loopedSource->disableDirectFilter();
		}

		for ( const auto & channel : m_channels )
		{
			channel.getSource()->disableDirectFilter();
		}

		m_directFilter.reset();
	}

	void
	Ambience::start () noexcept
	{
		if ( m_active )
		{
			return;
		}

		/* NOTE: If no sound effects or no loop, cancel. */
		if ( !this->requestLoopChannel() && !this->requestSoundEffectChannels() )
		{
			Tracer::warning(ClassId, "No sound effect in ambience ! Cancelling ...");

			return;
		}

		if ( !this->allocateSources() )
		{
			Tracer::error(ClassId, "Unable to allocate sources !");

			this->releaseSources();

			return;
		}

		/* Launch the loop channel. */
		if ( this->requestLoopChannel() )
		{
			/* NOTE: Delay the sound play if not loaded. */
			if ( m_loopSound->isLoaded() )
			{
				m_loopedSource->play(m_loopSound, Source::PlayMode::Loop);
			}
			else
			{
				this->observe(m_loopSound.get());
			}
		}

		/* Launch the sound effect channels. */
		if ( this->requestSoundEffectChannels() )
		{
			for ( auto & channel : m_channels )
			{
				channel.setTimeBeforeNextPlay(this->getRandomDelay());
			}
		}

		m_active = true;
	}

	bool
	Ambience::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(SoundResource::ClassUID) )
		{
			switch ( notificationCode )
			{
				case Resources::ResourceTrait::LoadFinished :
					m_loopedSource->play(m_loopSound, Source::PlayMode::Loop);
					break;

				case Resources::ResourceTrait::LoadFailed :
					m_loopSound.reset();
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

		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << ObservableTrait::whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	void
	Ambience::stop () noexcept
	{
		if ( !m_active )
		{
			return;
		}

		m_active = false;

		this->releaseSources();
	}

	void
	Ambience::update () noexcept
	{
		if ( !m_active )
		{
			return;
		}

		for ( auto & channel : m_channels )
		{
			/* NOTE : Update the time. */
			if ( !channel.isTimeToPlay() )
			{
				channel.update(EngineUpdateCycleDurationMS< uint32_t >);

				continue;
			}

			/* Time elapsed, we play the sound. */
			const auto soundDuration = channel.play(this->getRandomSound(), m_radius);

			/* Preparing the next new time when the channel will play a sound. */
			channel.setTimeBeforeNextPlay(this->getRandomDelay() + soundDuration);
		}
	}

	bool
	Ambience::loadSoundSet (const std::filesystem::path & filepath) noexcept
	{
		Json::Value root;

		if ( !FastJSON::getRootFromFile(filepath, root) )
		{
			TraceError{ClassId} << "Unable to read file " << filepath << " !";

			return false;
		}

		auto * soundManager = Resources::Manager::instance()->container< SoundResource >();

		/* 1. Read base sound set information. */
		this->setChannelCount(FastJSON::getNumber< size_t >(root, JKChannelCount, DefaultChannelCount));
		this->setRadius(FastJSON::getNumber< float >(root, JKRadius, DefaultRadius));

		/* 2. Read the loop sound effect. */
		if ( root.isMember(JKLoopSoundEffect) )
		{
			const auto & loopSFX = root[JKLoopSoundEffect];

			if ( loopSFX.isObject() )
			{
				const auto resourceName = FastJSON::getString(loopSFX, JKResourceName);

				if ( !resourceName.empty() )
				{
					this->setLoopSound(soundManager->getResource(resourceName), FastJSON::getNumber< float >(loopSFX, JKGain, DefaultGain));
				}
				else
				{
					Tracer::error(ClassId, "The loop sound effect resource name is empty or unspecified !");
				}
			}
			else
			{
				TraceError{ClassId} << "The '" << JKLoopSoundEffect << "' section in JSON file is ill-formed !";
			}
		}

		/* 3. Read the sound effect collection. */
		if ( root.isMember(JKSoundEffectCollection) )
		{
			const auto & SFXCollection = root[JKSoundEffectCollection];

			if ( SFXCollection.isArray() )
			{
				for ( const auto & SFX : SFXCollection )
				{
					if ( !SFX.isObject() )
					{
						Tracer::error(ClassId, "A sound effect section in JSON file is ill-formed !");

						continue;
					}

					const auto resourceName = FastJSON::getString(SFX, JKResourceName);

					if ( resourceName.empty() )
					{
						Tracer::error(ClassId, "A sound effect resource name is empty or unspecified !");

						continue;
					}

					const auto gain = FastJSON::getNumber< float >(SFX, JKGain, DefaultGain);
					const auto relative = FastJSON::getBoolean(SFX, JKRelative, true);
					const auto minPitch = FastJSON::getNumber< float >(SFX, JKMinimumPitch, 1.0F);
					const auto maxPitch = FastJSON::getNumber< float >(SFX, JKMaximumPitch, 1.0F);
					const auto velocity = FastJSON::getNumber< float >(SFX, JKRadialVelocity, 0.0F);

					if ( !this->addSound(soundManager->getResource(resourceName), gain, relative, minPitch, maxPitch, velocity) )
					{
						break;
					}
				}
			}
			else
			{
				TraceError{ClassId} << "The '" << JKSoundEffectCollection << "' section in JSON file is ill-formed !";
			}
		}

		return true;
	}

	void
	Ambience::reset () noexcept
	{
		/* Release channels and filters. */
		this->releaseSources();
		m_directFilter.reset();

		/* Release all sound resources. */
		m_loopSound.reset();
		m_soundEffects.clear();

		/* Reset parameters to default. */
		m_loopedChannelGain = DefaultGain;
		m_channelCount = DefaultChannelCount;
		m_radius = DefaultRadius;
		m_minDelay = DefaultMinDelay;
		m_maxDelay = DefaultMaxDelay;
		m_active = false;
	}
}
