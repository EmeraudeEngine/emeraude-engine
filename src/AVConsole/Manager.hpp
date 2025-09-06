/*
 * src/AVConsole/Manager.hpp
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
#include <cstddef>
#include <cstdint>
#include <any>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Console/Controllable.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "AbstractVirtualDevice.hpp"
#include "Types.hpp"

namespace EmEn
{
	class Settings;
}

namespace EmEn::AVConsole
{
	/**
	 * @brief The audio/video manager links every virtual audio/video input/output from a scene.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::Libs::NameableTrait The audio/video manager can have a name according to a scene.
	 * @extends EmEn::Console::Controllable The audio/video manager is usable from the console.
	 * @extends EmEn::Libs::ObserverTrait The audio/video manager wants to get notifications from devices.
	 * @extends EmEn::Libs::ObserverTrait The audio/video manager dispatches device configuration changes.
	 */
	class Manager final : public Libs::NameableTrait, public Console::Controllable, public Libs::ObserverTrait, public Libs::ObservableTrait
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				VideoDeviceAdded,
				VideoDeviceRemoved,
				AudioDeviceAdded,
				AudioDeviceRemoved,
				RenderToShadowMapAdded,
				RenderToTextureAdded,
				RenderToViewAdded,
				/* Enumeration boundary. */
				MaxEnum
			};


			/** @brief Observable class identification. */
			static constexpr auto ClassId{"AVConsole"};

			/** @brief The reserved name for the default view device. */
			static const std::string DefaultViewName;
			static const std::string DefaultSpeakerName;

			/**
			 * @brief Constructs the audio/video manager.
			 * @param name A reference to a string.
			 * @param graphicsRenderer A reference to the graphics renderer.
			 * @param audioManager A reference to the audio manager.
			 */
			Manager (const std::string & name, Graphics::Renderer & graphicsRenderer, Audio::Manager & audioManager) noexcept;

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
			 * @brief Shares the audio video managers.
			 * @return AVManagers &
			 */
			[[nodiscard]]
			AVManagers &
			managers () noexcept
			{
				return m_AVManagers;
			}

			/**
			 * @brief Shares the graphics renderer service.
			 * @return Graphics::Renderer &
			 */
			[[nodiscard]]
			Graphics::Renderer &
			graphicsRenderer () const noexcept
			{
				return m_AVManagers.graphicsRenderer;
			}

			/**
			 * @brief Shares the audio manager service.
			 * @return Audio::Manager &
			 */
			[[nodiscard]]
			Audio::Manager &
			audioManager () const noexcept
			{
				return m_AVManagers.audioManager;
			}

			/**
			 * @brief Returns whether a virtual video device exists.
			 * @param deviceId A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVideoDeviceExists (const std::string & deviceId) const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return m_virtualVideoDevices.contains(deviceId);
			}

			/**
			 * @brief Returns whether a virtual audio device exists.
			 * @param deviceId A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAudioDeviceExists (const std::string & deviceId) const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return m_virtualAudioDevices.contains(deviceId);
			}

			/**
			 * @brief Returns whether a primary video output is set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPrimaryVideoOutput () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return !m_primaryOutputVideoDeviceId.empty();
			}

			/**
			 * @brief Returns whether a primary audio output is set.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPrimaryAudioOutput () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return !m_primaryOutputAudioDeviceId.empty();
			}

			/**
			 * @brief Returns a video device by its name.
			 * @param deviceId A reference to a string.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< AbstractVirtualDevice >
			getVideoDevice (const std::string & deviceId) const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->getVideoDeviceNoLock(deviceId);
			}

			/**
			 * @brief Returns an audio device by its name.
			 * @param deviceId A reference to a string.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< AbstractVirtualDevice >
			getAudioDevice (const std::string & deviceId) const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->getAudioDeviceNoLock(deviceId);
			}

			/**
			 * @brief Returns a list of video sources.
			 * @return std::vector< std::shared_ptr< AbstractVirtualDevice > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< AbstractVirtualDevice > >
			getVideoDeviceSources () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->getVideoDeviceSourcesNoLock();
			}

			/**
			 * @brief Returns a list of audio sources.
			 * @return std::vector< std::shared_ptr< AbstractVirtualDevice > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< AbstractVirtualDevice > >
			getAudioDeviceSources () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->getAudioDeviceSourcesNoLock();
			}

			/**
			 * @brief Returns the primary video device.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< AbstractVirtualDevice >
			getPrimaryVideoDevice () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				if ( m_primaryOutputVideoDeviceId.empty() )
				{
					return nullptr;
				}

				return this->getVideoDeviceNoLock(m_primaryOutputVideoDeviceId);
			}

			/**
			 * @brief Returns the primary audio device.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			[[nodiscard]]
			std::shared_ptr< AbstractVirtualDevice >
			getPrimaryAudioDevice () const noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				if ( m_primaryOutputAudioDeviceId.empty() )
				{
					return nullptr;
				}

				return this->getAudioDeviceNoLock(m_primaryOutputAudioDeviceId);
			}

			/**
			 * @brief Adds a virtual video device.
			 * @param device A reference to a virtual video device smart pointer.
			 * @param primaryDevice Set the device as primary for its connexion type. Default false.
			 * @return bool
			 */
			bool
			addVideoDevice (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice = false) noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->addVideoDeviceNoLock(device, primaryDevice);
			}

			/**
			 * @brief Adds a virtual audio device.
			 * @param device A reference to a virtual audio device smart pointer.
			 * @param primaryDevice Set the device as primary for its connexion type. Default false.
			 * @return bool
			 */
			bool
			addAudioDevice (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice = false) noexcept
			{
				const std::lock_guard< std::mutex > lock{m_deviceAccess};

				return this->addAudioDeviceNoLock(device, primaryDevice);
			}

			/**
			 * @brief Removes a virtual video device.
			 * @param device A reference to a virtual video device smart pointer.
			 * @return bool
			 */
			bool removeVideoDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept;

			/**
			 * @brief Removes a virtual audio device.
			 * @param device A reference to a virtual audio device smart pointer.
			 * @return bool
			 */
			bool removeAudioDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept;

			/**
			 * @brief Connects two video devices.
			 * @param sourceDeviceId A reference to a string of an output virtual video device id.
			 * @param targetDeviceId A reference to a string of an input virtual video device id.
			 * @return bool
			 */
			bool connectVideoDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) noexcept;

			/**
			 * @brief Connects two audio devices.
			 * @param sourceDeviceId A reference to a string of an output virtual audio device id.
			 * @param targetDeviceId A reference to a string of an input virtual audio device id.
			 * @return bool
			 */
			bool connectAudioDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) noexcept;

			/**
			 * @brief Auto-connects the primary video devices.
			 * @return bool
			 */
			bool autoConnectPrimaryVideoDevices () noexcept;

			/**
			 * @brief Auto-connects the primary audio devices.
			 * @return bool
			 */
			bool autoConnectPrimaryAudioDevices () noexcept;

			/**
			 * @brief Returns a printable device List.
			 * @param deviceType The type of devices. Default both.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getDeviceList (DeviceType deviceType = DeviceType::Both) const noexcept;

			/**
			 * @brief Returns a printable state of connexions.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getConnexionStates () const noexcept;

			/**
			 * @brief Clears all devices from the console.
			 * @return void
			 */
			void clear () noexcept;

		private:

			/** @copydoc EmEn::Libs:ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Adds a virtual video device.
			 * @note This method doesn't lock the access mutex.
			 * @param device A reference to a virtual video device smart pointer.
			 * @param primaryDevice Set the device as primary for its connexion type.
			 * @return bool
			 */
			bool addVideoDeviceNoLock (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept;

			/**
			 * @brief Adds a virtual audio device.
			 * @note This method doesn't lock the access mutex.
			 * @param device A reference to a virtual audio device smart pointer.
			 * @param primaryDevice Set the device as primary for its connexion type.
			 * @return bool
			 */
			bool addAudioDeviceNoLock (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept;

			/**
			 * @brief Returns a video device by its name.
			 * @note This method doesn't lock the access mutex.
			 * @param deviceId A reference to a string.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			std::shared_ptr< AbstractVirtualDevice >
			getVideoDeviceNoLock (const std::string & deviceId) const noexcept
			{
				const auto deviceIt = m_virtualVideoDevices.find(deviceId);

				if ( deviceIt == m_virtualVideoDevices.cend() )
				{
					return nullptr;
				}

				return deviceIt->second;
			}

			/**
			 * @brief Returns an audio device by its name.
			 * @note This method doesn't lock the access mutex.
			 * @param deviceId A reference to a string.
			 * @return std::shared_ptr< AbstractVirtualDevice >
			 */
			std::shared_ptr< AbstractVirtualDevice >
			getAudioDeviceNoLock (const std::string & deviceId) const noexcept
			{
				const auto deviceIt = m_virtualAudioDevices.find(deviceId);

				if ( deviceIt == m_virtualAudioDevices.cend() )
				{
					return nullptr;
				}

				return deviceIt->second;
			}

			/**
			 * @brief Returns a list of video sources.
			 * @note This method doesn't lock the access mutex.
			 * @return std::vector< std::shared_ptr< AbstractVirtualDevice > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< AbstractVirtualDevice > > getVideoDeviceSourcesNoLock () const noexcept;

			/**
			 * @brief Returns a list of audio sources.
			 * @note This method doesn't lock the access mutex.
			 * @return std::vector< std::shared_ptr< AbstractVirtualDevice > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< AbstractVirtualDevice > > getAudioDeviceSourcesNoLock () const noexcept;

			/**
			 * @brief Automatically selects a primary input video device.
			 * @note This method doesn't lock the access mutex.
			 * @note If a primary device is already selected, the function won't change anything.
			 * @return bool
			 */
			bool autoSelectPrimaryInputVideoDevice () noexcept;

			/**
			 * @brief Automatically selects a primary input audio device.
			 * @note This method doesn't lock the access mutex.
			 * @note If a primary device is already selected, the function won't change anything.
			 * @return bool
			 */
			bool autoSelectPrimaryInputAudioDevice () noexcept;

			AVManagers m_AVManagers;
			std::unordered_map< std::string, std::shared_ptr< AbstractVirtualDevice > > m_virtualVideoDevices;
			std::unordered_map< std::string, std::shared_ptr< AbstractVirtualDevice > > m_virtualAudioDevices;
			std::string m_primaryInputVideoDeviceId; /* Like a camera. */
			std::string m_primaryOutputVideoDeviceId; /* Like a view. */
			std::string m_primaryInputAudioDeviceId; /* Like a microphone. */
			std::string m_primaryOutputAudioDeviceId; /* Like a speaker. */
			mutable std::mutex m_deviceAccess;
	};
}
