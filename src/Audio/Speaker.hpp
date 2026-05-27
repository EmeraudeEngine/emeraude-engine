/*
 * src/Audio/Speaker.hpp
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

#pragma once

/* STL inclusions. */
#include <string>

/* Local inclusions for inheritances. */
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief The Speaker class
	 * @extends EmEn::Scenes::AVConsole::AbstractVirtualDevice This is a virtual audio device.
	 */
	class Speaker final : public Scenes::AVConsole::AbstractVirtualDevice
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"AudioSpeaker"};

			/**
			 * @brief Constructs a speaker.
			 * @param name A reference to a name.
			 */
			explicit
			Speaker (const std::string & name) noexcept
				: AbstractVirtualDevice{name, Scenes::AVConsole::DeviceType::Audio, Scenes::AVConsole::ConnexionType::Input}
			{

			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_worldCoordinates;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > &  /*worldVelocity*/) noexcept override
			{
				m_worldCoordinates = worldCoordinates;
			}

		private:

			Base::Math::CartesianFrame< float > m_worldCoordinates;
	};
}
