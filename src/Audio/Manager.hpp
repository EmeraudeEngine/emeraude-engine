/*
 * src/Audio/Manager.hpp
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
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "OpenALExtensions.hpp"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Console/Controllable.hpp"

/* Local inclusions for usages. */
#include "Libs/WaveFactory/Types.hpp"
#include "Audio/TrackMixer.hpp"
#include "Audio/AudioRecorder.hpp"
#include "SettingKeys.hpp"
#include "Source.hpp"
#include "SoundEnvironmentProperties.hpp"
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class Manager;
	}

	class PrimaryServices;
}

namespace EmEn::Audio
{
	/**
	 * @brief The audio manager service class.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Libs::ObservableTrait This service is observable.
	 * @extends EmEn::Console::Controllable The console can control the audio manager.
	 */
	class Manager final : public ServiceInterface, public Libs::ObservableTrait, public Console::Controllable
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioManagerService"};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				SpeakerCreated,
				SpeakerDestroyed,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs an audio manager.
			 * @param primaryServices A reference to primary services.
			 * @param resourceManager A reference to the resource manager.
			 */
			Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager) noexcept
				: ServiceInterface{ClassId},
				Controllable{ClassId},
				m_primaryServices{primaryServices},
				m_resourceManager{resourceManager}
			{

			}

			/**
			 * @brief Destructs the audio manager.
			 */
			~Manager () override = default;

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

			/**
			 * @brief Returns the reference to the track mixer service.
			 * @return TrackMixer &
			 */
			[[nodiscard]]
			TrackMixer &
			trackMixer () noexcept
			{
				return m_trackMixer;
			}

			/**
			 * @brief Returns the reference to the track mixer service.
			 * @return const TrackMixer &
			 */
			[[nodiscard]]
			const TrackMixer &
			trackMixer () const noexcept
			{
				return m_trackMixer;
			}

			/**
			 * @brief Returns the reference to the audio external input service.
			 * @return AudioRecorder &
			 */
			[[nodiscard]]
			AudioRecorder &
			audioRecorder () noexcept
			{
				return m_audioRecorder;
			}

			/**
			 * @brief Returns the reference to the audio external input service.
			 * @return const AudioRecorder &
			 */
			[[nodiscard]]
			const AudioRecorder &
			audioRecorder () const noexcept
			{
				return m_audioRecorder;
			}

			/**
			 * @brief Sets the main state of the audio manager.
			 * @note If the audio has been disabled at startup, this method will have no effect.
			 * @param state The state.
			 */
			void enableAudio (bool state) noexcept;

			/**
			 * @brief Returns a list a available output audio devices.
			 * @return bool
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			availableOutputDevices () const noexcept
			{
				return m_availableOutputDevices;
			}

			/**
			 * @brief Returns a list a available input audio devices.
			 * @return bool
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			availableInputDevices () const noexcept
			{
				return m_availableInputDevices;
			}

			/**
			 * @brief Plays a sound on the default source.
			 * @param playable A reference to a playable interface smart pointer.
			 * @param mode The mode. Default Once.
			 * @param gain The gain of the channel to play the sound.
			 */
			void play (const std::shared_ptr< PlayableInterface > & playable, PlayMode mode = PlayMode::Once, float gain = 1.0F) const noexcept;

			/**
			 * @brief Plays a sound on the default source.
			 * @param resourceName A reference to a string for a resource.
			 * @param mode The play mode. Default Once.
			 * @param gain The gain of the channel to play the sound.
			 */
			void play (const std::string & resourceName, PlayMode mode = PlayMode::Once, float gain = 1.0F) const noexcept;

			/**
			 * @brief Returns the API (OpenAL) information.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getAPIInformation () const noexcept;

			/**
			 * @brief Sets the main level.
			 * @param gain The gain from 0.0 to 1.0.
			 */
			void setMainLevel (float gain) noexcept;

			/**
			 * @brief Returns the  main level.
			 * @return float
			 */
			[[nodiscard]]
			float mainLevel () const noexcept;

			/**
			 * @brief Changes the sound properties.
			 * @param properties A reference to a sound environment property structure.
			 * @return void
			 */
			void setSoundEnvironmentProperties (const SoundEnvironmentProperties & properties) noexcept;

			/**
			 * @brief Returns the actual sound properties.
			 * @return SoundEnvironmentProperties
			 */
			[[nodiscard]]
			SoundEnvironmentProperties getSoundEnvironmentProperties () const noexcept;

			/**
			 * @brief Sets the listener properties.
			 * @param properties An array of parameters. The first 3 floats are for position,
			 * the next 3 floats are for orientation, and the 3 last for velocity.
			 */
			void setListenerProperties (const std::array< ALfloat, 12 > & properties) noexcept;

			/**
			 * @brief Returns the listener properties by reference.
			 * @param properties An array of parameters. The first 3 floats are for position,
			 * the next 3 floats are for orientation, and the 3 last for velocity.
			 */
			void listenerProperties (std::array< ALfloat, 12 > & properties) const noexcept;

			/**
			 * @brief Sets meters per unit.
			 * @note Requires EFX extension.
			 * @param meters The speed in meters par unit.
			 */
			void setMetersPerUnit (float meters) noexcept;

			/**
			 * @brief Returns meters per unit.
			 * @note Requires EFX extension.
			 * @return float
			 */
			[[nodiscard]]
			float metersPerUnit () const noexcept;

			/**
			 * @brief Returns the ALC version as a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getALCVersionString () const noexcept;

			/**
			 * @brief Returns the EFX version as a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getEFXVersionString () const noexcept;

			/**
			 * @brief Returns the number of available audio sources.
			 * @return size_t.
			 */
			[[nodiscard]]
			size_t getAvailableSourceCount () const noexcept;

			/**
			 * @brief Returns an available audio source.
			 * @return SourceRequest
			 */
			[[nodiscard]]
			SourceRequest requestSource () noexcept;

			/**
			 * @brief Returns the playback frequency.
			 * @return Frequency
			 */
			[[nodiscard]]
			static
			Libs::WaveFactory::Frequency
			frequencyPlayback () noexcept
			{
				return s_playbackFrequency;
			}

			/**
			 * @brief Returns the record frequency.
			 * @return Frequency
			 */
			[[nodiscard]]
			static
			Libs::WaveFactory::Frequency
			recordFrequency () noexcept
			{
				return s_recordFrequency;
			}

			/**
			 * @brief Returns the music chunk size for streaming.
			 * @return size_t
			 */
			[[nodiscard]]
			static
			size_t
			musicChunkSize () noexcept
			{
				return s_musicChunkSize;
			}

			/**
			 * @brief Returns whether the audio layer is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			isAudioSystemAvailable () noexcept
			{
				return s_audioSystemAvailable;
			}

			/**
			 * @brief Returns the main state of the audio manager.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			isAudioEnabled () noexcept
			{
				return s_audioEnabled;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Queries the available basic audio device and save it.
			 * @param useExtendedAPI Use the extended API version of querying audio devices.
			 * @return bool
			 */
			bool queryOutputDevices (bool useExtendedAPI) noexcept;

			/**
			 * @brief Queries the available basic audio device and save it.
			 * @return bool
			 */
			bool queryInputDevices () noexcept;

			/**
			 * @brief Selects an output audio device to play sound and create a context with it.
			 * @return bool
			 */
			[[nodiscard]]
			bool setupAudioOutputDevice () noexcept;

			/**
			 * @brief Selects an input audio device to record sound.
			 * @return bool
			 */
			[[nodiscard]]
			bool setupAudioInputDevice () noexcept;

			/**
			 * @brief Release an unused source.
			 * @return void
			 */
			void releaseSource (Source * source) noexcept;

			/**
			 * @brief Sets the doppler effect factor.
			 * @param dopplerFactor
			 */
			void setDopplerFactor (float dopplerFactor) noexcept;

			/**
			 * @brief Returns the current doppler effect factor.
			 * @return float
			 */
			[[nodiscard]]
			float dopplerFactor () const noexcept;

			/**
			 * @brief Sets the speed of sound.
			 * @param speed The value in unit per second.
			 */
			void setSpeedOfSound (float speed) noexcept;

			/**
			 * @brief Returns the current speed of sound.
			 * @return float
			 */
			[[nodiscard]]
			float speedOfSound () const noexcept;

			/**
			 * @brief Sets the distance model for the sound attenuation.
			 * @param model One of the DistanceModel enum values.
			 */
			void setDistanceModel (DistanceModel model) noexcept;

			/**
			 * @brief Returns the current distance model in use for the sound attenuation.
			 * @return DistanceModel
			 */
			[[nodiscard]]
			DistanceModel distanceModel () const noexcept;

			/**
			 * @brief Returns the device attributes.
			 * @return std::vector< ALCint >
			 */
			[[nodiscard]]
			std::vector< ALCint > getDeviceAttributes () const noexcept;

			/**
			 * @brief Gets and save a copy of the context attributes into the manager.
			 * @return bool
			 */
			bool saveContextAttributes () noexcept;

			static inline Libs::WaveFactory::Frequency s_playbackFrequency{Libs::WaveFactory::Frequency::PCM48000Hz};
			static inline Libs::WaveFactory::Frequency s_recordFrequency{Libs::WaveFactory::Frequency::PCM48000Hz};
			static inline size_t s_musicChunkSize{DefaultAudioMusicChunkSize};

			/** @brief No audio device found to play sound. */
			static inline bool s_audioSystemAvailable{false};
			/** @brief No audio capture device found to record sound. */
			static inline bool s_audioCaptureAvailable{false};
			/** @brief Dynamic switch for audio playback. Even if the audio system is available. */
			static inline bool s_audioEnabled{false};

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			TrackMixer m_trackMixer{m_primaryServices, m_resourceManager, *this};
			AudioRecorder m_audioRecorder;
			std::vector< std::string > m_availableOutputDevices;
			std::string m_selectedOutputDeviceName;
			std::vector< std::string > m_availableInputDevices;
			std::string m_selectedInputDeviceName;
			ALCdevice * m_outputDevice{nullptr};
			ALCdevice * m_inputDevice{nullptr};
			ALCcontext * m_context{nullptr};
			std::map< ALCint, ALCint > m_contextAttributes;
			std::shared_ptr< Source > m_defaultSource;
			std::vector< std::shared_ptr< Source > > m_allSources;
			std::vector< Source * > m_availableSources;
			mutable std::mutex m_sourcePoolMutex;
			bool m_showInformation{false};
			bool m_usingAdvancedEnumeration{false};
	};
}
