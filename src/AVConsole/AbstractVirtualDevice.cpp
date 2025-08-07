/*
 * src/AVConsole/AbstractVirtualDevice.cpp
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

#include "AbstractVirtualDevice.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::AVConsole
{
	using namespace Libs;

	constexpr auto TracerTag{"VirtualDevice"};

	std::atomic< size_t > AbstractVirtualDevice::s_deviceCount{0};

	ConnexionResult
	AbstractVirtualDevice::connect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept
	{
		if ( const auto result = this->canConnect(targetDevice); result != ConnexionResult::Success )
		{
			return result;
		}

		{
			const std::scoped_lock lock{m_IOAccess, targetDevice->m_IOAccess};

			/* NOTE: Performs connexion from the other input list device first. */
			if ( !targetDevice->m_inputDevicesConnected.emplace(this->shared_from_this()).second )
			{
				return ConnexionResult::Failure;
			}

			/* NOTE: Connecting on this device output list. */
			m_outputDevicesConnected.emplace(targetDevice);
		}

		if ( fireEvents )
		{
			targetDevice->onInputDeviceConnected(managers, this);

			this->onOutputDeviceConnected(managers, targetDevice.get());
		}

		return ConnexionResult::Success;
	}

	ConnexionResult
	AbstractVirtualDevice::interconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & intermediateDevice, bool fireEvents) noexcept
	{
		/* 1. Check if connexion is allowed (intermediate must be Both). */
		if ( intermediateDevice->allowedConnexionType() != ConnexionType::Both )
		{
			TraceError{TracerTag} << "The virtual device '" << intermediateDevice->id() << "' must allow input/output to perform an interconnection !";

			return ConnexionResult::NotAllowed;
		}

		/* 2. Check if there is output! */
		if ( m_outputDevicesConnected.empty() )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' has no existing output connexion !";

			return ConnexionResult::Failure;
		}

		for ( const auto & outputDeviceWeak : m_outputDevicesConnected )
		{
			const auto outputDevice = outputDeviceWeak.lock();

			if ( outputDevice == nullptr )
			{
				continue;
			}

			/* 3. Disconnect direct link between devices. */
			if ( const auto result = this->disconnect(managers, outputDevice, fireEvents); result != ConnexionResult::Success )
			{
				continue;
			}

			/* 4. Connect the intermediate device between the disconnected devices. */
			if ( const auto result = this->connect(managers, intermediateDevice, true); result != ConnexionResult::Success )
			{
				return result;
			}

			if ( const auto result = intermediateDevice->connect(managers, outputDevice, true); result != ConnexionResult::Success )
			{
				return result;
			}

			// TODO: Here, we need to handle failure. What if one of the connections fails?
			// TODO: Leave the system in a "broken" state or try to roll back?
			// TODO: For now, we just return false.
		}

		return ConnexionResult::Success;
	}

	ConnexionResult
	AbstractVirtualDevice::interconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & intermediateDevice, const std::string & outputDeviceName, bool fireEvents) noexcept
	{
		/* 1. Check the existence of the device inside outputs. */
		if ( m_outputDevicesConnected.empty() )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' has no existing output connexion !";

			return ConnexionResult::Failure;
		}

		const auto outputDeviceIt = std::ranges::find_if(m_outputDevicesConnected, [&outputDeviceName] (const auto & deviceWeak) {
			const auto device = deviceWeak.lock();

			if ( device == nullptr)
			{
				return false;
			}

			return outputDeviceName.empty() || outputDeviceName == device->id();
		});

		if ( outputDeviceIt == m_outputDevicesConnected.cend() )
		{
			TraceError{TracerTag} << "There is no output virtual device named '" << outputDeviceName << "' !";

			return ConnexionResult::Failure;
		}

		/* 2. Check if connexion is allowed. */
		if ( intermediateDevice->allowedConnexionType() != ConnexionType::Both )
		{
			TraceError{TracerTag} << "The virtual device '" << intermediateDevice->id() << "' must allow input/output to perform an interconnection !";

			return ConnexionResult::NotAllowed;
		}

		/* 3. Disconnect the direct link between the devices. */
		if ( const auto result = this->disconnect(managers, outputDeviceIt->lock(), fireEvents); result != ConnexionResult::Success )
		{
			return result;
		}

		/* 4. Connect the intermediate device between the disconnected devices. */
		if ( const auto result = this->connect(managers, intermediateDevice, fireEvents); result != ConnexionResult::Success )
		{
			return result;
		}

		if ( const auto result = intermediateDevice->connect(managers, outputDeviceIt->lock(), fireEvents); result != ConnexionResult::Success )
		{
			return result;
		}

		return ConnexionResult::Success;
	}

	ConnexionResult
	AbstractVirtualDevice::disconnect (AVManagers & managers, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept
	{
		{
			const std::scoped_lock lock{m_IOAccess, targetDevice->m_IOAccess};

			/* NOTE: Performs disconnection from the other input list device first. */
			if ( targetDevice->m_inputDevicesConnected.erase(this->shared_from_this()) == 0 )
			{
				return ConnexionResult::Failure;
			}

			/* NOTE: Disconnecting on this device output list. */
			m_outputDevicesConnected.erase(targetDevice);
		}

		if ( fireEvents )
		{
			targetDevice->onInputDeviceDisconnected(managers, this);

			this->onOutputDeviceDisconnected(managers, targetDevice.get());
		}

		return ConnexionResult::Success;
	}

	void
	AbstractVirtualDevice::disconnectFromAll (AVManagers & managers, bool fireEvents) noexcept
	{
		/* NOTE: We lock IO access on this device. */
		const std::lock_guard< std::mutex > lockHere{m_IOAccess};

		const auto thisDevice = this->shared_from_this();

		for ( const auto & deviceWeakPointer : m_inputDevicesConnected )
		{
			if ( const auto inputDeviceConnected = deviceWeakPointer.lock() )
			{
				/* NOTE: We lock IO access on the other device. */
				const std::lock_guard< std::mutex > lockThere{inputDeviceConnected->m_IOAccess};

				inputDeviceConnected->m_outputDevicesConnected.erase(thisDevice);

				if ( fireEvents )
				{
					inputDeviceConnected->onOutputDeviceDisconnected(managers, this);
				}
			}
		}

		m_inputDevicesConnected.clear();

		for ( const auto & deviceWeakPointer : m_outputDevicesConnected )
		{
			if ( const auto outputDeviceConnected = deviceWeakPointer.lock() )
			{
				/* NOTE: We lock IO access on the other device. */
				const std::lock_guard< std::mutex > lockThere{outputDeviceConnected->m_IOAccess};

				outputDeviceConnected->m_inputDevicesConnected.erase(thisDevice);

				if ( fireEvents )
				{
					outputDeviceConnected->onInputDeviceDisconnected(managers, this);
				}
			}
		}

		m_outputDevicesConnected.clear();
	}

	std::string
	AbstractVirtualDevice::getConnexionState () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_IOAccess};

		std::stringstream string;

		if ( m_outputDevicesConnected.empty() )
		{
			string << "\t" " - " << this->id() << " -> [NOT_CONNECTED]" << "\n";
		}
		else
		{
			for ( const auto & outputWeak : m_outputDevicesConnected )
			{
				if ( const auto output = outputWeak.lock(); output == nullptr )
				{
					string << "\t" " - " << this->id() << " -> [BROKEN_DEVICE] " "\n";
				}
				else
				{
					string << "\t" " - " << this->id() << " -> " << output->id() << '\n';
				}
			}
		}

		return string.str();
	}
}
