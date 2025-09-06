/*
 * src/Audio/TrackMixer.hpp
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

#pragma once

/* STL inclusions. */
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/Controllable.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "MusicResource.hpp"
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
	 * @extends EmEn::Console::Controllable The console can control the track mixer.
	 * @extends EmEn::Libs::ObservableTrait This service is observable.
	 * @extends EmEn::Libs::ObserverTrait This service can observe resource loading.
	 */
	class TrackMixer final : public ServiceInterface, public Console::Controllable, public Libs::ObservableTrait, public Libs::ObserverTrait
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
				Controllable{ClassId},
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
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
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

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_serviceInitialized;
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
			 * @brief Enables the cross-fader
			 * @param state The state.
			 * @return void
			 */
			void
			enableCrossFader (bool state) noexcept
			{
				m_crossFaderEnabled = state;
			}

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
			 * @brief Adds a soundtrack to the playlist.
			 * @param track A reference to a music resource.
			 * @return void
			 */
			void
			addToPlaylist (const std::shared_ptr< MusicResource > & track) noexcept
			{
				m_playlist.push_back(track);
			}

			/**
			 * @brief Removes all soundtracks from the playlist.
			 * @return void
			 */
			void
			clearPlaylist ()
			{
				m_playlist.clear();
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

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
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
			std::shared_ptr< MusicResource > m_loadingTrack;
			std::thread m_eventThread;
			mutable std::mutex m_stateAccess;
			std::condition_variable m_fadeCv;
			std::atomic_bool m_stopThread{false};
			bool m_serviceInitialized{false};
			bool m_crossFaderEnabled{false};
			bool m_isFading{false};
			std::atomic_bool m_requestNextTrack{false};
	};
}
