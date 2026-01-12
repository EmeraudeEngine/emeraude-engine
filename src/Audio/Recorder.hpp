/*
 * src/Audio/Recorder.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Vector.hpp"

/* https://kcat.strangesoft.net/openal-extensions/SOFT_loopback.txt */
namespace EmEn::Audio
{
	/**
	 * @brief The audio recorder class.
	 * @extends EmEn::Scenes::AVConsole::AbstractVirtualDevice This is a virtual audio device.
	 */
	class Recorder final : public Scenes::AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Recorder"};

			/**
			 * @brief Constructs a recorder.
			 * @param name A reference to a name.
			 */
			explicit
			Recorder (const std::string & name) noexcept
				: AbstractVirtualDevice{name, Scenes::AVConsole::DeviceType::Audio, Scenes::AVConsole::ConnexionType::Both}
			{

			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_worldCoordinates;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
	};
}
