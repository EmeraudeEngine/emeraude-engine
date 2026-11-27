/*
 * src/Audio/TrackMixer.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <stop_token>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/ControllableTrait.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Buffer.hpp" // IWYU pragma: keep (the ring slots hold a std::unique_ptr< Buffer > the mixer destroys inline: the complete type is needed)
#include "Hash/FNV1a.hpp"
#include "MusicResource.hpp"
#include "Mutex.hpp"
#include "ObservableTrait.hpp"
#include "Source.hpp" // IWYU pragma: keep (the inline constructor needs the complete type of its std::unique_ptr< Source > members when a consumer builds with exceptions)
#include "Thread.hpp"
#include "ThreadSafety.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Audio
	{
		class Manager;
		class PlaylistResource;
	}

	namespace Resources
	{
		class Manager;
	}

	class PrimaryServices;
}

namespace EmEn::Audio
{
	/**
	 * @brief The track mixer service class.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Console::ControllableTrait The console can control the track mixer.
	 * @extends EmEn::Base::ObservableTrait This service is observable.
	 * @extends EmEn::Base::ObserverTrait This service can observe resource loading.
	 * @note A track is STREAMED: the mixer owns a ring of a few OpenAL buffers per source (TrackStream) and its event
	 * thread refills them from the samples of the music resource, which holds no OpenAL buffer. The position, the
	 * seek, the loop and the end of a track are therefore the mixer's, not OpenAL's.
	 * @note [Thread-safe] Every public method may be called from any thread. Three threads use the mixer: the caller's
	 * (main, logic, console), its own event thread (the streams, the end of a track and the next one, the cross-fade)
	 * and a resource loading thread (onNotification()). One mutex, m_stateAccess, guards every piece of state,
	 * Clang-checked (EMEN_GUARDED_BY, decision D12). The observers are notified AFTER the lock is released, on the
	 * calling thread: an observer may call the mixer back.
	 */
	class EMEN_API TrackMixer final : public ServiceInterface, public Console::ControllableTrait, public Base::ObservableTrait, public Base::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TrackMixerService"};

			/** @brief Define the track mixer user state (Not OpenAL). */
			enum class UserState : uint8_t
			{
				/** @brief The user stopped the music. */
				Stopped,
				/** @brief The user started the music. */
				Playing,
				/** @brief The user paused the music. */
				Paused
			};

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
			{
				MusicPlaying,
				MusicSwitching,
				MusicPaused,
				MusicResumed,
				MusicStopped,
				TrackChanged, /**< @brief Notifies that the current track index has changed (without playback). */
				PlaylistSwapped, /**< @brief Notifies that the playlist has been fully replaced via loadPlaylist(). Observers may refresh their UI. */
				/* Enumeration boundary. */
				MaxEnum
			};

			/** @brief A consistent copy of the mixer state, taken under one lock (status()). */
			struct Status final
			{
				/** @brief The playlist. */
				std::vector< std::shared_ptr< MusicResource > > playlist;
				/** @brief The playlist entry at the current index, nullptr when the index is outside the playlist. */
				std::shared_ptr< MusicResource > currentTrack;
				/** @brief The track the playing source holds (playing or paused), a playlist entry or not; nullptr when
				 * no source holds one. */
				std::shared_ptr< MusicResource > playingTrack;
				/** @brief The manifest backing the playlist, nullptr for an ad-hoc playlist. */
				std::shared_ptr< PlaylistResource > loadedPlaylist;
				/** @brief The current index in the playlist (SIZE_MAX for a track played outside of it). */
				size_t trackIndex{0};
				/** @brief The music gain. */
				float gain{0.0F};
				/** @brief The user state. */
				UserState userState{UserState::Stopped};
				/** @brief The play mode. */
				PlayMode playMode{PlayMode::Loop};
				/** @brief Whether a source holds a track (playing or paused). */
				bool hasTrack{false};
				/** @brief Whether the cross-fader is enabled. */
				bool crossFaderEnabled{false};
				/** @brief Whether shuffle is enabled. */
				bool shuffleEnabled{false};
			};

			/**
			 * @brief Constructs a track mixer.
			 * @param primaryServices A reference to primary services.
			 * @param resourceManager A reference to the resource manager.
			 * @param audioManager A reference to the audio manager.
			 */
			TrackMixer (PrimaryServices & primaryServices, Resources::Manager & resourceManager, Manager & audioManager) noexcept
				: ServiceInterface{ClassId},
				ControllableTrait{ClassId},
				m_primaryServices{primaryServices},
				m_resourceManager{resourceManager},
				m_audioManager{audioManager}
			{

			}

			/**
			 * @brief Destructs the track mixer: the event thread is stopped and joined BEFORE any member it uses is
			 * destroyed, whether onTerminate() ran or not (an initialization failure path skips it).
			 */
			~TrackMixer () override;

			/** @brief Copy constructor (deleted: the event thread and the OpenAL sources are owned). */
			TrackMixer (const TrackMixer & copy) noexcept = delete;

			/** @brief Move constructor (deleted: the event thread captures this). */
			TrackMixer (TrackMixer && copy) noexcept = delete;

			/** @brief Copy assignment (deleted). */
			TrackMixer & operator= (const TrackMixer & copy) noexcept = delete;

			/** @brief Move assignment (deleted). */
			TrackMixer & operator= (TrackMixer && copy) noexcept = delete;

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Sets the track gain.
			 * @param volume The gain.
			 */
			void setVolume (float volume) noexcept;

			/**
			 * @brief Returns the current gain.
			 * @return float
			 */
			[[nodiscard]]
			float
			volume () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_gain;
			}

			/**
			 * @brief Enables the cross-fader.
			 * @note When disabling, stops any ongoing fade and ensures only the current track plays.
			 * @param state The state.
			 */
			void enableCrossFader (bool state) noexcept;

			/**
			 * @brief Returns whether the cross-fader is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCrossFaderEnabled () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_crossFaderEnabled;
			}

			/**
			 * @brief Sets the play mode (Once or Loop).
			 * @note Applies to the track being played too: its stream wraps, or ends, according to the mode read when
			 * it reaches the end of the samples.
			 * @param mode The play mode.
			 */
			void setPlayMode (PlayMode mode) noexcept;

			/**
			 * @brief Returns the current play mode.
			 * @return PlayMode
			 */
			[[nodiscard]]
			PlayMode
			playMode () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_playMode;
			}

			/**
			 * @brief Adds a soundtrack to the playlist.
			 * @note Ad-hoc addition severs the link to any previously loaded playlist manifest
			 *	   (loadedPlaylist() will return nullptr after this call). Use loadPlaylist() to
			 *	   rebuild a playlist that is still backed by a named manifest.
			 * @param track A reference to a music resource.
			 */
			void addToPlaylist (const std::shared_ptr< MusicResource > & track) noexcept;

			/**
			 * @brief Replaces the current playlist with the content of a PlaylistResource manifest.
			 * @note Semantics:
			 *	   - Captures whether playback was active before any mutation.
			 *	   - Resolves every track name via the MusicResource container; unknown entries are skipped with a warning.
			 *	   - If the manifest is null or resolves to zero valid tracks, the previous playlist is left intact and the call returns false.
			 *	   - Otherwise: stops current playback, clears the playlist, refills it from the resolved tracks, and if playback was active
			 *		 AND the new playlist is non-empty, starts from index 0 via playIndex(0).
			 *	   - Shuffle ordering (if enabled) is regenerated automatically by the next playback start.
			 * @param playlist A shared pointer to the playlist manifest.
			 * @return bool True on successful swap, false on null pointer or empty resolution (no mutation performed).
			 */
			bool loadPlaylist (const std::shared_ptr< PlaylistResource > & playlist) noexcept;

			/**
			 * @brief Removes all soundtracks from the playlist.
			 * @note Also drops the loaded playlist manifest reference — there is no manifest backing an empty playlist.
			 */
			void clearPlaylist () noexcept;

			/**
			 * @brief Returns the sorted, deduplicated list of all PlaylistResource names the application can reach.
			 * @note Union of store-backed manifests (`data-stores/MusicPlaylists/` JSON files) and any PlaylistResource
			 *	   created at runtime via `Resources::Container::createResource()`. Useful for populating UI
			 *	   dropdowns without having to touch the Resources system directly.
			 * @return std::vector< std::string > Sorted playlist names. Empty if the container is unreachable.
			 */
			[[nodiscard]]
			std::vector< std::string > availablePlaylistNames () const noexcept;

			/**
			 * @brief Returns the manifest currently backing the playlist, if any.
			 * @note Returns nullptr when: no manifest was ever loaded, the playlist was cleared, or the current
			 *	   content was built ad-hoc via addToPlaylist() (any manual mutation severs the manifest link).
			 *	   Callers can query loadedPlaylist()->name() / trackCount() for display purposes.
			 * @return std::shared_ptr< PlaylistResource > A copy: the mixer may swap its playlist meanwhile.
			 */
			[[nodiscard]]
			std::shared_ptr< PlaylistResource >
			loadedPlaylist () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_loadedPlaylist;
			}

			/**
			 * @brief Returns a copy of the playlist.
			 * @note A copy, never a reference: another thread may change the playlist while the caller reads it.
			 * @return std::vector< std::shared_ptr< MusicResource > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< MusicResource > >
			playlist () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_playlist;
			}

			/**
			 * @brief Returns the playlist size.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			playlistSize () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_playlist.size();
			}

			/**
			 * @brief Returns the current track index in the playlist.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			currentTrackIndex () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_musicIndex;
			}

			/**
			 * @brief Returns a consistent copy of the whole mixer state (one lock).
			 * @return Status
			 */
			[[nodiscard]]
			Status status () const noexcept;

			/**
			 * @brief Finds the first playlist entry whose resource name contains the given substring (case-insensitive).
			 * @note Intended for remote-console and AI-driven workflows where the caller provides an approximate
			 *	   title, e.g. "Volcanian" resolves to "Kyrandia2/Volcanian Salesperson". Matching is restricted
			 *	   to the currently loaded playlist — not the full MusicResource container.
			 * @param partialName A partial or full track resource name. Empty input yields a nullptr result.
			 * @return std::shared_ptr< MusicResource > The first matching track, or nullptr if no entry matches.
			 */
			[[nodiscard]]
			std::shared_ptr< MusicResource > findPlaylistTrack (const std::string & partialName) const noexcept;

			/**
			 * @brief Returns the user state.
			 * @return UserState
			 */
			[[nodiscard]]
			UserState
			userState () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_userState;
			}

			/**
			 * @brief Plays the playlist.
			 * @return bool
			 */
			bool play () noexcept;

			/**
			 * @brief Plays a soundtrack.
			 * @param track A reference to a music resource.
			 * @return bool
			 */
			bool play (const std::shared_ptr< MusicResource > & track) noexcept;

			/**
			 * @brief Plays a track at the specified index in the playlist.
			 * @param index The index of the track in the playlist.
			 * @return bool
			 */
			bool playIndex (size_t index) noexcept;

			/**
			 * @brief Returns whether the soundtrack is playing.
			 * @return bool
			 */
			[[nodiscard]]
			bool isPlaying () const noexcept;

			/**
			 * @brief Starts the next music in the playlist.
			 * @return bool
			 */
			bool next () noexcept;

			/**
			 * @brief Starts the previous music in the playlist.
			 * @return bool
			 */
			bool previous () noexcept;

			/**
			 * @brief Returns the current playback position in seconds, within the track (a looping track starts again
			 * from 0).
			 * @return float
			 */
			[[nodiscard]]
			float currentPosition () const noexcept;

			/**
			 * @brief Returns the duration of the current track in seconds.
			 * @note The track the source holds (Status::playingTrack), in the playlist or not; with no source playing,
			 * the playlist entry at the current index. 0 when there is neither.
			 * @return float
			 */
			[[nodiscard]]
			float currentDuration () const noexcept;

			/**
			 * @brief Seeks to a position in the current track: its stream is emptied and filled again from there.
			 * @param position The position in seconds, clamped to the track; a value that is not finite is refused.
			 */
			void seek (float position) noexcept;

			/**
			 * @brief Enables or disables shuffle mode.
			 * @param state True to enable shuffle, false to disable.
			 */
			void enableShuffle (bool state) noexcept;

			/**
			 * @brief Returns whether shuffle mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShuffleEnabled () const noexcept
			{
				const Base::MutexLock lock{m_stateAccess};

				return m_shuffleEnabled;
			}

			/**
			 * @brief Pauses the music.
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes the music.
			 */
			void resume () noexcept;

			/**
			 * @brief Stops the music, a track fading out included. A track still loading will not start when it is
			 * loaded.
			 */
			void stop () noexcept;

		private:

			/** @brief The track type enumerations. */
			enum class PlayingTrack : uint8_t
			{
				None,
				TrackA,
				TrackB
			};

			/** @brief The number of OpenAL buffers a track streams through: with the default chunk (8192 samples, so
			 * 4096 stereo frames) about 0.68 s of music ahead at 48 kHz, 256 KiB. */
			static constexpr size_t StreamRingSize{8};

			/**
			 * @brief What one of the two sources streams: a ring of OpenAL buffers the mixer owns, and the cursor in
			 * the music they are filled from.
			 * @note OpenAL only knows the few buffers queued: the position within the music is firstFrame plus the
			 * frames of the buffers already taken back plus the source's own offset in its queue.
			 */
			struct TrackStream final
			{
				/** @brief One buffer of the ring. */
				struct Slot final
				{
					/** @brief The OpenAL buffer. Created by onInitialize(), deleted by onTerminate() (the device still open). */
					std::unique_ptr< Buffer > buffer;
					/** @brief The sample frames the buffer holds while it is queued. */
					size_t frames{0};
					/** @brief Whether the buffer is in the source's queue. */
					bool queued{false};
				};

				/** @brief The ring. */
				std::array< Slot, StreamRingSize > ring;
				/** @brief The music being streamed; nullptr when the track streams nothing. */
				std::shared_ptr< MusicResource > music;
				/** @brief The chunk size in samples (frames × channels), a whole number of frames. */
				size_t chunkSize{0};
				/** @brief The next chunk of the music to queue. */
				size_t nextChunk{0};
				/** @brief The frame of the music the first buffer queued since the stream was opened started at. */
				size_t firstFrame{0};
				/** @brief The frames of the buffers taken back from the queue since the stream was opened. */
				size_t consumedFrames{0};
				/** @brief True once the last chunk was queued in Once mode: nothing more to feed. */
				bool exhausted{false};
			};

			/** @brief The period of the event thread while a track is held or a fade runs. */
			static constexpr auto EventPeriod{std::chrono::milliseconds{16}};

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & data) noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief The event thread: refills the streams, plays the next track when one ends, runs the cross-fade.
			 * @note It sleeps while no track is held and nothing fades; otherwise it turns every 16 ms (EventPeriod).
			 * @param stopToken The thread's stop token (Base::Thread): a stop request wakes its wait and ends it, within
			 * one turn at most.
			 */
			void eventLoop (const std::stop_token & stopToken) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Fades in a track.
			 * @param track A pointer to the track.
			 * @param step The step of raising or lowering the volume.
			 * @return bool
			 */
			bool fadeIn (Source * track, float step) const noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Fades out a track; its stream is closed once silent.
			 * @param which The track (A or B).
			 * @param step The step of raising or lowering the volume.
			 */
			void fadeOut (PlayingTrack which, float step) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Returns the source of a track.
			 * @pre The mixer is initialized (the sources exist) and which is TrackA or TrackB.
			 * @param which The track; anything but TrackB gives the source A.
			 * @return Source &
			 */
			[[nodiscard]]
			Source & trackSource (PlayingTrack which) const noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Returns the stream of a track.
			 * @param which The track; anything but TrackB gives the stream A.
			 * @return TrackStream &
			 */
			[[nodiscard]]
			TrackStream & trackStream (PlayingTrack which) noexcept EMEN_REQUIRES(m_stateAccess);

			/** @copydoc trackStream(PlayingTrack) */
			[[nodiscard]]
			const TrackStream & trackStream (PlayingTrack which) const noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Creates the OpenAL buffers of both rings.
			 * @return bool False when OpenAL refused one of them.
			 */
			[[nodiscard]]
			bool createStreamBuffers () noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Starts streaming a loaded music on a track, from a position: whatever the track streamed is
			 * closed first, the ring is filled and the source started.
			 * @param which The track (A or B).
			 * @param music A reference to a LOADED music resource.
			 * @param startFrame The sample frame to start from, clamped to the music. Default the beginning.
			 * @return bool False when the music has no sample or OpenAL refused; the stream is then closed.
			 */
			[[nodiscard]]
			bool openStream (PlayingTrack which, const std::shared_ptr< MusicResource > & music, size_t startFrame = 0) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Stops a track's source, takes every buffer of the ring back and lets the music go. Idempotent.
			 * @param which The track (A or B).
			 */
			void closeStream (PlayingTrack which) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Fills every free buffer of a ring with the next chunks of its music and queues them.
			 * @note At the end of the samples the stream wraps in Loop mode, or is marked exhausted in Once mode.
			 * @param which The track (A or B).
			 */
			void fillStream (PlayingTrack which) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief One turn of a stream: takes the played buffers back, refills them, restarts a source that ran dry
			 * (an underrun, traced) and ends the track when its last sample was played.
			 * @note The end of the PLAYING track frees it (no track is held any more) and requests the next one.
			 * @param which The track (A or B).
			 */
			void serviceStream (PlayingTrack which) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Returns the position of a stream within its music, in sample frames.
			 * @param which The track (A or B).
			 * @return size_t 0 when the track streams nothing.
			 */
			[[nodiscard]]
			size_t streamPosition (PlayingTrack which) const noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Starts a loaded track on the free source (or the only one), with the cross-fade if enabled.
			 * @param track A reference to a loaded music resource.
			 * @return bool
			 */
			[[nodiscard]]
			bool startTrack (const std::shared_ptr< MusicResource > & track) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Notifies MusicSwitching or MusicPlaying for a track that started.
			 * @param track A reference to the track.
			 * @param switching Whether the cross-fader was enabled (a fade to it).
			 */
			void notifyTrackStarted (const MusicResource & track, bool switching) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Plays the track the mixer waited for, if it is this one. The claim on m_loadingTrack is taken under
			 * the lock: the loading notification and play()'s own re-check never both start it.
			 * @param loaded The resource that finished loading.
			 */
			void playPendingTrack (const ObservableTrait * loaded) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Keeps the shuffle order in step with the playlist (regenerated when shuffle is on, emptied when off).
			 * @note Every change of the playlist calls it: a stale order indexed past a shorter playlist.
			 */
			void playlistChanged () noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Moves the current index to the next or the previous entry (shuffle order or playlist order).
			 * @pre The playlist is not empty.
			 * @param forward True for the next entry.
			 */
			void stepIndex (bool forward) noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Plays the next or the previous entry (or only moves the index when stopped), then notifies.
			 * @param forward True for the next entry.
			 * @return bool
			 */
			bool stepTrack (bool forward) noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Generates a shuffled order for the playlist (Base::PortableRandom::shuffle()).
			 */
			void generateShuffleOrder () noexcept EMEN_REQUIRES(m_stateAccess);

			/**
			 * @brief Requests the event thread to stop and joins it. Idempotent.
			 * @note The stop flag is raised UNDER m_stateAccess, so the waiter cannot miss the notification between its
			 * predicate check and its wait.
			 */
			void stopEventThread () noexcept EMEN_EXCLUDES(m_stateAccess);

			/**
			 * @brief Advances the cross-fade by one step.
			 */
			void fadeStep () noexcept EMEN_REQUIRES(m_stateAccess);

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			Manager & m_audioManager;
			/* NOTE: The lock of every member below but the event thread (decision D12: Clang checks the protocol). */
			mutable Base::Mutex m_stateAccess;
			std::condition_variable_any m_fadeCv;
			/* NOTE: The pointers are set once by onInitialize() before any other thread exists; the SOURCES are used under
			 * the lock only (the cross-fade on the event thread and play() on the caller's used to race on the gains). */
			std::unique_ptr< Source > m_trackA EMEN_PT_GUARDED_BY(m_stateAccess);
			std::unique_ptr< Source > m_trackB EMEN_PT_GUARDED_BY(m_stateAccess);
			TrackStream m_streamA EMEN_GUARDED_BY(m_stateAccess);
			TrackStream m_streamB EMEN_GUARDED_BY(m_stateAccess);
			std::vector< std::shared_ptr< MusicResource > > m_playlist EMEN_GUARDED_BY(m_stateAccess);
			std::shared_ptr< PlaylistResource > m_loadedPlaylist EMEN_GUARDED_BY(m_stateAccess);
			std::shared_ptr< MusicResource > m_loadingTrack EMEN_GUARDED_BY(m_stateAccess);
			/** @brief The track the playing source holds (startTrack()), whatever the playlist index: a track played
			 * outside the playlist has no index (SIZE_MAX). Empty whenever m_playingTrack is None. */
			std::shared_ptr< MusicResource > m_playingMusic EMEN_GUARDED_BY(m_stateAccess);
			std::vector< size_t > m_shuffleOrder EMEN_GUARDED_BY(m_stateAccess);
			std::mt19937 m_shuffleGenerator EMEN_GUARDED_BY(m_stateAccess){std::random_device{}()};
			size_t m_musicIndex EMEN_GUARDED_BY(m_stateAccess){0};
			size_t m_shuffleIndex EMEN_GUARDED_BY(m_stateAccess){0};
			float m_gain EMEN_GUARDED_BY(m_stateAccess){0.0F};
			UserState m_userState EMEN_GUARDED_BY(m_stateAccess){UserState::Stopped};
			PlayMode m_playMode EMEN_GUARDED_BY(m_stateAccess){PlayMode::Loop};
			PlayingTrack m_playingTrack EMEN_GUARDED_BY(m_stateAccess){PlayingTrack::None};
			bool m_requestNextTrack EMEN_GUARDED_BY(m_stateAccess){false};
			bool m_crossFaderEnabled EMEN_GUARDED_BY(m_stateAccess){false};
			bool m_isFading EMEN_GUARDED_BY(m_stateAccess){false};
			bool m_shuffleEnabled EMEN_GUARDED_BY(m_stateAccess){false};
			/* NOTE: LAST on purpose (Ave Robustus II, member order): the event thread uses every member above, so it is
			 * destroyed — joined — first. The destructor also stops it explicitly before any member goes. */
			Base::Thread m_eventThread;
	};
}
