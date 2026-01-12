/*
 * src/Audio/AmbienceChannel.cpp
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

#include "AmbienceChannel.hpp"

namespace EmEn::Audio
{
	using namespace Libs;
	using namespace Libs::Math;

	unsigned int
	AmbienceChannel::play (const AmbienceSound & sound, float radius) noexcept
	{
		/* Randomize the source parameters. */
		const auto pitch = sound.getRandomPitch();

		m_position = Vector< 3, float >::quickRandom(-radius, radius);
		m_velocity = sound.getRandomVelocity();

		if ( !sound.isRelative() )
		{
			Vector< 3, float > listenerPosition;

			alGetListenerfv(AL_POSITION, listenerPosition.data());

			m_position += listenerPosition;
		}

		/* Configure the ambience channel. */
		if ( sound.useVelocityRandomization() )
		{
			this->setVelocity(sound.getRandomVelocity());
		}

		/* Configure of the audio source. */
		m_source->setRelativeState(sound.isRelative());
		m_source->setPosition(m_position);
		m_source->setVelocity(m_velocity);
		m_source->setDirection(Vector< 3, float >::quickRandom(-1.0F, 1.0F).normalize());
		m_source->setGain(sound.channelGain());
		m_source->setPitch(pitch);

		m_source->play(sound.soundResource(), PlayMode::Once);

		return static_cast< unsigned int >(sound.soundResource()->localData().milliseconds() / pitch);
	}

	void
	AmbienceChannel::update (unsigned int time) noexcept
	{
		m_time += time;

		/* Update the position if there is a fake velocity enabled. */
		if ( !m_velocity.isZero() )
		{
			m_position += m_velocity;
			m_source->setPosition(m_position);
		}
	}
}
