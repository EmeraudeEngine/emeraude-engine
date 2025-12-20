/*
 * src/Audio/HardwareOutput.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Vector.hpp"
#include "Audio/Manager.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The hardware output class.
	 * @extends EmEn::Scenes::AVConsole::AbstractVirtualDevice This is a virtual audio device.
	 */
	class HardwareOutput final : public Scenes::AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"HardwareOutput"};

			/**
			 * @brief Constructs the hardware output.
			 * @param name A reference to a name.
			 * @param audioManager A reference to the audioManager.
			 */
			HardwareOutput (const std::string & name, Manager & audioManager) noexcept
				: AbstractVirtualDevice{name, Scenes::AVConsole::DeviceType::Audio, Scenes::AVConsole::ConnexionType::Input},
				m_audioManager{&audioManager}
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

			Manager * m_audioManager{nullptr};
			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
	};
}
