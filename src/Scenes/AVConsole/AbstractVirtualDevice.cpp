/*
 * src/Scenes/AVConsole/AbstractVirtualDevice.cpp
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

#include "AbstractVirtualDevice.hpp"

/* STL inclusions. */
#include <ranges>
#include <sstream>
#include <vector>

namespace EmEn::Scenes::AVConsole
{
	using namespace Base;

	ConnexionResult
	AbstractVirtualDevice::connect (EngineContext & engineContext, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept
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
			targetDevice->onInputDeviceConnected(engineContext, *this);

			this->onOutputDeviceConnected(engineContext, *targetDevice);
		}

		return ConnexionResult::Success;
	}

	ConnexionResult
	AbstractVirtualDevice::disconnect (EngineContext & engineContext, const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool fireEvents) noexcept
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
			targetDevice->onInputDeviceDisconnected(engineContext, *this);

			this->onOutputDeviceDisconnected(engineContext, *targetDevice);
		}

		return ConnexionResult::Success;
	}

	void
	AbstractVirtualDevice::disconnectFromAll (EngineContext & engineContext, bool fireEvents) noexcept
	{
		/* NOTE: The same discipline as connect() / disconnect(): both sides of a link are edited under ONE
		 * std::scoped_lock of the two mutexes (no lock-order deadlock between two devices), and the events are
		 * fired once the locks are released (a handler may query this device). weak_from_this(): valid even when no
		 * shared_ptr owns this device any more (the teardown path; shared_from_this() crashed there). */
		const auto thisDevice = this->weak_from_this();

		std::vector< std::shared_ptr< AbstractVirtualDevice > > inputDevices;
		std::vector< std::shared_ptr< AbstractVirtualDevice > > outputDevices;

		{
			const std::scoped_lock lock{m_IOAccess};

			inputDevices.reserve(m_inputDevicesConnected.size());
			outputDevices.reserve(m_outputDevicesConnected.size());

			for ( const auto & deviceWeakPointer : m_inputDevicesConnected )
			{
				if ( auto device = deviceWeakPointer.lock() )
				{
					inputDevices.emplace_back(std::move(device));
				}
			}

			for ( const auto & deviceWeakPointer : m_outputDevicesConnected )
			{
				if ( auto device = deviceWeakPointer.lock() )
				{
					outputDevices.emplace_back(std::move(device));
				}
			}
		}

		for ( const auto & inputDevice : inputDevices )
		{
			{
				const std::scoped_lock lock{m_IOAccess, inputDevice->m_IOAccess};

				inputDevice->m_outputDevicesConnected.erase(thisDevice);
				m_inputDevicesConnected.erase(inputDevice);
			}

			if ( fireEvents )
			{
				inputDevice->onOutputDeviceDisconnected(engineContext, *this);
			}
		}

		for ( const auto & outputDevice : outputDevices )
		{
			{
				const std::scoped_lock lock{m_IOAccess, outputDevice->m_IOAccess};

				outputDevice->m_inputDevicesConnected.erase(thisDevice);
				m_outputDevicesConnected.erase(outputDevice);
			}

			if ( fireEvents )
			{
				outputDevice->onInputDeviceDisconnected(engineContext, *this);
			}
		}

		/* The expired leftovers (devices already gone). */
		const std::scoped_lock lock{m_IOAccess};

		m_inputDevicesConnected.clear();
		m_outputDevicesConnected.clear();
	}

	std::string
	AbstractVirtualDevice::getConnexionState () const noexcept
	{
		const std::scoped_lock lock{m_IOAccess};

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
