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

/* STL inclusions. */
#include <algorithm>
#include <random>
#include <numeric>

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
	using namespace Libs;

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

		OpenAL::alEventControlSOFT(static_cast< ALsizei >(events.size()), events.data(), AL_TRUE);

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
		this->setVolume(m_primaryServices.settings().getOrSetDefault< float >(AudioMusicVolumeKey, DefaultAudioMusicVolume));

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

		return true;
	}

	bool
	TrackMixer::onTerminate () noexcept
	{
		m_stopThread = true;
		m_fadeCv.notify_one();

		if ( m_eventThread.joinable() )
		{
			m_eventThread.join();
		}

		if ( m_trackA != nullptr )
		{
			m_trackA->stop();
		}

		if ( m_trackB != nullptr )
		{
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

	void
	TrackMixer::enableCrossFader (bool state) noexcept
	{
		m_crossFaderEnabled = state;

		/* NOTE: If disabling cross-fader while fading or with two tracks playing,
		 * stop the non-current track and set the current one to full volume. */
		if ( !state && this->usable() )
		{
			const std::lock_guard< std::mutex > lock{m_stateAccess};

			/* Stop any ongoing fade. */
			m_isFading = false;

			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
					/* Stop track B if it's playing. */
					if ( !m_trackB->isMuted() )
					{
						m_trackB->stop();
						m_trackB->removeSound();
					}
					/* Ensure track A is at full volume. */
					m_trackA->setGain(m_gain);
					break;

				case PlayingTrack::TrackB :
					/* Stop track A if it's playing. */
					if ( !m_trackA->isMuted() )
					{
						m_trackA->stop();
						m_trackA->removeSound();
					}
					/* Ensure track B is at full volume. */
					m_trackB->setGain(m_gain);
					break;

				case PlayingTrack::None:
					break;
			}
		}
	}

	void
	TrackMixer::setPlayMode (PlayMode mode) noexcept
	{
		m_playMode = mode;

		/* NOTE: Also update the currently playing source if any. */
		if ( this->usable() )
		{
			const std::lock_guard< std::mutex > lock{m_stateAccess};

			const bool looping = (mode == PlayMode::Loop);

			switch ( m_playingTrack )
			{
				case PlayingTrack::TrackA :
					m_trackA->setLooping(looping);
					break;

				case PlayingTrack::TrackB :
					m_trackB->setLooping(looping);
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
					else
					{
						/* NOTE: Stop the old track immediately when cross-fader is disabled. */
						m_trackA->stop();
						m_trackA->removeSound();
					}
					break;

				case PlayingTrack::TrackB:
					m_playingTrack = PlayingTrack::TrackA;

					if ( m_crossFaderEnabled )
					{
						m_isFading = true;
					}
					else
					{
						/* NOTE: Stop the old track immediately when cross-fader is disabled. */
						m_trackB->stop();
						m_trackB->removeSound();
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
	TrackMixer::playIndex (size_t index) noexcept
	{
		if ( m_playlist.empty() )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		if ( index >= m_playlist.size() )
		{
			Tracer::warning(ClassId, "Invalid playlist index !");

			return false;
		}

		m_musicIndex = index;

		return this->play(m_playlist[m_musicIndex]);
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

		/* NOTE: Handle shuffle mode. */
		if ( m_shuffleEnabled && !m_shuffleOrder.empty() )
		{
			m_shuffleIndex++;

			if ( m_shuffleIndex >= m_shuffleOrder.size() )
			{
				m_shuffleIndex = 0;
			}

			m_musicIndex = m_shuffleOrder[m_shuffleIndex];
		}
		else
		{
			m_musicIndex++;

			if ( m_musicIndex >= m_playlist.size() )
			{
				m_musicIndex = 0;
			}
		}

		/* NOTE: If the player is stopped, we just change the track index and notify. */
		if ( m_userState == UserState::Stopped )
		{
			this->notify(TrackChanged, m_musicIndex);

			return true;
		}

		const bool success = this->play(m_playlist[m_musicIndex]);

		/* NOTE: Notify that the track index has changed (for UI update). */
		if ( success )
		{
			this->notify(TrackChanged, m_musicIndex);
		}

		return success;
	}

	bool
	TrackMixer::previous () noexcept
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

		/* NOTE: Handle shuffle mode. */
		if ( m_shuffleEnabled && !m_shuffleOrder.empty() )
		{
			if ( m_shuffleIndex == 0 )
			{
				m_shuffleIndex = m_shuffleOrder.size() - 1;
			}
			else
			{
				m_shuffleIndex--;
			}

			m_musicIndex = m_shuffleOrder[m_shuffleIndex];
		}
		else
		{
			if ( m_musicIndex == 0 )
			{
				m_musicIndex = m_playlist.size() - 1;
			}
			else
			{
				m_musicIndex--;
			}
		}

		/* NOTE: If the player is stopped, we just change the track index and notify. */
		if ( m_userState == UserState::Stopped )
		{
			this->notify(TrackChanged, m_musicIndex);

			return true;
		}

		const bool success = this->play(m_playlist[m_musicIndex]);

		/* NOTE: Notify that the track index has changed (for UI update). */
		if ( success )
		{
			this->notify(TrackChanged, m_musicIndex);
		}

		return success;
	}

	float
	TrackMixer::currentPosition () const noexcept
	{
		if ( !this->usable() )
		{
			return 0.0F;
		}

		const std::lock_guard< std::mutex > lock{m_stateAccess};

		switch ( m_playingTrack )
		{
			case PlayingTrack::TrackA :
				return m_trackA->playbackPosition();

			case PlayingTrack::TrackB :
				return m_trackB->playbackPosition();

			case PlayingTrack::None :
			default :
				return 0.0F;
		}
	}

	float
	TrackMixer::currentDuration () const noexcept
	{
		if ( m_playlist.empty() || m_musicIndex >= m_playlist.size() )
		{
			return 0.0F;
		}

		return m_playlist[m_musicIndex]->duration();
	}

	void
	TrackMixer::seek (float position) noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_stateAccess};

		switch ( m_playingTrack )
		{
			case PlayingTrack::TrackA :
				m_trackA->setPlaybackPosition(position);
				break;

			case PlayingTrack::TrackB :
				m_trackB->setPlaybackPosition(position);
				break;

			case PlayingTrack::None :
				break;
		}
	}

	void
	TrackMixer::enableShuffle (bool state) noexcept
	{
		m_shuffleEnabled = state;

		if ( state && !m_playlist.empty() )
		{
			this->generateShuffleOrder();

			/* Find current track in shuffle order to continue from there. */
			for ( size_t i = 0; i < m_shuffleOrder.size(); i++ )
			{
				if ( m_shuffleOrder[i] == m_musicIndex )
				{
					m_shuffleIndex = i;
					break;
				}
			}
		}
	}

	void
	TrackMixer::generateShuffleOrder () noexcept
	{
		m_shuffleOrder.resize(m_playlist.size());
		std::iota(m_shuffleOrder.begin(), m_shuffleOrder.end(), 0);

		std::random_device rd;
		std::mt19937 gen{rd()};
		std::shuffle(m_shuffleOrder.begin(), m_shuffleOrder.end(), gen);

		m_shuffleIndex = 0;
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
		if ( observable->is(MusicResource::getClassUID()) )
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
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
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
