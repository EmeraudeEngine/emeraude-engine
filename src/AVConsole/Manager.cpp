/*
 * src/AVConsole/Manager.cpp
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

#include "Manager.hpp"

/* Local inclusions. */
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::AVConsole
{
	using namespace Libs;
	using namespace Graphics;

	const std::string Manager::DefaultViewName{"DefaultView"};
	const std::string Manager::DefaultSpeakerName{"DefaultSpeaker"};

	Manager::Manager (const std::string & name, Renderer & graphicsRenderer, Audio::Manager & audioManager) noexcept
		: NameableTrait{name + ClassId},
		Controllable{ClassId},
		m_AVManagers{graphicsRenderer, audioManager}
	{
		/* Console commands bindings. */
		this->bindCommand("listDevices", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			auto deviceType{DeviceType::Both};

			if ( !arguments.empty() )
			{
				const auto argument = arguments[0].asString();

				if ( argument == "video" )
				{
					deviceType = DeviceType::Video;
				}
				else if ( argument == "audio" )
				{
					deviceType = DeviceType::Audio;
				}
				else if ( argument == "both" )
				{
					deviceType = DeviceType::Both;
				}
			}

			outputs.emplace_back(Severity::Info, this->getDeviceList(deviceType));

			return 0;
		}, "Get a list of input/output audio/video devices.");

		this->bindCommand("registerRoute", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() != 3 )
			{
				outputs.emplace_back(Severity::Error, "This method need 3 parameters.");

				return 1;
			}

			const auto type = arguments[0].asString();
			const auto source = arguments[1].asString();
			const auto target = arguments[2].asString();

			if ( type == "video" )
			{
				if ( !this->connectVideoDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the video device.");

					return 3;
				}
			}
			else if ( type == "audio" )
			{
				if ( !this->connectAudioDevices(source, target) )
				{
					outputs.emplace_back(Severity::Error, "Unable to connect the audio device.");

					return 3;
				}
			}
			else
			{
				outputs.emplace_back(Severity::Error, "First parameter must be 'video' or 'audio'.");

				return 2;
			}

			return 0;
		}, "Register a route from input device to output device.");
	}

	bool
	Manager::addVideoDeviceNoLock (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept
	{
		if ( device->deviceType() != DeviceType::Video )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not a video device !";

			return false;
		}

		if ( !m_virtualVideoDevices.contains(device->id()) )
		{
			if ( !m_virtualVideoDevices.emplace(device->id(), device).second )
			{
				TraceWarning{ClassId} << "Unable to register the virtual video device '" << device->id() << "' !";

				return false;
			}

			TraceSuccess{ClassId} << "New virtual video device '" << device->id() << "' available !";

			this->notify(VideoDeviceAdded, device);
		}
		else
		{
			TraceInfo{ClassId} << "Virtual video device '" << device->id() << "' already registered !";
		}

		/* NOTE: Set the device as primary if requested and even if it is already registered. */
		if ( primaryDevice )
		{
			switch ( device->allowedConnexionType() )
			{
				case ConnexionType::Output :
					TraceDebug{ClassId} << "Virtual video device '" << device->id() << "' declared as primary input (I.e. Camera) !";

					m_primaryInputVideoDeviceId = device->id();
					break;

				case ConnexionType::Input :
				case ConnexionType::Both :
					TraceDebug{ClassId} << "Virtual video device '" << device->id() << "' declared as primary output (I.e. Screen) !";

					m_primaryOutputVideoDeviceId = device->id();
					break;
			}
		}

		return true;
	}

	bool
	Manager::addAudioDeviceNoLock (const std::shared_ptr< AbstractVirtualDevice > & device, bool primaryDevice) noexcept
	{
		if ( device->deviceType() != DeviceType::Audio )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not an audio device !";

			return false;
		}

		if ( !m_virtualAudioDevices.contains(device->id()) )
		{
			if ( !m_virtualAudioDevices.emplace(device->id(), device).second )
			{
				TraceWarning{ClassId} << "Unable to register the virtual audio device '" << device->id() << "' !";

				return false;
			}

			TraceSuccess{ClassId} << "New virtual audio device '" << device->id() << "' available !";

			this->notify(AudioDeviceAdded, device);
		}
		else
		{
			TraceInfo{ClassId} << "Virtual audio device '" << device->id() << "' already registered !";
		}

		/* NOTE: Set the device as primary if requested and even if it is already registered. */
		if ( primaryDevice )
		{
			switch ( device->allowedConnexionType() )
			{
				case ConnexionType::Output :
					TraceDebug{ClassId} << "Virtual audio device '" << device->id() << "' declared as primary input (I.e. Microphone) !";

					m_primaryInputAudioDeviceId = device->id();
					break;

				case ConnexionType::Input :
				case ConnexionType::Both :
					TraceDebug{ClassId} << "Virtual audio device '" << device->id() << "' declared as primary output (I.e. Speaker) !";

					m_primaryOutputAudioDeviceId = device->id();
					break;
			}
		}

		return true;
	}

	bool
	Manager::removeVideoDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_deviceAccess};

		if ( device->deviceType() != DeviceType::Video )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not a video device !";

			return false;
		}

		device->disconnectFromAll(m_AVManagers, true);

		if ( m_virtualVideoDevices.erase(device->id()) <= 0 )
		{
			TraceInfo{ClassId} << "There is no virtual video device '" << device->id() << "' registered !";

			return false;
		}

		TraceSuccess{ClassId} << "Virtual video device '" << device->id() << "' removed !";

		this->notify(VideoDeviceRemoved, device);

		return true;
	}

	bool
	Manager::removeAudioDevice (const std::shared_ptr< AbstractVirtualDevice > & device) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_deviceAccess};

		if ( device->deviceType() != DeviceType::Audio )
		{
			TraceWarning{ClassId} << "The virtual device '" << device->id() << "' is not an audio device !";

			return false;
		}

		device->disconnectFromAll(m_AVManagers, true);

		if ( m_virtualAudioDevices.erase(device->id()) <= 0 )
		{
			TraceInfo{ClassId} << "There is no virtual audio device '" << device->id() << "' registered !";

			return false;
		}

		TraceSuccess{ClassId} << "Virtual audio device '" << device->id() << "' removed !";

		this->notify(AudioDeviceRemoved, device);

		return true;
	}

	std::vector< std::shared_ptr< AbstractVirtualDevice > >
	Manager::getVideoDeviceSourcesNoLock () const noexcept
	{
		std::vector< std::shared_ptr< AbstractVirtualDevice > > list{};
		list.reserve(m_virtualVideoDevices.size());

		for ( const auto & device : std::ranges::views::values(m_virtualVideoDevices) )
		{
			if ( device->allowedConnexionType() == ConnexionType::Output )
			{
				list.emplace_back(device);
			}
		}

		return list;
	}

	std::vector< std::shared_ptr< AbstractVirtualDevice > >
	Manager::getAudioDeviceSourcesNoLock () const noexcept
	{
		std::vector< std::shared_ptr< AbstractVirtualDevice > > list{};
		list.reserve(m_virtualAudioDevices.size());

		for ( const auto & device : std::ranges::views::values(m_virtualAudioDevices) )
		{
			if ( device->allowedConnexionType() == ConnexionType::Output )
			{
				list.emplace_back(device);
			}
		}

		return list;
	}

	bool
	Manager::connectVideoDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) noexcept
	{
		std::shared_ptr< AbstractVirtualDevice > sourceDevice;
		std::shared_ptr< AbstractVirtualDevice > targetDevice;

		{
			const std::lock_guard< std::mutex > lock{m_deviceAccess};

			sourceDevice = this->getVideoDeviceNoLock(sourceDeviceId);

			if ( sourceDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to find virtual video device '" << sourceDeviceId << "' as source device to connect !";

				return false;
			}

			targetDevice = this->getVideoDeviceNoLock(targetDeviceId);

			if ( targetDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to find virtual video device '" << targetDeviceId << "' as target device to connect !";

				return false;
			}
		}

		if ( sourceDevice->isConnectedWith(targetDevice, ConnexionType::Output) )
		{
			return true;
		}

		switch ( sourceDevice->connect(m_AVManagers, targetDevice, true) )
		{
			case ConnexionResult::Success :
				TraceSuccess{ClassId} << "The video device '" << sourceDeviceId << "' is connected to '" << targetDeviceId << "' !";

				break;

			case ConnexionResult::Failure :
				TraceError{ClassId} << "Unable to connect video device '" << sourceDeviceId << "' to '" << targetDeviceId << "' !";

				return false;

			case ConnexionResult::DifferentDeviceType :
				TraceError{ClassId} << "The device '" << sourceDeviceId << "' or '" << targetDeviceId << "' is not a video device !";

				return false;

			case ConnexionResult::NotAllowed :
				TraceError{ClassId} << "The device '" << sourceDeviceId << "' and '" << targetDeviceId << "' are not allowed to connect !";

				return false;
		}

		return true;
	}

	bool
	Manager::connectAudioDevices (const std::string & sourceDeviceId, const std::string & targetDeviceId) noexcept
	{
		std::shared_ptr< AbstractVirtualDevice > sourceDevice;
		std::shared_ptr< AbstractVirtualDevice > targetDevice;

		{
			const std::lock_guard< std::mutex > lock{m_deviceAccess};

			sourceDevice = this->getAudioDeviceNoLock(sourceDeviceId);

			if ( sourceDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to find virtual audio device '" << sourceDeviceId << "' as source device to connect !";

				return false;
			}

			targetDevice = this->getAudioDeviceNoLock(targetDeviceId);

			if ( targetDevice == nullptr )
			{
				TraceError{ClassId} << "Unable to find virtual audio device '" << targetDeviceId << "' as target device to connect !";

				return false;
			}
		}

		if ( sourceDevice->isConnectedWith(targetDevice, ConnexionType::Output) )
		{
			return true;
		}

		switch ( sourceDevice->connect(m_AVManagers, targetDevice, true) )
		{
			case ConnexionResult::Success :
				TraceSuccess{ClassId} << "The audio device '" << sourceDeviceId << "' is connected to '" << targetDeviceId << "' !";

				break;

			case ConnexionResult::Failure :
				TraceError{ClassId} << "Unable to connect audio device '" << sourceDeviceId << "' to '" << targetDeviceId << "' !";

				return false;

			case ConnexionResult::DifferentDeviceType :
				TraceError{ClassId} << "The device '" << sourceDeviceId << "' or '" << targetDeviceId << "' is not an audio device !";

				return false;

			case ConnexionResult::NotAllowed :
				TraceError{ClassId} << "The audio device '" << sourceDeviceId << "' and '" << targetDeviceId << "' are not allowed to connect !";

				return false;
		}

		return true;
	}

	bool
	Manager::autoConnectPrimaryVideoDevices () noexcept
	{
		std::string sourceId;
		std::string targetId;

		{
			const std::lock_guard< std::mutex > lock{m_deviceAccess};

			if ( !this->autoSelectPrimaryInputVideoDevice() )
			{
				Tracer::error(ClassId, "There is no input primary video device declared !");

				return false;
			}

			if ( m_primaryOutputVideoDeviceId.empty() )
			{
				Tracer::info(ClassId, "There is no output primary video device declared ! Creating a view ...");

				return false;
			}

			sourceId = m_primaryInputVideoDeviceId;
			targetId = m_primaryOutputVideoDeviceId;
		}

		TraceDebug{ClassId} << "Connecting devices : " << sourceId << " => " << targetId;

		return this->connectVideoDevices(sourceId, targetId);
	}

	bool
	Manager::autoConnectPrimaryAudioDevices () noexcept
	{
		std::string sourceId;
		std::string targetId;

		{
			const std::lock_guard< std::mutex > lock{m_deviceAccess};

			if ( !this->autoSelectPrimaryInputAudioDevice() )
			{
				Tracer::error(ClassId, "There is no input primary audio device declared !");

				return false;
			}

			if ( m_primaryOutputAudioDeviceId.empty() )
			{
				Tracer::info(ClassId, "There is no output primary audio device declared ! Creating a speaker ...");

				return false;
			}

			sourceId = m_primaryInputAudioDeviceId;
			targetId = m_primaryOutputAudioDeviceId;
		}

		TraceDebug{ClassId} << "Connecting devices : " << sourceId << " => " << targetId;

		return this->connectAudioDevices(sourceId, targetId);
	}

	std::string
	Manager::getConnexionStates () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_deviceAccess};

		std::stringstream string;

		string << "Video routes :" "\n";

		for ( const auto & device : this->getVideoDeviceSourcesNoLock() )
		{
			string << device->getConnexionState();
		}

		string << "Audio routes :" "\n";

		for ( const auto & device : this->getAudioDeviceSourcesNoLock() )
		{
			string << device->getConnexionState();
		}

		return string.str();
	}

	std::string
	Manager::getDeviceList (DeviceType deviceType) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_deviceAccess};

		std::stringstream string;

		if ( deviceType == DeviceType::Video || deviceType == DeviceType::Both )
		{
			{
				size_t count = 0;

				string << "Video input devices :" "\n";

				for ( const auto & [name, device] : m_virtualVideoDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Output || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}

			{
				size_t count = 0;

				string << "Video output devices :" "\n";

				for ( const auto & [name, device] : m_virtualVideoDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Input || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}
		}

		if ( deviceType == DeviceType::Audio || deviceType == DeviceType::Both )
		{
			{
				size_t count = 0;

				string << "Audio input devices :" "\n";

				for ( const auto & [name, device] : m_virtualAudioDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Output || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}

			{
				size_t count = 0;

				string << "Audio output devices :" "\n";

				for ( const auto & [name, device] : m_virtualAudioDevices )
				{
					const auto deviceConnexionType = device->allowedConnexionType();

					if ( deviceConnexionType == ConnexionType::Input || deviceConnexionType == ConnexionType::Both )
					{
						string << " - '" << name << "'" "\n";

						count++;
					}
				}

				if ( count == 0 )
				{
					string << " None !" "\n";
				}
			}
		}

		return string.str();
	}

	void
	Manager::clear () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_deviceAccess};

		/* NOTE: Clearing the primary device names. */
		m_primaryOutputAudioDeviceId.clear();
		m_primaryInputAudioDeviceId.clear();
		m_primaryOutputVideoDeviceId.clear();
		m_primaryInputVideoDeviceId.clear();

		m_virtualVideoDevices.clear();
		m_virtualAudioDevices.clear();
	}

	bool
	Manager::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	void
	Manager::onRegisterToConsole () noexcept
	{

	}

	bool
	Manager::autoSelectPrimaryInputVideoDevice () noexcept
	{
		if ( m_primaryInputVideoDeviceId.empty() )
		{
			const auto selectedDeviceIt = std::ranges::find_if(m_virtualVideoDevices, [] (const auto & deviceIt) {
				return deviceIt.second->allowedConnexionType() == ConnexionType::Output;
			});

			if ( selectedDeviceIt == m_virtualVideoDevices.cend() )
			{
				return false;
			}

			m_primaryInputVideoDeviceId = selectedDeviceIt->first;
		}

		return true;
	}

	bool
	Manager::autoSelectPrimaryInputAudioDevice () noexcept
	{
		if ( m_primaryInputAudioDeviceId.empty() )
		{
			const auto selectedDeviceIt = std::ranges::find_if(m_virtualAudioDevices, [] (const auto & deviceIt) {
				return deviceIt.second->allowedConnexionType() == ConnexionType::Output;
			});

			if ( selectedDeviceIt == m_virtualAudioDevices.cend() )
			{
				return false;
			}

			m_primaryInputAudioDeviceId = selectedDeviceIt->first;
		}

		return true;
	}
}
