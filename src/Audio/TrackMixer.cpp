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
#include "OpenALExtensions.hpp"
#include "Resources/Manager.hpp"
#include "Manager.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace EmEn::Libs;

	const size_t TrackMixer::ClassUID{getClassUID(ClassId)};

	bool
	TrackMixer::enableSourceEvents () noexcept
	{
		if ( !OpenAL::isEventsAvailable() )
		{
			TraceWarning{ClassId} << "The OpenAL extension for source events is not available !";

			return false;
		}

		OpenAL::alEventCallbackSOFT(eventCallback, this);

		constexpr std::array< ALCenum, 1 > events{
			//AL_EVENT_TYPE_BUFFER_COMPLETED_SOFT,
			AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT,
			//AL_EVENT_TYPE_DISCONNECTED_SOFT
		};

		OpenAL::alEventControlSOFT(events.size(), events.data(), AL_TRUE);

		return true;
	}

	void
	TrackMixer::eventCallback (ALenum eventType, ALuint object, ALuint param, ALsizei /*length*/, const ALchar * message, void * userParam) noexcept
	{
		if ( eventType != AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT || param != AL_STOPPED )
		{
			return;
		}

		auto * trackMixer = static_cast< TrackMixer * >(userParam);

		const std::lock_guard< std::mutex > lock{trackMixer->m_stateAccess};

		const PlayingTrack currentTrack = trackMixer->m_playingTrack;
		const ALuint currentSourceId = currentTrack == PlayingTrack::TrackA
			? trackMixer->m_trackA->identifier()
			: trackMixer->m_trackB->identifier();

		if ( object != currentSourceId )
		{
			return;
		}

		TraceDebug{ClassId} << message;

		trackMixer->m_requestNextTrack = true;
		trackMixer->m_fadeCv.notify_one();
	}

	bool
	TrackMixer::onInitialize () noexcept
	{
		if ( this->enableSourceEvents() )
		{
			TraceSuccess{ClassId} << "Events for source are enabled !";

			m_playMode = PlayMode::Once;
		}

		/* Sets master volume. */
		this->setVolume(m_primaryServices.settings().get< float >(AudioMusicVolumeKey, DefaultAudioMusicVolume));

		/* NOTE: Allocating track sources (to volume 0) */
		m_trackA = std::make_unique< Source >();
		m_trackA->setRelativeState(true);
		m_trackA->setRolloffFactor(0.0F);
		m_trackA->setGain(0.0F);

		m_trackB = std::make_unique< Source >();
		m_trackB->setRelativeState(true);
		m_trackB->setRolloffFactor(0.0F);
		m_trackB->setGain(0.0F);

		this->registerToConsole();

		m_stopThread = false;
		m_eventThread = std::thread{&TrackMixer::eventLoop, this};

		m_serviceInitialized = true;

		return true;
	}

	bool
	TrackMixer::onTerminate () noexcept
	{
		if ( m_serviceInitialized )
		{
			m_serviceInitialized = false;

			m_stopThread = true;
			m_fadeCv.notify_one();

			if ( m_eventThread.joinable() )
			{
				m_eventThread.join();
			}

			m_trackA->stop();
			m_trackB->stop();
		}

		return true;
	}

	void
	TrackMixer::setVolume (float volume) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_stateAccess};

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
	TrackMixer::play () noexcept
	{
		/* NOTE: If OpenAL is playing, we don't do anything. */
		if ( this->isPlaying() )
		{
			return true;
		}

		if ( m_playlist.empty() )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		/* NOTE: Check if the current music index is correct. */
		if ( m_musicIndex >= m_playlist.size() )
		{
			m_musicIndex = 0;
		}

		/* NOTE: We play the music at the index. */
		return this->play(m_playlist[m_musicIndex]);
	}

	bool
	TrackMixer::play (const std::shared_ptr< MusicResource > & track) noexcept
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

		{
			const std::lock_guard< std::mutex > lock{m_stateAccess};

			m_userState = UserState::Playing;
		}

		/* Check if we need to wait the track to be loaded in memory. */
		if ( !this->checkTrackLoading(track) )
		{
			Tracer::debug(ClassId, "Waits for the track to be fully loaded into memory for playback ...");

			return true;
		}

		if ( m_crossFaderEnabled )
		{
			this->notify(MusicSwitching, (std::stringstream{} << "Fading to '" << track->title() << "' track from '" << track->artist() << "'.").str());
		}
		else
		{
			this->notify(MusicPlaying, (std::stringstream{} << "Now playing '" << track->title() << "' track from '" << track->artist() << "'.").str());
		}

		/* NOTE: Check which track was playing. */
		PlayingTrack trackToPlayNow;
		bool wasPlaying = true;

		{
			const std::lock_guard< std::mutex > lock{m_stateAccess};

			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					m_playingTrack = PlayingTrack::TrackA;

					wasPlaying = false;
					break;

				case PlayingTrack::TrackA:
					m_playingTrack = PlayingTrack::TrackB;

					if ( m_crossFaderEnabled )
					{
						m_isFading = true;
					}
					break;

				case PlayingTrack::TrackB:
					m_playingTrack = PlayingTrack::TrackA;

					if ( m_crossFaderEnabled )
					{
						m_isFading = true;
					}
					break;
			}

			trackToPlayNow = m_playingTrack;
		}
    
		bool success = false;
		const float initialGain = m_crossFaderEnabled && wasPlaying ? 0.0F : m_gain;

		if ( trackToPlayNow == PlayingTrack::TrackA )
		{
			m_trackA->setGain(initialGain);

			success = m_trackA->play(track, m_playMode);
		}
		else
		{
			m_trackB->setGain(initialGain);

			success = m_trackB->play(track, m_playMode);
		}

		if ( m_isFading && success )
		{
			m_fadeCv.notify_one();
		}

		return success;
	}

	bool
	TrackMixer::next () noexcept
	{
		/* NOTE: If the player is paused, we don't change anything. */
		if ( m_userState == UserState::Paused )
		{
			return false;
		}

		if ( m_playlist.empty() )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		m_musicIndex++;

		if ( m_musicIndex >= m_playlist.size() )
		{
			m_musicIndex = 0;
		}

		/* NOTE: If the player is stopped, we just change the track. */
		if ( m_userState == UserState::Stopped )
		{
			return true;
		}

		return this->play(m_playlist[m_musicIndex]);
	}

	bool
	TrackMixer::isPlaying () const noexcept
	{
		if ( this->usable() )
		{
			const std::lock_guard< std::mutex > lock{m_stateAccess};

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

		const std::lock_guard< std::mutex > lock{m_stateAccess};

		switch ( m_playingTrack )
		{
			case PlayingTrack::None:
				break;

			case PlayingTrack::TrackA :
				m_userState = UserState::Paused;

				m_trackA->pause();

				this->notify(MusicPaused);

				break;

			case PlayingTrack::TrackB :
				m_userState = UserState::Paused;

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

		const std::lock_guard< std::mutex > lock{m_stateAccess};

		switch ( m_playingTrack )
		{
			case PlayingTrack::None:
				break;

			case PlayingTrack::TrackA :
				m_userState = UserState::Playing;

				m_trackA->resume();

				this->notify(MusicResumed);

				break;

			case PlayingTrack::TrackB :
				m_userState = UserState::Playing;

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

		const std::lock_guard< std::mutex > lock{m_stateAccess};

		m_userState = UserState::Stopped;

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
	TrackMixer::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
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
						const auto loadedTrack = m_loadingTrack;

						m_loadingTrack.reset();

						this->play(loadedTrack);
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

		/* NOTE: Don't know what it is, goodbye! */
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

			this->play(soundtrack);

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
	TrackMixer::fadeIn (Source * track, float step) const noexcept
	{
		auto currentGain = track->gain();

		currentGain += step;

		if ( currentGain >= m_gain )
		{
			track->setGain(m_gain);

			return true;
		}

		track->setGain(currentGain);

		return false;
	}

	void
	TrackMixer::fadeOut (Source * track, float step) noexcept
	{
		auto currentGain = track->gain();

		currentGain -= step;

		if ( currentGain <= 0.0F )
		{
			track->stop();
			track->removeSound();
		}
		else
		{
			track->setGain(currentGain);
		}
	}

	void
	TrackMixer::eventLoop ()
	{
		using namespace std::chrono_literals;

		while ( !m_stopThread )
		{
			{
				std::unique_lock< std::mutex > lock {m_stateAccess};

				m_fadeCv.wait(lock, [&] {
					return m_isFading || m_requestNextTrack || m_stopThread;
				});
			}

			if ( m_stopThread )
			{
				return;
			}

			if ( m_requestNextTrack )
			{
				m_requestNextTrack = false;

				if ( m_userState != UserState::Stopped )
				{
					this->next();
				}
			}

			while ( m_isFading && !m_stopThread )
			{
				constexpr auto stepValue = 0.01F;

				switch ( m_playingTrack )
				{
					/* NOTE: We are playing on the track A, so we fade out the track B. */
					case PlayingTrack::TrackA :
						if ( !m_trackB->isMuted() )
						{
							TrackMixer::fadeOut(m_trackB.get(), stepValue);
						}

						if ( this->fadeIn(m_trackA.get(), stepValue) )
						{
							m_isFading = false;
						}
						break;

					case PlayingTrack::TrackB :
						if ( !m_trackA->isMuted() )
						{
							TrackMixer::fadeOut(m_trackA.get(), stepValue);
						}

						if ( this->fadeIn(m_trackB.get(), stepValue) )
						{
							m_isFading = false;
						}
						break;

					case PlayingTrack::None :
						m_isFading = false;
						break;
				}

				std::this_thread::sleep_for(16ms);
			}
		}
	}
}
