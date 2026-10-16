/*
 * src/Audio/TrackMixer.cpp
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

#include "TrackMixer.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <set>
#include <sstream>

/* Local inclusions. */
#include "String.hpp"
#include "Manager.hpp"
#include "OpenALExtensions.hpp"
#include "PortableRandom.hpp"
#include "Resources/Manager.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace Base;

	namespace
	{
		/* The OpenAL Soft events the mixer listens to. */
		constexpr std::array< ALCenum, 1 > SourceEvents{
			AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT
		};
	}

	bool
	TrackMixer::enableSourceEvents () noexcept
	{
		if ( !OpenAL::isEventsAvailable() )
		{
			TraceWarning{ClassId} << "The OpenAL extension for source events is not available !";

			return false;
		}

		OpenAL::alEventCallbackSOFT(eventCallback, this);
		OpenAL::alEventControlSOFT(static_cast< ALsizei >(SourceEvents.size()), SourceEvents.data(), AL_TRUE);

		m_sourceEventsEnabled = true;

		return true;
	}

	void
	TrackMixer::disableSourceEvents () noexcept
	{
		if ( !m_sourceEventsEnabled )
		{
			return;
		}

		/* NOTE: The registration held a raw this: it must not outlive the mixer (Ave Robustus II, a registration is
		 * released by its owner). It was never removed before 2026-10-12. */
		OpenAL::alEventControlSOFT(static_cast< ALsizei >(SourceEvents.size()), SourceEvents.data(), AL_FALSE);
		OpenAL::alEventCallbackSOFT(nullptr, nullptr);

		m_sourceEventsEnabled = false;
	}

	void
	TrackMixer::eventCallback (ALenum eventType, ALuint object, ALuint param, ALsizei /*length*/, const ALchar * message, void * userParam) noexcept
	{
		if ( eventType != AL_EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT || param != AL_STOPPED )
		{
			return;
		}

		auto * trackMixer = static_cast< TrackMixer * >(userParam);

		{
			const MutexLock lock{trackMixer->m_stateAccess};

			/* NOTE: A source stopped while no track plays (stop(), the old source of a switch) is not the end of the
			 * current track. The playing source used to default to track B here. */
			if ( trackMixer->m_playingTrack == PlayingTrack::None )
			{
				return;
			}

			const ALuint currentSourceId = trackMixer->m_playingTrack == PlayingTrack::TrackA ?
				trackMixer->m_trackA->identifier() :
				trackMixer->m_trackB->identifier();

			if ( object != currentSourceId )
			{
				return;
			}

			trackMixer->m_requestNextTrack = true;
		}

		trackMixer->m_fadeCv.notify_one();

		TraceDebug{ClassId} << message;
	}

	bool
	TrackMixer::onInitialize () noexcept
	{
		{
			const MutexLock lock{m_stateAccess};

			/* NOTE: The sources exist BEFORE the OpenAL events are registered: the callback reads them. */
			m_trackA = std::make_unique< Source >();
			m_trackA->setRelativeState(true);
			m_trackA->setRolloffFactor(0.0F);
			m_trackA->setGain(0.0F);

			m_trackB = std::make_unique< Source >();
			m_trackB->setRelativeState(true);
			m_trackB->setRolloffFactor(0.0F);
			m_trackB->setGain(0.0F);
		}

		if ( this->enableSourceEvents() )
		{
			TraceSuccess{ClassId} << "Events for source are enabled !";

			const MutexLock lock{m_stateAccess};

			m_playMode = PlayMode::Once;
		}

		/* Sets master volume. */
		this->setVolume(m_primaryServices.settings().getOrSetDefault< float >(AudioMusicVolumeKey, DefaultAudioMusicVolume));

		/* NOTE: SoundFont for MIDI rendering is now managed per-resource via the dependency system.
		 * Each MusicResource queries settings and loads the soundfont as a dependency when needed. */

		/* NOTE: Console registration is handled by AudioManager via registerToObject(). */

		if ( !m_eventThread.start([this] (const std::stop_token & stopToken) { this->eventLoop(stopToken); }) )
		{
			/* The system refused the thread (Base::Thread traced why): the track mixer is refused (owner policy,
			 * 2026-10-07). onTerminate() releases what was set up above. */
			Tracer::error(ClassId, "Unable to start the track mixer event thread: the track mixer is disabled !");

			static_cast< void >(this->onTerminate());

			return false;
		}

		return true;
	}

	TrackMixer::~TrackMixer ()
	{
		this->disableSourceEvents();
		this->stopEventThread();
	}

	void
	TrackMixer::stopEventThread () noexcept
	{
		/* NOTE: The thread's stop token wakes its wait (std::condition_variable_any with a stop token): no flag, no
		 * notification of our own (Ave Robustus II, the std::jthread model). */
		m_eventThread.requestStop();

		if ( m_eventThread.joinable() )
		{
			m_eventThread.join();
		}
	}

	bool
	TrackMixer::onTerminate () noexcept
	{
		/* NOTE: The OpenAL callback first (it wakes the event thread), then the event thread, then the sources. */
		this->disableSourceEvents();
		this->stopEventThread();

		const MutexLock lock{m_stateAccess};

		if ( m_trackA != nullptr )
		{
			m_trackA->stop();
		}

		if ( m_trackB != nullptr )
		{
			m_trackB->stop();
		}

		m_playingTrack = PlayingTrack::None;
		m_isFading = false;
		m_loadingTrack.reset();

		/* NOTE: The sources live on the audio manager's current context (the recorder's game context when it is up):
		 * released here, while it still exists (Ave Robustus II). onInitialize() creates them again. */
		m_trackA.reset();
		m_trackB.reset();

		return true;
	}

	void
	TrackMixer::setVolume (float volume) noexcept
	{
		const MutexLock lock{m_stateAccess};

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
		const MutexLock lock{m_stateAccess};

		m_crossFaderEnabled = state;

		/* NOTE: If disabling cross-fader while fading or with two tracks playing,
		 * stop the non-current track and set the current one to full volume. */
		if ( state || !this->usable() )
		{
			return;
		}

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

	void
	TrackMixer::setPlayMode (PlayMode mode) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_playMode = mode;

		/* NOTE: Also update the currently playing source if any. */
		if ( !this->usable() )
		{
			return;
		}

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

	void
	TrackMixer::addToPlaylist (const std::shared_ptr< MusicResource > & track) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_playlist.push_back(track);
		m_loadedPlaylist.reset();

		this->playlistChanged();
	}

	void
	TrackMixer::clearPlaylist () noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_playlist.clear();
		m_loadedPlaylist.reset();

		this->playlistChanged();
	}

	TrackMixer::Status
	TrackMixer::status () const noexcept
	{
		const MutexLock lock{m_stateAccess};

		Status status;
		status.playlist = m_playlist;
		status.currentTrack = m_musicIndex < m_playlist.size() ? m_playlist[m_musicIndex] : nullptr;
		status.loadedPlaylist = m_loadedPlaylist;
		status.trackIndex = m_musicIndex;
		status.gain = m_gain;
		status.userState = m_userState;
		status.playMode = m_playMode;
		status.hasTrack = m_playingTrack != PlayingTrack::None;
		status.crossFaderEnabled = m_crossFaderEnabled;
		status.shuffleEnabled = m_shuffleEnabled;

		return status;
	}

	bool
	TrackMixer::play () noexcept
	{
		/* NOTE: If OpenAL is playing, we don't do anything. */
		if ( this->isPlaying() )
		{
			return true;
		}

		std::shared_ptr< MusicResource > track;

		{
			const MutexLock lock{m_stateAccess};

			if ( !m_playlist.empty() )
			{
				/* NOTE: Check if the current music index is correct. */
				if ( m_musicIndex >= m_playlist.size() )
				{
					m_musicIndex = 0;
				}

				track = m_playlist[m_musicIndex];
			}
		}

		if ( track == nullptr )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		/* NOTE: We play the music at the index. */
		return this->play(track);
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

		bool waitForLoading = false;
		bool success = false;
		bool switching = false;

		{
			const MutexLock lock{m_stateAccess};

			m_userState = UserState::Playing;

			/* Sync m_musicIndex with the track's position in the playlist (if any).
			 * Without this, direct play(track) calls (e.g. from the console) leave
			 * m_musicIndex stale, so nowPlaying/status/currentPosition/currentDuration
			 * all report the previous track. SIZE_MAX signals "out of playlist". */
			const auto entry = std::ranges::find(m_playlist, track);

			m_musicIndex = entry != m_playlist.end() ? static_cast< size_t >(entry - m_playlist.begin()) : std::numeric_limits< size_t >::max();

			if ( track->isLoaded() )
			{
				/* NOTE: A newer request supersedes a track still loading. */
				m_loadingTrack.reset();

				switching = m_crossFaderEnabled;
				success = this->startTrack(track);
			}
			else
			{
				m_loadingTrack = track;
				waitForLoading = true;
			}
		}

		/* Check if we need to wait the track to be loaded in memory. */
		if ( waitForLoading )
		{
			Tracer::debug(ClassId, "Waits for the track to be fully loaded into memory for playback ...");

			this->observe(track.get());

			/* NOTE: The track may have finished loading between isLoaded() and observe(): its notification is then
			 * gone. playPendingTrack() claims the pending track under the lock, so the notification and this re-check
			 * never both start it. */
			if ( track->isLoaded() )
			{
				this->playPendingTrack(track.get());
			}

			return true;
		}

		if ( success )
		{
			this->notifyTrackStarted(*track, switching);
		}

		return success;
	}

	void
	TrackMixer::notifyTrackStarted (const MusicResource & track, bool switching) noexcept
	{
		/* NOTE: Notified after the lock: an observer may call the mixer back. */
		if ( switching )
		{
			this->notify(MusicSwitching, (std::stringstream{} << "Fading to '" << track.title() << "' track from '" << track.artist() << "'.").str());
		}
		else
		{
			this->notify(MusicPlaying, (std::stringstream{} << "Now playing '" << track.title() << "' track from '" << track.artist() << "'.").str());
		}
	}

	bool
	TrackMixer::startTrack (const std::shared_ptr< MusicResource > & track) noexcept
	{
		/* NOTE: Check which track was playing. */
		const auto previousTrack = m_playingTrack;
		bool wasPlaying = true;

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

		const float initialGain = m_crossFaderEnabled && wasPlaying ? 0.0F : m_gain;
		auto & source = m_playingTrack == PlayingTrack::TrackA ? *m_trackA : *m_trackB;

		source.setGain(initialGain);

		if ( !source.play(track, m_playMode) )
		{
			/* NOTE: A failed start leaves the state as it really is: the old track still plays when it was fading out,
			 * nothing plays when it was stopped above. */
			m_isFading = false;
			m_playingTrack = m_crossFaderEnabled ? previousTrack : PlayingTrack::None;

			return false;
		}

		if ( m_isFading )
		{
			m_fadeCv.notify_one();
		}

		return true;
	}

	void
	TrackMixer::playPendingTrack (const ObservableTrait * loaded) noexcept
	{
		std::shared_ptr< MusicResource > pending;
		bool success = false;
		bool switching = false;

		{
			const MutexLock lock{m_stateAccess};

			if ( m_loadingTrack == nullptr || static_cast< const ObservableTrait * >(m_loadingTrack.get()) != loaded )
			{
				return;
			}

			pending = std::move(m_loadingTrack);
			m_loadingTrack.reset();

			/* NOTE: The user state and the index were set by play() when it asked for this track; a stop() since then
			 * dropped the claim above. Started here, never through play(): no recursion. */
			switching = m_crossFaderEnabled;
			success = this->startTrack(pending);
		}

		if ( success )
		{
			this->notifyTrackStarted(*pending, switching);
		}
	}

	bool
	TrackMixer::playIndex (size_t index) noexcept
	{
		std::shared_ptr< MusicResource > track;
		bool empty = false;

		{
			const MutexLock lock{m_stateAccess};

			empty = m_playlist.empty();

			if ( index < m_playlist.size() )
			{
				m_musicIndex = index;
				track = m_playlist[index];
			}
		}

		if ( empty )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		if ( track == nullptr )
		{
			Tracer::warning(ClassId, "Invalid playlist index !");

			return false;
		}

		return this->play(track);
	}

	void
	TrackMixer::stepIndex (bool forward) noexcept
	{
		/* NOTE: Handle shuffle mode. playlistChanged() keeps the order the size of the playlist. */
		if ( m_shuffleEnabled && m_shuffleOrder.size() == m_playlist.size() )
		{
			if ( forward )
			{
				m_shuffleIndex = m_shuffleIndex + 1 < m_shuffleOrder.size() ? m_shuffleIndex + 1 : 0;
			}
			else
			{
				m_shuffleIndex = m_shuffleIndex == 0 || m_shuffleIndex > m_shuffleOrder.size() ? m_shuffleOrder.size() - 1 : m_shuffleIndex - 1;
			}

			m_musicIndex = m_shuffleOrder[m_shuffleIndex];

			return;
		}

		/* NOTE: A track played outside the playlist leaves the index at SIZE_MAX: the next one is the first. */
		if ( forward )
		{
			m_musicIndex = m_musicIndex + 1 < m_playlist.size() ? m_musicIndex + 1 : 0;
		}
		else
		{
			m_musicIndex = m_musicIndex == 0 || m_musicIndex >= m_playlist.size() ? m_playlist.size() - 1 : m_musicIndex - 1;
		}
	}

	bool
	TrackMixer::stepTrack (bool forward) noexcept
	{
		std::shared_ptr< MusicResource > track;
		size_t index = 0;
		bool stopped = false;

		{
			const MutexLock lock{m_stateAccess};

			/* NOTE: If the player is paused, we don't change anything. */
			if ( m_userState == UserState::Paused )
			{
				return false;
			}

			if ( !m_playlist.empty() )
			{
				this->stepIndex(forward);

				index = m_musicIndex;
				stopped = m_userState == UserState::Stopped;
				track = m_playlist[index];
			}
		}

		if ( track == nullptr )
		{
			Tracer::warning(ClassId, "The playlist is empty !");

			return false;
		}

		/* NOTE: If the player is stopped, we just change the track index and notify. */
		if ( stopped )
		{
			this->notify(TrackChanged, index);

			return true;
		}

		const bool success = this->play(track);

		/* NOTE: Notify that the track index has changed (for UI update). */
		if ( success )
		{
			this->notify(TrackChanged, index);
		}

		return success;
	}

	bool
	TrackMixer::next () noexcept
	{
		return this->stepTrack(true);
	}

	bool
	TrackMixer::previous () noexcept
	{
		return this->stepTrack(false);
	}

	float
	TrackMixer::currentPosition () const noexcept
	{
		if ( !this->usable() )
		{
			return 0.0F;
		}

		const MutexLock lock{m_stateAccess};

		switch ( m_playingTrack )
		{
			case PlayingTrack::TrackA :
				return m_trackA->playbackPosition();

			case PlayingTrack::TrackB :
				return m_trackB->playbackPosition();

			case PlayingTrack::None :
				return 0.0F;
		}

		return 0.0F;
	}

	float
	TrackMixer::currentDuration () const noexcept
	{
		std::shared_ptr< MusicResource > track;

		{
			const MutexLock lock{m_stateAccess};

			if ( m_musicIndex >= m_playlist.size() )
			{
				return 0.0F;
			}

			track = m_playlist[m_musicIndex];
		}

		return track != nullptr ? track->duration() : 0.0F;
	}

	std::vector< std::string >
	TrackMixer::availablePlaylistNames () const noexcept
	{
		auto * container = m_resourceManager.container< PlaylistResource >();

		if ( container == nullptr )
		{
			return {};
		}

		/* Merge store-backed (disk .json) + loaded-in-memory (includes runtime-created via createResource)
		 * into a sorted, deduplicated set so callers get a single canonical list. */
		std::set< std::string > unique;

		for ( const auto & name : container->getResourceNames() )
		{
			unique.insert(name);
		}

		for ( const auto & name : container->loadedResourceNames() )
		{
			unique.insert(name);
		}

		return {unique.begin(), unique.end()};
	}

	bool
	TrackMixer::loadPlaylist (const std::shared_ptr< PlaylistResource > & playlist) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The track mixer is unavailable !");

			return false;
		}

		if ( playlist == nullptr )
		{
			Tracer::error(ClassId, "The playlist manifest is a null pointer !");

			return false;
		}

		/* Resolve each track name against the MusicResource container BEFORE touching the current playlist,
		 * so that a failed resolution never destroys user state. */
		auto * musics = m_resourceManager.container< MusicResource >();

		std::vector< std::shared_ptr< MusicResource > > resolved;
		resolved.reserve(playlist->trackCount());

		for ( const auto & trackName : playlist->trackNames() )
		{
			if ( !musics->isResourceExists(trackName) )
			{
				TraceWarning{ClassId} << "Playlist manifest '" << playlist->name() << "' references unknown track '" << trackName << "'. Skipped.";

				continue;
			}

			resolved.push_back(musics->getResource(trackName));
		}

		if ( resolved.empty() )
		{
			TraceError{ClassId} << "Playlist manifest '" << playlist->name() << "' resolved to zero valid tracks. Previous playlist preserved.";

			return false;
		}

		/* Commit phase: capture state, tear down, refill.
		 * NOTE: use userState() rather than isPlaying() because isPlaying() only reflects whether a source is
		 * actively emitting now. MIDI rendering is async — a track that the user asked to play may still be
		 * in the "loading/rendering" window, which would read isPlaying()==false. userState() captures the
		 * persistent intent ("user wants music"), which is the correct trigger for auto-resume after a swap. */
		const bool wasPlaying = this->userState() == UserState::Playing;
		const auto trackCount = resolved.size();

		this->stop();

		{
			const MutexLock lock{m_stateAccess};

			/* NOTE: The manifest is recorded with the tracks in one step: "any ad-hoc mutation severs the manifest
			 * link" (addToPlaylist()) holds, and no reader sees the new tracks under the old manifest. */
			m_playlist = std::move(resolved);
			m_loadedPlaylist = playlist;
			m_musicIndex = 0;

			this->playlistChanged();
		}

		TraceSuccess{ClassId} << "Loaded playlist '" << playlist->name() << "' with " << trackCount << " track(s).";

		/* Notify observers (UI layer, AI tooling, etc.) that the playlist changed. Carries the new manifest name. */
		this->notify(PlaylistSwapped, playlist->name());

		if ( wasPlaying )
		{
			return this->playIndex(0);
		}

		return true;
	}

	std::shared_ptr< MusicResource >
	TrackMixer::findPlaylistTrack (const std::string & partialName) const noexcept
	{
		if ( partialName.empty() )
		{
			return nullptr;
		}

		const auto needle = String::toLower(partialName);

		const MutexLock lock{m_stateAccess};

		for ( const auto & track : m_playlist )
		{
			if ( track == nullptr )
			{
				continue;
			}

			if ( String::toLower(track->name()).find(needle) != std::string::npos )
			{
				return track;
			}
		}

		return nullptr;
	}

	void
	TrackMixer::seek (float position) const noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		const MutexLock lock{m_stateAccess};

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
		const MutexLock lock{m_stateAccess};

		m_shuffleEnabled = state;

		this->playlistChanged();
	}

	void
	TrackMixer::playlistChanged () noexcept
	{
		if ( !m_shuffleEnabled || m_playlist.empty() )
		{
			m_shuffleOrder.clear();
			m_shuffleIndex = 0;

			return;
		}

		this->generateShuffleOrder();

		/* Find current track in shuffle order to continue from there. */
		if ( const auto entry = std::ranges::find(m_shuffleOrder, m_musicIndex); entry != m_shuffleOrder.end() )
		{
			m_shuffleIndex = static_cast< size_t >(entry - m_shuffleOrder.begin());
		}
	}

	void
	TrackMixer::generateShuffleOrder () noexcept
	{
		m_shuffleOrder.resize(m_playlist.size());
		std::iota(m_shuffleOrder.begin(), m_shuffleOrder.end(), size_t{0});

		/* NOTE: Not a seeded result (the generator is seeded once from std::random_device), but the cascade's shuffle
		 * all the same (coding style: never std::shuffle). */
		PortableRandom::shuffle(m_shuffleOrder, m_shuffleGenerator);

		m_shuffleIndex = 0;
	}

	bool
	TrackMixer::isPlaying () const noexcept
	{
		if ( !this->usable() )
		{
			return false;
		}

		const MutexLock lock{m_stateAccess};

		return m_playingTrack != PlayingTrack::None;
	}

	void
	TrackMixer::pause () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		{
			const MutexLock lock{m_stateAccess};

			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					return;

				case PlayingTrack::TrackA :
					m_trackA->pause();
					break;

				case PlayingTrack::TrackB :
					m_trackB->pause();
					break;
			}

			m_userState = UserState::Paused;
		}

		/* NOTE: Notified after the lock: an observer may call the mixer back (it used to deadlock). */
		this->notify(MusicPaused);
	}

	void
	TrackMixer::resume () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		{
			const MutexLock lock{m_stateAccess};

			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					return;

				case PlayingTrack::TrackA :
					m_trackA->resume();
					break;

				case PlayingTrack::TrackB :
					m_trackB->resume();
					break;
			}

			m_userState = UserState::Playing;
		}

		/* NOTE: Notified after the lock: an observer may call the mixer back (it used to deadlock). */
		this->notify(MusicResumed);
	}

	void
	TrackMixer::stop () noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		{
			const MutexLock lock{m_stateAccess};

			m_userState = UserState::Stopped;

			/* NOTE: A track still loading must not start once loaded: the user stopped the music. */
			m_loadingTrack.reset();

			switch ( m_playingTrack )
			{
				case PlayingTrack::None:
					return;

				case PlayingTrack::TrackA :
					m_trackA->stop();
					break;

				case PlayingTrack::TrackB :
					m_trackB->stop();
					break;
			}

			m_playingTrack = PlayingTrack::None;
			m_isFading = false;
		}

		/* NOTE: Notified after the lock: an observer may call the mixer back (it used to deadlock). */
		this->notify(MusicStopped);
	}

	bool
	TrackMixer::onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & /*data*/) noexcept
	{
		if ( observable->is(MusicResource::getClassUID()) )
		{
			/* NOTE: A resource loading thread may send this: everything goes through the lock. */
			switch ( notificationCode )
			{
				/* The track loaded successfully, we can now play it (if it is still the one waited for). */
				case Resources::ResourceTrait::LoadFinished :
					this->playPendingTrack(observable);
					break;

				case Resources::ResourceTrait::LoadFailed :
				{
					bool waited = false;

					{
						const MutexLock lock{m_stateAccess};

						if ( m_loadingTrack != nullptr && static_cast< const ObservableTrait * >(m_loadingTrack.get()) == observable )
						{
							m_loadingTrack.reset();

							waited = true;
						}
					}

					if ( waited )
					{
						Tracer::warning(ClassId, "The track has failed to load ! Cancelling the playback...");
					}
				}
					break;

				default:
					if constexpr ( ObserverDebugEnabled )
					{
						TraceDebug{ClassId} << "Event #" << notificationCode << " from a music resource ignored.";
					}
					break;
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
	TrackMixer::fadeStep () noexcept
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
				return;

			case PlayingTrack::TrackB :
				if ( !m_trackA->isMuted() )
				{
					TrackMixer::fadeOut(m_trackA.get(), stepValue);
				}

				if ( this->fadeIn(m_trackB.get(), stepValue) )
				{
					m_isFading = false;
				}
				return;

			case PlayingTrack::None :
				break;
		}

		/* NOTE: No playing track, or a value outside the enumeration: nothing to fade (the loop would otherwise spin
		 * every 16 ms until the shutdown). */
		m_isFading = false;
	}

	void
	TrackMixer::eventLoop (const std::stop_token & stopToken) noexcept
	{
		using namespace std::chrono_literals;

		/* NOTE: Every read and write of the shared state (the flags, the user state, the playing track) happens under
		 * m_stateAccess; only the 16 ms sleep between two fade steps is outside it. */
		while ( true )
		{
			bool playNext = false;

			{
				MutexLock lock{m_stateAccess};

				/* NOTE: Returns false when the stop is requested (the predicate still false). */
				if ( !m_fadeCv.wait(lock, stopToken, [this] () EMEN_REQUIRES(m_stateAccess) {
					return m_isFading || m_requestNextTrack;
				}) )
				{
					return;
				}

				if ( m_requestNextTrack )
				{
					m_requestNextTrack = false;
					playNext = m_userState != UserState::Stopped;
				}
			}

			/* NOTE: next() takes m_stateAccess itself, and notifies the observers from this thread. */
			if ( playNext )
			{
				this->next();
			}

			while ( true )
			{
				{
					const MutexLock lock{m_stateAccess};

					if ( !m_isFading || stopToken.stop_requested() )
					{
						break;
					}

					this->fadeStep();
				}

				std::this_thread::sleep_for(16ms);
			}
		}
	}
}
