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
 * https://github.com/EmeraudeEngine/emeraude-engine
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
	using namespace Base;
	using namespace Base::Math;

	void
	HardwareOutput::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_worldCoordinates = worldCoordinates;

		const auto & position = worldCoordinates.position();
		const auto & atVector = worldCoordinates.forwardVector();
		/* 🔴 KNOWN DEFECT — OPEN (Y-up residue). OpenAL's AL_ORIENTATION takes a genuine UP vector.
		 * This hands it the DOWN one: a compensation for the retired Y-DOWN world that the Aug 2026
		 * Y-up flip should have removed. The world is Y-up now, so this MUST read upwardVector();
		 * as it stands the vertical axis of the audio field is INVERTED.
		 * ⚠️ Invisible to every visual check — stereo/HRTF panning silently disagrees with the
		 * picture, with nothing on screen to show for it. Verify by playing a positional sound
		 * ABOVE the listener and confirming which way it comes from.
		 * Deliberately NOT corrected here: it needs an audible check (owner decision, Aug 2026).
		 * See TODO.md § Y-UP RESIDUES. */
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
