/*
 * src/Audio/TrackMixer.cpp
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

#include "TrackMixer.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* Local inclusions. */
#include "Manager.hpp"
#include "Resources/Manager.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;

	const size_t TrackMixer::ClassUID{getClassUID(ClassId)};

	bool
	TrackMixer::onInitialize () noexcept
	{
		/* Sets master volume. */
		const auto gain = m_primaryServices.settings().get< float >(AudioMusicVolumeKey, DefaultAudioMusicVolume);

		this->setVolume(gain);

		/* Allocating track sources (to volume 0) */
		for ( const auto & trackPtr : std::array{&m_trackA, &m_trackB} )
		{
			auto & track = (*trackPtr);

			track = m_audioManager.requestSource();

			if ( track == nullptr )
			{
				Tracer::error(ClassId, "There is no more source available to create the mixer sound tracks !");

				return false;
			}

			track->setRelativeState(true);
			track->setRolloffFactor(0.0F);
			track->setGain(0.0F);
		}

		this->registerToConsole();

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	TrackMixer::onTerminate () noexcept
	{
		if ( this->usable() )
		{
			m_flags[ServiceInitialized] = false;

			m_trackA->stop();
			m_trackB->stop();
		}

		return true;
	}

	void
	TrackMixer::setVolume (float volume) noexcept
	{
		m_gain = Math::clampToUnit(volume);

		if ( this->usable() )
		{
			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
					m_trackA->setGain(m_gain);
					break;

				case PlayingTrack::TrackB :
					m_trackB->setGain(m_gain);
					break;

				case PlayingTrack::None:
					break;
			}
		}
	}

	bool
	TrackMixer::checkTrackLoading (const std::shared_ptr< MusicResource > & track) noexcept
	{
		if ( track->isLoaded() )
		{
			return true;
		}

		m_loadingTrack = track;

		this->observe(track.get());

		return false;
	}

	bool
	TrackMixer::setSoundTrack (const std::shared_ptr< MusicResource > & track, bool fade) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The track mixer is unavailable !");

			return false;
		}

		if ( track == nullptr )
		{
			Tracer::error(ClassId, "The track is a null pointer !");

			return false;
		}

		m_flags[IsFadingWasDemanded] = fade;

		/* Check if we need to wait the track to be loaded in memory. */
		if ( !this->checkTrackLoading(track) )
		{
			Tracer::info(ClassId, "Waits for the track to be fully loaded into memory for playback ...");

			return true;
		}

		if ( m_flags[IsFadingWasDemanded] )
		{
			this->notify(MusicSwitching, (std::stringstream{} << "Fading to '" << track->title() << "' track from '" << track->artist() << "'.").str());

			/* Which track is playing. */
			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
					m_flags[IsTrackingFading] = true;

					m_trackB->play(track, Source::PlayMode::Loop);

					m_playingTrack = PlayingTrack::TrackB;
					break;

				case PlayingTrack::None:
				case PlayingTrack::TrackB :
					m_flags[IsTrackingFading] = true;

					m_trackA->play(track, Source::PlayMode::Loop);

					m_playingTrack = PlayingTrack::TrackA;
					break;
			}
		}
		else
		{
			this->notify(MusicPlaying, (std::stringstream{} << "Now playing '" << track->title() << "' track from '" << track->artist() << "'.").str());

			/* Which track is playing. */
			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					m_playingTrack = PlayingTrack::TrackA;

					[[fallthrough]];

				case PlayingTrack::TrackA :
					m_flags[IsTrackingFading] = false;

					m_trackA->setGain(m_gain);
					m_trackA->play(track, Source::PlayMode::Loop);
					break;

				case PlayingTrack::TrackB :
					m_flags[IsTrackingFading] = false;

					m_trackB->setGain(m_gain);
					m_trackB->play(track, Source::PlayMode::Loop);
					break;
			}
		}

		return true;
	}

	bool
	TrackMixer::isPlaying () const noexcept
	{
		if ( this->usable() )
		{
			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
				case PlayingTrack::TrackB :
					return true;

				default :
					break;
			}
		}

		return false;
	}

	void
	TrackMixer::pause () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		switch ( m_playingTrack )
		{
			case PlayingTrack::None:
				break;

			case PlayingTrack::TrackA :
				m_trackA->pause();

				this->notify(MusicPaused);
				break;

			case PlayingTrack::TrackB :
				m_trackB->pause();

				this->notify(MusicPaused);
				break;
		}
	}

	void
	TrackMixer::resume () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		switch ( m_playingTrack )
		{
			case PlayingTrack::None:
				break;

			case PlayingTrack::TrackA :
				m_trackA->resume();

				this->notify(MusicResumed);
				break;

			case PlayingTrack::TrackB :
				m_trackB->resume();

				this->notify(MusicResumed);
				break;
		}
	}

	void
	TrackMixer::stop () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		switch ( m_playingTrack )
		{
			case PlayingTrack::None:
				break;

			case PlayingTrack::TrackA :
				m_trackA->stop();
				m_playingTrack = PlayingTrack::None;

				this->notify(MusicStopped);
				break;

			case PlayingTrack::TrackB :
				m_trackB->stop();
				m_playingTrack = PlayingTrack::None;

				this->notify(MusicStopped);
				break;
		}
	}

	bool
	TrackMixer::onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(MusicResource::ClassUID) )
		{
			if ( m_loadingTrack != nullptr )
			{
				switch ( notificationCode )
				{
					/* The track loaded successfully, we can now play it. */
					case Resources::ResourceTrait::LoadFinished :
					{
						auto loadedTrack = m_loadingTrack;

						m_loadingTrack.reset();

						this->setSoundTrack(loadedTrack, m_flags[IsFadingWasDemanded]);
					}
						break;

					case Resources::ResourceTrait::LoadFailed :
						Tracer::warning(ClassId, "The track has failed to load ! Cancelling the playback...");
						break;

					default:
						if constexpr ( ObserverDebugEnabled )
						{
							TraceDebug{ClassId} << "Event #" << notificationCode << " from a music resource ignored.";
						}
						break;
				}
			}
			else
			{
				Tracer::info(ClassId, "No music track was waited here !");
			}

			/* NOTE: We don't keep any observable here. */
			return false;
		}

		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	void
	TrackMixer::onRegisterToConsole () noexcept
	{
		this->bindCommand("play", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return 0;
			}

			/* Checks if we need to resume. */
			if ( arguments.empty() )
			{
				switch ( m_playingTrack )
				{
					case PlayingTrack::None :
						outputs.emplace_back(Severity::Warning, "There is no soundtrack !");
						break;

					case PlayingTrack::TrackA :
						if ( m_trackA->isPaused() )
						{
							m_trackA->resume();

							outputs.emplace_back(Severity::Info, "Resuming track A.");

							return 0;
						}
						break;

					case PlayingTrack::TrackB :
						if ( m_trackB->isPaused() )
						{
							m_trackB->resume();

							outputs.emplace_back(Severity::Info, "Resuming track B.");;

							return 0;
						}
						break;
				}

				return 1;
			}

			/* Search the song. */
			const auto soundTrackName = arguments[0].asString();
			const auto soundtrack = m_resourceManager.container< MusicResource >()->getResource(soundTrackName);

			if ( soundtrack == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Soundtrack '" << soundTrackName << "' doesn't exist !");

				return 2;
			}

			this->setSoundTrack(soundtrack);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Playing '" << soundTrackName << "' ...");

			return 0;
		}, "Play or resume a music. There is no need of parameter to resume.");

		this->bindCommand("pause", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return 1;
			}

			switch ( m_playingTrack )
			{
				case PlayingTrack::None :
					outputs.emplace_back(Severity::Warning, "There is no track playing !");
					break;

				case PlayingTrack::TrackA :
					m_trackA->pause();

					outputs.emplace_back(Severity::Info, "Track A paused.");
					break;

				case PlayingTrack::TrackB :
					m_trackB->pause();

					outputs.emplace_back(Severity::Info, "Track B paused.");
					break;
			}

			return 0;
		}, "Pause music playback.");

		this->bindCommand("stop", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( !this->usable() )
			{
				outputs.emplace_back(Severity::Warning, "The track mixer is unavailable !");

				return 1;
			}

			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					outputs.emplace_back(Severity::Warning, "There is no track playing !");
					break;

				case PlayingTrack::TrackA :
					m_trackA->stop();
					m_playingTrack = PlayingTrack::None;

					outputs.emplace_back(Severity::Info, "Track A stopped.");
					break;

				case PlayingTrack::TrackB :
					m_trackB->stop();
					m_playingTrack = PlayingTrack::None;

					outputs.emplace_back(Severity::Info, "Track B stopped.");
					break;
			}

			return 0;
		}, "Stop music.");
	}

	bool
	TrackMixer::fadeTrack (Source & track, float step) const noexcept
	{
		auto bound = false;

		auto currentVolume = track.gain();

		/* Stepping */
		currentVolume += step;

		if ( currentVolume > m_gain )
		{
			currentVolume = m_gain;
			bound = true;
		}
		else if ( currentVolume < 0.0F )
		{
			currentVolume = 0.0F;
			bound = true;
		}

		track.setGain(currentVolume);

		return bound;
	}

	void
	TrackMixer::update () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		if ( m_flags[IsTrackingFading] )
		{
			constexpr auto stepValue = 0.01F;

			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
					/* Decrease TrackB volume. */
					if ( !m_trackB->isMuted() )
					{
						this->fadeTrack(*m_trackB, -stepValue);
					}

					/* Increase TrackA volume. */
					if ( this->fadeTrack(*m_trackA, stepValue) )
					{
						m_trackB->setGain(0.0F);

						m_flags[IsTrackingFading] = false;
					}
					break;

				case PlayingTrack::TrackB :
					/* Decrease TrackA volume. */
					if ( !m_trackA->isMuted() )
					{
						this->fadeTrack(*m_trackA, -stepValue);
					}

					/* Increase TrackB volume. */
					if ( this->fadeTrack(*m_trackB, stepValue) )
					{
						m_trackA->setGain(0.0F);

						m_flags[IsTrackingFading] = false;
					}
					break;

				case PlayingTrack::None :
					m_flags[IsTrackingFading] = false;
					break;
			}
		}
	}
}
