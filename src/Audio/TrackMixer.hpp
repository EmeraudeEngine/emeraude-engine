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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/ControllableTrait.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "MusicResource.hpp"
#include "PlaylistResource.hpp"
#include "Source.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Audio
	{
		class Manager;
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
	 * @extends EmEn::Libs::ObservableTrait This service is observable.
	 * @extends EmEn::Libs::ObserverTrait This service can observe resource loading.
	 */
	class TrackMixer final : public ServiceInterface, public Console::ControllableTrait, public Libs::ObservableTrait, public Libs::ObserverTrait
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
			enum NotificationCode
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
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
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
				return m_gain;
			}

			/**
			 * @brief Enables the cross-fader.
			 * @note When disabling, stops any ongoing fade and ensures only the current track plays.
			 * @param state The state.
			 * @return void
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
				return m_crossFaderEnabled;
			}

			/**
			 * @brief Sets the play mode (Once or Loop).
			 * @note Also updates the currently playing source if any.
			 * @param mode The play mode.
			 * @return void
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
				return m_playMode;
			}

			/**
			 * @brief Adds a soundtrack to the playlist.
			 * @note Ad-hoc addition severs the link to any previously loaded playlist manifest
			 *       (loadedPlaylist() will return nullptr after this call). Use loadPlaylist() to
			 *       rebuild a playlist that is still backed by a named manifest.
			 * @param track A reference to a music resource.
			 * @return void
			 */
			void
			addToPlaylist (const std::shared_ptr< MusicResource > & track) noexcept
			{
				m_playlist.push_back(track);
				m_loadedPlaylist.reset();
			}

			/**
			 * @brief Replaces the current playlist with the content of a PlaylistResource manifest.
			 * @note Semantics:
			 *       - Captures whether playback was active before any mutation.
			 *       - Resolves every track name via the MusicResource container; unknown entries are skipped with a warning.
			 *       - If the manifest is null or resolves to zero valid tracks, the previous playlist is left intact and the call returns false.
			 *       - Otherwise: stops current playback, clears the playlist, refills it from the resolved tracks, and if playback was active
			 *         AND the new playlist is non-empty, starts from index 0 via playIndex(0).
			 *       - Shuffle ordering (if enabled) is regenerated automatically by the next playback start.
			 * @param playlist A shared pointer to the playlist manifest.
			 * @return bool True on successful swap, false on null pointer or empty resolution (no mutation performed).
			 */
			bool loadPlaylist (const std::shared_ptr< PlaylistResource > & playlist) noexcept;

			/**
			 * @brief Removes all soundtracks from the playlist.
			 * @note Also drops the loaded playlist manifest reference — there is no manifest backing an empty playlist.
			 * @return void
			 */
			void
			clearPlaylist ()
			{
				m_playlist.clear();
				m_loadedPlaylist.reset();
			}

			/**
			 * @brief Returns the sorted, deduplicated list of all PlaylistResource names the application can reach.
			 * @note Union of store-backed manifests (`data-stores/MusicPlaylists/` JSON files) and any PlaylistResource
			 *       created at runtime via `Resources::Container::createResource()`. Useful for populating UI
			 *       dropdowns without having to touch the Resources system directly.
			 * @return std::vector< std::string > Sorted playlist names. Empty if the container is unreachable.
			 */
			[[nodiscard]]
			std::vector< std::string > availablePlaylistNames () const noexcept;

			/**
			 * @brief Returns the manifest currently backing the playlist, if any.
			 * @note Returns nullptr when: no manifest was ever loaded, the playlist was cleared, or the current
			 *       content was built ad-hoc via addToPlaylist() (any manual mutation severs the manifest link).
			 *       Callers can query loadedPlaylist()->name() / trackCount() for display purposes.
			 * @return const std::shared_ptr< PlaylistResource > &
			 */
			[[nodiscard]]
			const std::shared_ptr< PlaylistResource > &
			loadedPlaylist () const noexcept
			{
				return m_loadedPlaylist;
			}

			/**
			 * @brief Returns a const reference to the playlist.
			 * @return const std::vector< std::shared_ptr< MusicResource > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< MusicResource > > &
			playlist () const noexcept
			{
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
				return m_musicIndex;
			}

			/**
			 * @brief Finds the first playlist entry whose resource name contains the given substring (case-insensitive).
			 * @note Intended for remote-console and AI-driven workflows where the caller provides an approximate
			 *       title, e.g. "Volcanian" resolves to "Kyrandia2/Volcanian Salesperson". Matching is restricted
			 *       to the currently loaded playlist — not the full MusicResource container.
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
			 * @brief Returns the current playback position in seconds.
			 * @return float
			 */
			[[nodiscard]]
			float currentPosition () const noexcept;

			/**
			 * @brief Returns the duration of the current track in seconds.
			 * @return float
			 */
			[[nodiscard]]
			float currentDuration () const noexcept;

			/**
			 * @brief Seeks to a position in the current track.
			 * @param position The position in seconds.
			 * @return void
			 */
			void seek (float position) const noexcept;

			/**
			 * @brief Enables or disables shuffle mode.
			 * @param state True to enable shuffle, false to disable.
			 * @return void
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
				return m_shuffleEnabled;
			}

			/**
			 * @brief Pauses the music.
			 * @return void
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes the music.
			 * @return void
			 */
			void resume () noexcept;

			/**
			 * @brief Stops the music.
			 * @return void
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

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Enables OpenAL soft for source events.
			 * @return bool
			 */
			bool enableSourceEvents () noexcept;

			/**
			 * @brief Process for fading.
			 * @return void
			 */
			void eventLoop ();

			/**
			 * @brief Fades in a track.
			 * @param track A pointer to the track.
			 * @param step The step of raising or lowering the volume.
			 * @return bool
			 */
			bool fadeIn (Source * track, float step) const noexcept;

			/**
			 * @brief Fades out a track.
			 * @param track A pointer to the track.
			 * @param step The step of raising or lowering the volume.
			 * @return void
			 */
			static void fadeOut (Source * track, float step) noexcept;

			/**
			 * @brief Check the music resource loading.
			 * @param track A reference to a music resource smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkTrackLoading (const std::shared_ptr< MusicResource > & track) noexcept;

			/**
			 * @brief OpenAL source event callback.
			 * @param eventType
			 * @param object
			 * @param param
			 * @param length
			 * @param message
			 * @param userParam
			 * @return void
			 */
			static void eventCallback (ALenum eventType, ALuint object, ALuint param, ALsizei length, const ALchar * message, void * userParam) noexcept;

			/**
			 * @brief Generates a shuffled order for the playlist.
			 * @return void
			 */
			void generateShuffleOrder () noexcept;

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			Manager & m_audioManager;
			std::unique_ptr< Source > m_trackA;
			std::unique_ptr< Source > m_trackB;
			float m_gain{0.0F};
			UserState m_userState{UserState::Stopped};
			PlayMode m_playMode{PlayMode::Loop};
			PlayingTrack m_playingTrack{PlayingTrack::None};
			size_t m_musicIndex{0};
			std::vector< std::shared_ptr< MusicResource > > m_playlist;
			std::shared_ptr< PlaylistResource > m_loadedPlaylist;
			std::shared_ptr< MusicResource > m_loadingTrack;
			std::thread m_eventThread;
			mutable std::mutex m_stateAccess;
			std::condition_variable m_fadeCv;
			std::atomic_bool m_stopThread{false};
			bool m_crossFaderEnabled{false};
			bool m_isFading{false};
			std::atomic_bool m_requestNextTrack{false};
			bool m_shuffleEnabled{false};
			std::vector< size_t > m_shuffleOrder;
			size_t m_shuffleIndex{0};
	};
}
