/*
 * src/Audio/Effects/FrequencyShifter.cpp
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

#include "FrequencyShifter.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	FrequencyShifter::FrequencyShifter () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_FREQUENCY_SHIFTER);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Frequency Shifter effect !");
		}
	}

	void
	FrequencyShifter::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_FREQUENCY_SHIFTER_FREQUENCY, AL_FREQUENCY_SHIFTER_DEFAULT_FREQUENCY);
		EFX::alEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_LEFT_DIRECTION, AL_FREQUENCY_SHIFTER_DEFAULT_LEFT_DIRECTION);
		EFX::alEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION, AL_FREQUENCY_SHIFTER_DEFAULT_RIGHT_DIRECTION);
	}

	void
	FrequencyShifter::setFrequency (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FREQUENCY_SHIFTER_MIN_FREQUENCY || value > AL_FREQUENCY_SHIFTER_MAX_FREQUENCY )
		{
			TraceWarning{ClassId} << "Frequency must be between " << AL_FREQUENCY_SHIFTER_MIN_FREQUENCY << " and " << AL_FREQUENCY_SHIFTER_MAX_FREQUENCY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_FREQUENCY_SHIFTER_FREQUENCY, value);
	}

	void
	FrequencyShifter::setLeftDirection (Direction value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case Direction::Down :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_DOWN;
				break;

			case Direction::Up :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_UP;
				break;

			case Direction::Off :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_OFF;
				break;
		}

		EFX::alEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_LEFT_DIRECTION, def);
	}

	void
	FrequencyShifter::setRightDirection (Direction value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case Direction::Down :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_DOWN;
				break;

			case Direction::Up :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_UP;
				break;

			case Direction::Off :
				def = AL_FREQUENCY_SHIFTER_DIRECTION_OFF;
				break;
		}

		EFX::alEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION, def);
	}

	float
	FrequencyShifter::frequency () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_FREQUENCY_SHIFTER_FREQUENCY, &value);
		}

		return value;
	}

	FrequencyShifter::Direction
	FrequencyShifter::leftDirection () const noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return Direction::Down;
		}

		ALint value = 0;

		EFX::alGetEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_LEFT_DIRECTION, &value);

		switch ( value )
		{
			case AL_FREQUENCY_SHIFTER_DIRECTION_UP :
				return Direction::Up;

			case AL_FREQUENCY_SHIFTER_DIRECTION_OFF :
				return Direction::Off;

			case AL_FREQUENCY_SHIFTER_DIRECTION_DOWN :
			default :
				return Direction::Down;
		}
	}

	FrequencyShifter::Direction
	FrequencyShifter::rightDirection () const noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return Direction::Down;
		}

		ALint value = 0;

		EFX::alGetEffecti(this->identifier(), AL_FREQUENCY_SHIFTER_RIGHT_DIRECTION, &value);

		switch ( value )
		{
			case AL_FREQUENCY_SHIFTER_DIRECTION_UP :
				return Direction::Up;

			case AL_FREQUENCY_SHIFTER_DIRECTION_OFF :
				return Direction::Off;

			case AL_FREQUENCY_SHIFTER_DIRECTION_DOWN :
			default :
				return Direction::Down;
		}
	}
}
