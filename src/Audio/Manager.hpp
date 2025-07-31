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
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "OpenAL.EFX.hpp"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/Controllable.hpp"

/* Local inclusions for usages. */
#include "Libs/WaveFactory/Types.hpp"
#include "SettingKeys.hpp"
#include "Source.hpp" // FIXME
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
	/** @brief The playing mode enumeration. */
	enum class PlayMode
	{
		Once,
		Loop
	};

	/**
	 * @brief The audio manager service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Console::Controllable The audio manager can be controlled by the console.
	 */
	class Manager final : public ServiceInterface, public Console::Controllable
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioManagerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

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
			Manager (PrimaryServices & primaryServices, Resources::Manager & resourceManager) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (Manager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Manager & operator= (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Manager & operator= (Manager && copy) noexcept = delete;

			/**
			 * @brief Destructs the audio manager.
			 */
			~Manager () override;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

			/**
			 * @brief Sets the main state of the audio manager.
			 * @note If the audio has been disabled at startup, this method will have no effect.
			 * @param state The state.
			 */
			void enableAudio (bool state) noexcept;

			/**
			 * @brief Returns the main state of the audio manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAudioEnabled () const noexcept
			{
				return m_flags[Enabled];
			}

			/**
			 * @brief Returns the EFX extension checker.
			 * @return std::shared_ptr< EFX >
			 */
			[[nodiscard]]
			std::shared_ptr< EFX >
			getEFX () noexcept
			{
				return m_EFX;
			}

			/**
			 * @brief Returns the default Source.
			 * @return std::shared_ptr< Source >
			 */
			[[nodiscard]]
			std::shared_ptr< Source >
			defaultSource () const noexcept
			{
				return m_defaultSource;
			}

			/**
			 * @brief Plays a sound on the default source.
			 * @param playable A reference to a playable interface smart pointer.
			 * @param mode The play mode. Default Once.
			 * @param gain The gain of the channel to play the sound.
			 */
			void
			play (const std::shared_ptr< PlayableInterface > & playable, Source::PlayMode mode = Source::PlayMode::Once, float gain = 1.0F) const noexcept
			{
				m_defaultSource->setGain(gain);
				m_defaultSource->play(playable, mode);
			}

			/**
			 * @brief Plays a sound on the default source.
			 * @param resourceName A reference to a string for a resource.
			 * @param mode The play mode. Default Once.
			 * @param gain The gain of the channel to play the sound.
			 */
			void play (const std::string & resourceName, Source::PlayMode mode = Source::PlayMode::Once, float gain = 1.0F) const noexcept;

			/**
			 * @brief Returns the API (OpenAL) information.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getAPIInformation () const noexcept;

			/**
			 * @brief Returns the frequency playback.
			 * @return Frequency
			 */
			[[nodiscard]]
			Libs::WaveFactory::Frequency
			frequencyPlayback () const noexcept
			{
				return m_playbackFrequency;
			}

			/**
			 * @brief Returns the music chunk size for streaming.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			musicChunkSize () const noexcept
			{
				return m_musicChunkSize;
			}

			/**
			 * @brief Sets the master volume.
			 * @param gain The gain from 0.0 to 1.0.
			 */
			void setMasterVolume (float gain) noexcept;

			/**
			 * @brief Returns the master volume.
			 * @return float
			 */
			[[nodiscard]]
			float masterVolume () noexcept;

			/**
			 * @brief Changes the sound properties.
			 * @param properties A reference to a sound environment properties.
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
			 * the next 3 floats are for orientation and the 3 lasts for velocity.
			 */
			void setListenerProperties (const std::array< ALfloat, 12 > & properties) noexcept;

			/**
			 * @brief Returns the listener properties by reference.
			 * @param properties An array of parameters. The first 3 floats are for position,
			 * the next 3 floats are for orientation and the 3 lasts for velocity.
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
			 * @brief Returns the number of available audio source.
			 * @return size_t.
			 */
			[[nodiscard]]
			size_t getAvailableSourceCount () const noexcept;

			/**
			 * @brief Returns an available audio source.
			 * @return std::shared_ptr< Source >
			 */
			[[nodiscard]]
			std::shared_ptr< Source > requestSource () const noexcept;

			/**
			 * @brief Returns the instance of the audio manager.
			 * @FIXME Remove this !
			 * @return Manager *
			 */
			[[nodiscard]]
			static
			Manager *
			instance () noexcept
			{
				return s_instance;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

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
			 * @brief Returns the split device name from the raw API data.
			 * @param list The raw list from OpenAL.
			 * @return std::vector< std::string >
			 */
			[[nodiscard]]
			static std::vector< std::string > getDeviceName (const ALCchar * list) noexcept;

			/**
			 * @brief Queries the available device and save it.
			 */
			void queryDevices () noexcept;

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

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ShowInformation{1UL};
			static constexpr auto AudioDisabledAtStartup{2UL};
			static constexpr auto Enabled{3UL};

			static Manager * s_instance;

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourceManager;
			ALCdevice * m_device{nullptr};
			ALCcontext * m_context{nullptr};
			std::vector< std::string > m_availableAudioDevices;
			std::shared_ptr< EFX > m_EFX;
			std::map< ALCint, ALCint > m_contextAttributes;
			std::shared_ptr< Source > m_defaultSource;
			std::vector< std::shared_ptr< Source > > m_sources;
			Libs::WaveFactory::Frequency m_playbackFrequency{Libs::WaveFactory::Frequency::PCM22050Hz};
			size_t m_musicChunkSize{DefaultAudioMusicChunkSize};
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*ShowInformation*/,
				false/*AudioDisabledAtStartup*/,
				false/*Enabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
