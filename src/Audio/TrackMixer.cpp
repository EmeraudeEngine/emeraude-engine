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

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <thread>
#include <utility>

/* Third-party inclusions. */
#include "AL/al.h"

/* Local inclusions. */
#include "Any.hpp"
#include "Audio/Buffer.hpp"
#include "Audio/Manager.hpp"
#include "Audio/MusicResource.hpp"
#include "Audio/PlaylistResource.hpp"
#include "Audio/Source.hpp"
#include "Audio/Types.hpp"
#include "BlobTrait.hpp"
#include "Logging/Severity.hpp"
#include "Math/Base.hpp"
#include "PortableRandom.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Container.hpp"
#include "Resources/Manager.hpp"
#include "Resources/ResourceTrait.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "String.hpp"
#include "Tracer.hpp"
#include "WaveFactory/Types.hpp"
#include "WaveFactory/Wave.hpp"

namespace EmEn::Audio
{
	using namespace Base;

	Source &
	TrackMixer::trackSource (PlayingTrack which) const noexcept
	{
		return which == PlayingTrack::TrackB ? *m_trackB : *m_trackA;
	}

	TrackMixer::TrackStream &
	TrackMixer::trackStream (PlayingTrack which) noexcept
	{
		return which == PlayingTrack::TrackB ? m_streamB : m_streamA;
	}

	const TrackMixer::TrackStream &
	TrackMixer::trackStream (PlayingTrack which) const noexcept
	{
		return which == PlayingTrack::TrackB ? m_streamB : m_streamA;
	}

	bool
	TrackMixer::createStreamBuffers () noexcept
	{
		for ( auto * stream : {&m_streamA, &m_streamB} )
		{
			for ( auto & slot : stream->ring )
			{
				slot.buffer = std::make_unique< Buffer >();

				if ( !slot.buffer->isCreated() )
				{
					return false;
				}
			}
		}

		return true;
	}

	bool
	TrackMixer::openStream (PlayingTrack which, const std::shared_ptr< MusicResource > & music, size_t startFrame) noexcept
	{
		this->closeStream(which);

		if ( music == nullptr )
		{
			return false;
		}

		const auto & wave = music->localData();
		const auto channels = static_cast< size_t >(wave.channels());
		const auto frameCount = wave.sampleCount();

		if ( channels == 0 || frameCount == 0 )
		{
			TraceWarning{ClassId} << "The track '" << music->name() << "' has no sample, playback cancelled.";

			return false;
		}

		auto & stream = this->trackStream(which);
		auto & source = this->trackSource(which);

		/* NOTE: A chunk holds whole frames: a chunk size that is not a multiple of the channel count would split one
		 * between two buffers. */
		stream.chunkSize = std::max(channels, m_audioManager.musicChunkSize() / channels * channels);

		const auto chunkFrames = stream.chunkSize / channels;
		const auto frame = std::min(startFrame, frameCount - 1);

		stream.music = music;
		stream.nextChunk = frame / chunkFrames;
		stream.firstFrame = stream.nextChunk * chunkFrames;
		stream.consumedFrames = 0;
		stream.exhausted = false;

		this->fillStream(which);

		if ( source.queuedBufferCount() == 0 )
		{
			TraceError{ClassId} << "Unable to queue the first buffers of the track '" << music->name() << "' !";

			this->closeStream(which);

			return false;
		}

		/* NOTE: The stream starts at a chunk boundary; the source's cursor does the rest. */
		if ( frame > stream.firstFrame )
		{
			source.setSampleOffset(frame - stream.firstFrame);
		}

		if ( !source.playQueue() )
		{
			this->closeStream(which);

			return false;
		}

		return true;
	}

	void
	TrackMixer::closeStream (PlayingTrack which) noexcept
	{
		auto & stream = this->trackStream(which);

		/* NOTE: Stopped, then the null buffer: the whole queue comes back, played or not. */
		this->trackSource(which).detachBuffers();

		for ( auto & slot : stream.ring )
		{
			slot.queued = false;
			slot.frames = 0;
		}

		stream.music.reset();
		stream.nextChunk = 0;
		stream.firstFrame = 0;
		stream.consumedFrames = 0;
		stream.exhausted = false;
	}

	void
	TrackMixer::fillStream (PlayingTrack which) noexcept
	{
		auto & stream = this->trackStream(which);

		if ( stream.music == nullptr || stream.chunkSize == 0 )
		{
			return;
		}

		const auto & wave = stream.music->localData();
		const auto channels = static_cast< size_t >(wave.channels());
		const auto sampleCount = wave.elementCount();
		const auto chunkCount = wave.chunkCount(stream.chunkSize);

		auto & source = this->trackSource(which);

		for ( auto & slot : stream.ring )
		{
			if ( slot.queued )
			{
				continue;
			}

			if ( stream.nextChunk >= chunkCount )
			{
				/* NOTE: The mode is read here, when the end of the samples is reached: setPlayMode() applies to the
				 * track being played. */
				if ( m_playMode != PlayMode::Loop )
				{
					stream.exhausted = true;

					return;
				}

				stream.nextChunk = 0;
			}

			stream.exhausted = false;

			const auto offset = stream.nextChunk * stream.chunkSize;
			const auto samples = std::min(stream.chunkSize, sampleCount - offset);

			if ( slot.buffer == nullptr || !slot.buffer->feedData(wave, stream.nextChunk, stream.chunkSize) )
			{
				TraceError{ClassId} << "Unable to fill a stream buffer with the track '" << stream.music->name() << "' !";

				return;
			}

			const std::array< ALuint, 1 > name{slot.buffer->identifier()};

			if ( !source.queueBuffers(name) )
			{
				return;
			}

			slot.queued = true;
			slot.frames = samples / channels;

			++stream.nextChunk;
		}
	}

	void
	TrackMixer::serviceStream (PlayingTrack which) noexcept
	{
		auto & stream = this->trackStream(which);

		if ( stream.music == nullptr )
		{
			return;
		}

		auto & source = this->trackSource(which);

		/* 1. The buffers the source has played come back to the ring. */
		std::array< ALuint, StreamRingSize > played{};

		const auto playedCount = source.unqueueProcessedBuffers(played);

		for ( const auto name : std::span{played}.first(playedCount) )
		{
			for ( auto & slot : stream.ring )
			{
				if ( slot.queued && slot.buffer != nullptr && slot.buffer->identifier() == name )
				{
					stream.consumedFrames += slot.frames;
					slot.queued = false;

					break;
				}
			}
		}

		/* 2. They leave again with the next chunks. */
		this->fillStream(which);

		if ( !source.isStopped() )
		{
			return;
		}

		/* 3. The source stopped by itself: it played everything it had. */
		if ( source.queuedBufferCount() > 0 )
		{
			/* NOTE: An underrun: this thread came back too late and the queue ran dry while samples remained. The
			 * buffers just queued are played from the first one. */
			TraceWarning{ClassId} << "The stream of the track '" << stream.music->name() << "' ran dry (underrun), restarted.";

			source.playQueue();

			return;
		}

		if ( !stream.exhausted )
		{
			/* NOTE: Nothing could be queued (OpenAL refused, traced by fillStream()): the next turn tries again. */
			return;
		}

		/* 4. The end of the track. */
		this->closeStream(which);

		if ( which == m_playingTrack )
		{
			/* NOTE: No track is held any more: the next one starts on a free mixer, without a fade from silence. */
			m_playingTrack = PlayingTrack::None;
			m_playingMusic.reset();
			m_isFading = false;
			m_requestNextTrack = true;
		}
	}

	size_t
	TrackMixer::streamPosition (PlayingTrack which) const noexcept
	{
		const auto & stream = this->trackStream(which);

		if ( stream.music == nullptr )
		{
			return 0;
		}

		const auto frameCount = stream.music->localData().sampleCount();

		if ( frameCount == 0 )
		{
			return 0;
		}

		/* NOTE: A looping stream runs past the end of the music: the position is within the track. */
		return (stream.firstFrame + stream.consumedFrames + this->trackSource(which).sampleOffset()) % frameCount;
	}

	bool
	TrackMixer::onInitialize () noexcept
	{
		bool streamBuffersCreated = true;

		{
			const MutexLock lock{m_stateAccess};

			m_trackA = std::make_unique< Source >();
			m_trackA->setRelativeState(true);
			m_trackA->setRolloffFactor(0.0F);
			m_trackA->setGain(0.0F);

			m_trackB = std::make_unique< Source >();
			m_trackB->setRelativeState(true);
			m_trackB->setRolloffFactor(0.0F);
			m_trackB->setGain(0.0F);

			/* NOTE: The end of a track is seen by the mixer's own thread (serviceStream()): a playlist advances
			 * whatever the OpenAL implementation. It used to rely on the source events of OpenAL Soft, and looped one
			 * track without them. */
			m_playMode = PlayMode::Once;

			if ( !this->createStreamBuffers() )
			{
				Tracer::error(ClassId, "Unable to create the stream buffers: the track mixer is disabled !");

				streamBuffersCreated = false;
			}
		}

		if ( !streamBuffersCreated )
		{
			static_cast< void >(this->onTerminate());

			return false;
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
		/* NOTE: The event thread first (it feeds the streams), then the streams, then their buffers and the sources. */
		this->stopEventThread();

		const MutexLock lock{m_stateAccess};

		if ( m_trackA != nullptr )
		{
			this->closeStream(PlayingTrack::TrackA);
		}

		if ( m_trackB != nullptr )
		{
			this->closeStream(PlayingTrack::TrackB);
		}

		m_playingTrack = PlayingTrack::None;
		m_playingMusic.reset();
		m_isFading = false;
		m_requestNextTrack = false;
		m_loadingTrack.reset();

		/* NOTE: The buffers of the rings are deleted here, detached and their device still open, like the sources. */
		for ( auto * stream : {&m_streamA, &m_streamB} )
		{
			for ( auto & slot : stream->ring )
			{
				slot.buffer.reset();
			}
		}

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
				this->closeStream(PlayingTrack::TrackB);
				/* Ensure track A is at full volume. */
				m_trackA->setGain(m_gain);
				break;

			case PlayingTrack::TrackB :
				/* Stop track A if it's playing. */
				this->closeStream(PlayingTrack::TrackA);
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

		/* NOTE: The stream of the track being played reads it when it reaches the end of the samples (fillStream()). */
		m_playMode = mode;
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
		status.playingTrack = m_playingMusic;
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
					this->closeStream(PlayingTrack::TrackA);
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
					this->closeStream(PlayingTrack::TrackB);
				}
				break;
		}

		const float initialGain = m_crossFaderEnabled && wasPlaying ? 0.0F : m_gain;

		this->trackSource(m_playingTrack).setGain(initialGain);

		if ( !this->openStream(m_playingTrack, track) )
		{
			/* NOTE: A failed start leaves the state as it really is: the old track still plays when it was fading out,
			 * nothing plays when it was stopped above. */
			m_isFading = false;
			m_playingTrack = m_crossFaderEnabled ? previousTrack : PlayingTrack::None;

			if ( m_playingTrack == PlayingTrack::None )
			{
				m_playingMusic.reset();
			}

			return false;
		}

		m_playingMusic = track;

		/* NOTE: The event thread sleeps while no track is held: it now has a stream to feed, and maybe a fade. */
		m_fadeCv.notify_one();

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

			/* NOTE: A moved-from std::shared_ptr is empty (guaranteed): the claim is released here. */
			pending = std::move(m_loadingTrack);

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

		if ( m_playingTrack == PlayingTrack::None || m_playingMusic == nullptr )
		{
			return 0.0F;
		}

		const auto hertz = static_cast< float >(m_playingMusic->localData().frequency().hz());

		return hertz > 0.0F ? static_cast< float >(this->streamPosition(m_playingTrack)) / hertz : 0.0F;
	}

	float
	TrackMixer::currentDuration () const noexcept
	{
		std::shared_ptr< MusicResource > track;

		{
			const MutexLock lock{m_stateAccess};

			/* NOTE: The track the source holds first: a track played by name outside the playlist has no index
			 * (SIZE_MAX), so its duration read as 0 s and the console refused every seek ("between 0 and 0 seconds").
			 * With no source playing, the playlist entry at the index, as before. */
			if ( m_playingMusic != nullptr )
			{
				track = m_playingMusic;
			}
			else if ( m_musicIndex < m_playlist.size() )
			{
				track = m_playlist[m_musicIndex];
			}
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
	TrackMixer::seek (float position) noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		if ( !std::isfinite(position) )
		{
			Tracer::warning(ClassId, "The seek position is not a finite number, ignored.");

			return;
		}

		const MutexLock lock{m_stateAccess};

		if ( m_playingTrack == PlayingTrack::None || m_playingMusic == nullptr )
		{
			return;
		}

		/* NOTE: A copy: openStream() closes the stream first, which lets its own reference go. */
		const auto music = m_playingMusic;
		const auto & wave = music->localData();
		const auto hertz = static_cast< float >(wave.frequency().hz());
		const auto frameCount = wave.sampleCount();

		if ( frameCount == 0 || hertz <= 0.0F )
		{
			return;
		}

		const auto seconds = std::clamp(position, 0.0F, static_cast< float >(frameCount - 1) / hertz);
		const auto frame = std::min(static_cast< size_t >(seconds * hertz), frameCount - 1);
		const auto wasPaused = this->trackSource(m_playingTrack).isPaused();

		/* NOTE: OpenAL only holds the few buffers queued: a seek empties the ring and fills it again from there. */
		if ( !this->openStream(m_playingTrack, music, frame) )
		{
			m_playingTrack = PlayingTrack::None;
			m_playingMusic.reset();
			m_isFading = false;

			return;
		}

		if ( wasPaused )
		{
			this->trackSource(m_playingTrack).pause();
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

			if ( m_playingTrack == PlayingTrack::None )
			{
				return;
			}

			/* NOTE: Both streams: a track fading out when the music is stopped went on playing at the gain the fade
			 * had left it. */
			this->closeStream(PlayingTrack::TrackA);
			this->closeStream(PlayingTrack::TrackB);

			m_playingTrack = PlayingTrack::None;
			m_playingMusic.reset();
			m_isFading = false;
			m_requestNextTrack = false;
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
	TrackMixer::fadeOut (PlayingTrack which, float step) noexcept
	{
		auto & source = this->trackSource(which);

		const auto currentGain = source.gain() - step;

		if ( currentGain <= 0.0F )
		{
			source.setGain(0.0F);

			this->closeStream(which);
		}
		else
		{
			source.setGain(currentGain);
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
				if ( m_streamB.music != nullptr )
				{
					this->fadeOut(PlayingTrack::TrackB, stepValue);
				}

				if ( this->fadeIn(m_trackA.get(), stepValue) && m_streamB.music == nullptr )
				{
					m_isFading = false;
				}
				return;

			case PlayingTrack::TrackB :
				if ( m_streamA.music != nullptr )
				{
					this->fadeOut(PlayingTrack::TrackA, stepValue);
				}

				if ( this->fadeIn(m_trackB.get(), stepValue) && m_streamA.music == nullptr )
				{
					m_isFading = false;
				}
				return;

			case PlayingTrack::None :
				break;
		}

		/* NOTE: No playing track, or a value outside the enumeration: nothing to fade (the loop would otherwise spin
		 * every turn until the shutdown). */
		m_isFading = false;
	}

	void
	TrackMixer::eventLoop (const std::stop_token & stopToken) noexcept
	{
		/* NOTE: Every read and write of the shared state (the streams, the flags, the user state, the playing track)
		 * happens under m_stateAccess; only the sleep between two turns is outside it. */
		while ( true )
		{
			bool playNext = false;
			bool busy = false;

			{
				MutexLock lock{m_stateAccess};

				/* NOTE: Returns false when the stop is requested (the predicate still false). */
				if ( !m_fadeCv.wait(lock, stopToken, [this] () EMEN_REQUIRES(m_stateAccess) {
					return m_isFading || m_requestNextTrack || m_streamA.music != nullptr || m_streamB.music != nullptr;
				}) )
				{
					return;
				}

				this->serviceStream(PlayingTrack::TrackA);
				this->serviceStream(PlayingTrack::TrackB);

				if ( m_requestNextTrack )
				{
					m_requestNextTrack = false;
					playNext = m_userState != UserState::Stopped;
				}

				if ( m_isFading )
				{
					this->fadeStep();
				}

				busy = m_isFading || m_streamA.music != nullptr || m_streamB.music != nullptr;
			}

			/* NOTE: next() takes m_stateAccess itself, and notifies the observers from this thread. */
			if ( playNext )
			{
				this->next();
			}

			if ( stopToken.stop_requested() )
			{
				return;
			}

			if ( busy )
			{
				std::this_thread::sleep_for(EventPeriod);
			}
		}
	}
}
