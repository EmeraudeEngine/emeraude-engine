/*
 * src/Audio/HardwareOutput.cpp
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

#include "HardwareOutput.hpp"

/* STL inclusions. */
#include <array>
#include <string>

/* Local inclusions. */
#include "Audio/Manager.hpp"

namespace EmEn::Audio
{
	using namespace Libs;
	using namespace Libs::Math;

	void
	HardwareOutput::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_worldCoordinates = worldCoordinates;

		const auto & position = worldCoordinates.position();
		const auto & atVector = worldCoordinates.forwardVector();
		/* NOTE: This is not a bug, OpenAL was designed to work with OpenGL.
		 * Here we are using Vulkan which inverted the Y axis.
		 * As the engine follow the Vulkan rule at every level, we must send the downward vector instead. */
		const auto & upVector = worldCoordinates.downwardVector();

		const std::array< ALfloat, 12 > properties = {
			position[X], position[Y], position[Z],
			atVector[X], atVector[Y], atVector[Z],
			upVector[X], upVector[Y], upVector[Z],
			worldVelocity[X], worldVelocity[Y], worldVelocity[Z]
		};

		m_audioManager->setListenerProperties(properties);
	}
}
