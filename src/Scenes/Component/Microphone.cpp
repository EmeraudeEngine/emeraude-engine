/*
 * src/Scenes/Component/Microphone.cpp
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

#include "Microphone.hpp"

/* STL inclusions. */
#include <string>
#include <ostream>
#include <array>

/* Local inclusions. */
#include "Audio/Manager.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Audio;

	Microphone::Microphone (const std::string & name, const AbstractEntity & parentEntity) noexcept
		: Abstract(name, parentEntity),
		AbstractVirtualDevice(name, AVConsole::DeviceType::Audio, AVConsole::ConnexionType::Output)
	{

	}

	void
	Microphone::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
	}

	void
	Microphone::processLogics (const Scene & /*scene*/) noexcept
	{

	}

	void
	Microphone::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		if ( !this->hasOutputConnected() )
		{
			return;
		}

		for ( const auto & output : this->outputs() )
		{
			output->updateDeviceFromCoordinates(worldCoordinates, worldVelocity);
		}
	}

	void
	Microphone::onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept
	{
		/* Initialize the target device with coordinates. */
		targetDevice->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
	}

	bool
	Microphone::playAnimation (uint8_t /*animationID*/, const Variant & /*value*/, size_t /*cycle*/) noexcept
	{
		return false;
	}

	std::ostream &
	operator<< (std::ostream & out, const Microphone & /*microphone*/)
	{
		std::array< ALfloat, 12 > properties{0};

		Audio::Manager::instance()->listenerProperties(properties);

		return out <<
			"Audio Listener information" "\n"
			"Position: " << properties[0] << ", " << properties[1] << ", " << properties[2] << "\n"
			"Forward: " << properties[3] << ", " << properties[4] << ", " << properties[5] << "\n"
			"Upward: " << properties[6] << ", " << properties[7] << ", " << properties[8] << "\n"
			"Velocity: " << properties[9] << ", " << properties[10] << ", " << properties[11] << '\n';
	}

	std::string
	to_string (const Microphone & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
