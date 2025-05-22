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
#include <ranges>
#include <sstream>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::AVConsole
{
	using namespace EmEn::Libs;

	constexpr auto TracerTag{"VirtualDevice"};

	size_t AbstractVirtualDevice::s_deviceCount{0};

	AbstractVirtualDevice::AbstractVirtualDevice (const std::string & name, DeviceType type, ConnexionType allowedConnexionType) noexcept
		: m_id(AbstractVirtualDevice::buildDeviceId(name)),
		  m_type(type),
		  m_allowedConnexionType(allowedConnexionType)
	{

	}

	bool
	AbstractVirtualDevice::isConnectedWith (const std::shared_ptr< AbstractVirtualDevice > & device, ConnexionType direction) const noexcept
	{
		if ( direction == ConnexionType::Input || direction == ConnexionType::Both )
		{
			return m_inputs.contains(device);
		}

		/* NOTE: Both are already tested above. */
		if ( direction == ConnexionType::Output /*|| direction == ConnexionType::Both*/ )
		{
			return m_outputs.contains(device);
		}

		return false;
	}

	bool
	AbstractVirtualDevice::canConnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice) const noexcept
	{
		/* NOTE: Avoid connecting an audio device with a video device -> non sens! */
		if ( m_type != targetDevice->m_type )
		{
			Tracer::error(TracerTag, "Devices are not the same type !");

			return false;
		}

		/* NOTE: As connexions go from device output to another device input, this device must allow outputting! */
		if ( m_allowedConnexionType == ConnexionType::Input )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' do not allows output !";

			return false;
		}

		return true;
	}

	bool
	AbstractVirtualDevice::connect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool quietly) noexcept
	{
		if ( !this->canConnect(targetDevice) )
		{
			return false;
		}

		/* NOTE: This will check if the target device is allowed to connect back. */
		if ( !targetDevice->connectBack(this->shared_from_this(), quietly) )
		{
			return false;
		}

		m_outputs.emplace(targetDevice);

		if ( !quietly )
		{
			this->onTargetConnected(targetDevice.get());
		}

		return true;
	}

	bool
	AbstractVirtualDevice::connectBack (const std::shared_ptr< AbstractVirtualDevice > & sourceDevice, bool quietly) noexcept
	{
		if ( m_allowedConnexionType == ConnexionType::Output )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' do not allows input !";

			return false;
		}

		if ( !m_inputs.emplace(sourceDevice).second )
		{
			TraceError{TracerTag} << "Unable to register '" << sourceDevice->id() << "' to the virtual device '" << this->id() << "' inputs !";

			return false;
		}

		if ( !quietly )
		{
			this->onSourceConnected(sourceDevice.get());
		}

		return true;
	}

	bool
	AbstractVirtualDevice::interconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice) noexcept
	{
		/* NOTE: Before performs any disconnection, we need to know first
		 * if the device is able to support input and output. */
		if ( targetDevice->allowedConnexionType() == ConnexionType::Both )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' must allow input/output to perform a interconnection !";

			return false;
		}

		/* NOTE: Double-check. Not really necessary, but help to have a clear output message on failure.  */
		if ( !this->canConnect(targetDevice) )
		{
			return false;
		}

		if ( m_outputs.empty() )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' has no existing output connexion !";

			return false;
		}

		return std::ranges::all_of(m_outputs, [&] (const auto & outputDevice) {
			/* NOTE: First unlink the direct connexion and relink with the new device in between. */
			return this->disconnect(outputDevice, true) && this->connect(targetDevice, true) && targetDevice->connect(outputDevice, false);
		});
	}

	bool
	AbstractVirtualDevice::interconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, const std::string & outputDeviceName) noexcept
	{
		/* NOTE: Before performs any disconnection, we need to know first
		 * if the device is able to support input and output. */
		if ( targetDevice->allowedConnexionType() == ConnexionType::Both )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' must allow input/output to perform a interconnection !";

			return false;
		}

		/* NOTE: Double-check. Not really necessary, but help to have a clear output message on failure.  */
		if ( !this->canConnect(targetDevice) )
		{
			return false;
		}

		if ( m_outputs.empty() )
		{
			TraceError{TracerTag} << "The virtual device '" << this->id() << "' has no existing output connexion !";

			return false;
		}

		const auto outputDevice = std::ranges::find_if(m_outputs, [&outputDeviceName] (const auto & outputDevice) {
			return outputDeviceName.empty() || outputDeviceName == outputDevice->id();
		});

		if ( outputDevice == m_outputs.cend() )
		{
			TraceError{TracerTag} << "There is no output virtual device named '" << outputDeviceName << "' !";

			return false;
		}

		/* NOTE: First unlink the direct connexion and relink with the new device in between. */
		return this->disconnect(*outputDevice, true) && this->connect(targetDevice, true) && targetDevice->connect(*outputDevice, false);
	}

	bool
	AbstractVirtualDevice::disconnect (const std::shared_ptr< AbstractVirtualDevice > & targetDevice, bool quietly) noexcept
	{
		if ( !targetDevice->disconnectBack(this->shared_from_this(), quietly) )
		{
			return false;
		}

		m_outputs.erase(targetDevice);

		if ( !quietly )
		{
			this->onTargetDisconnected(targetDevice.get());
		}

		return true;
	}

	bool
	AbstractVirtualDevice::disconnectBack (const std::shared_ptr< AbstractVirtualDevice > & sourceDevice, bool quietly) noexcept
	{
		if ( m_inputs.erase(sourceDevice) == 0 )
		{
			TraceError{TracerTag} << "Unable to unregister '" << sourceDevice->id() << "' from the virtual device '" << this->id() << "' inputs !";

			return false;
		}

		if ( !quietly )
		{
			this->onSourceDisconnected(sourceDevice.get());
		}

		return true;
	}

	void
	AbstractVirtualDevice::disconnectFromAll () noexcept
	{
		/* Remove these devices from the outputs of every device where it's connected. */
		for ( const auto & input : m_inputs )
		{
			input->m_outputs.erase(this->shared_from_this());
		}

		m_inputs.clear();

		/* Remove these devices from the inputs of every device where it's connected. */
		for ( const auto & output : m_outputs )
		{
			output->disconnectBack(this->shared_from_this(), true);
		}

		m_outputs.clear();
	}

	std::string
	AbstractVirtualDevice::getConnexionState () const noexcept
	{
		std::stringstream string{};

		if ( m_outputs.empty() )
		{
			string << "\t" " - " << this->id() << " -> [NOT CONNECTED]" << "\n";
		}
		else
		{
			for ( const auto & output : m_outputs )
			{
				string << "\t" " - " << this->id() << " -> " << output->id() << "\n";
			}
		}

		return string.str();
	}

	std::string
	AbstractVirtualDevice::buildDeviceId (const std::string & name) noexcept
	{
		std::stringstream deviceId;

		deviceId << name << '_' << s_deviceCount++;

		return deviceId.str();
	}
}
