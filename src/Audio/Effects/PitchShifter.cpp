/*
 * src/Audio/Effects/PitchShifter.cpp
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

#include "PitchShifter.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	PitchShifter::PitchShifter () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_PITCH_SHIFTER);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Pith Shifter effect !");
		}
	}

	void
	PitchShifter::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_PITCH_SHIFTER_COARSE_TUNE, AL_PITCH_SHIFTER_DEFAULT_COARSE_TUNE);
		EFX::alEffecti(this->identifier(), AL_PITCH_SHIFTER_FINE_TUNE, AL_PITCH_SHIFTER_DEFAULT_FINE_TUNE);
	}

	void
	PitchShifter::setShifterCoarseTune (int value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_PITCH_SHIFTER_MIN_COARSE_TUNE || value > AL_PITCH_SHIFTER_MAX_COARSE_TUNE )
		{
			TraceWarning{ClassId} << "Coarse tune must be between " << AL_PITCH_SHIFTER_MIN_COARSE_TUNE << " and " << AL_PITCH_SHIFTER_MAX_COARSE_TUNE << '.';

			return;
		}

		EFX::alEffecti(this->identifier(), AL_PITCH_SHIFTER_COARSE_TUNE, value);
	}

	void
	PitchShifter::setShifterFineTune (int value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_PITCH_SHIFTER_MIN_FINE_TUNE || value > AL_PITCH_SHIFTER_MAX_FINE_TUNE )
		{
			TraceWarning{ClassId} << "Fine tune must be between " << AL_PITCH_SHIFTER_MIN_FINE_TUNE << " and " << AL_PITCH_SHIFTER_MAX_FINE_TUNE << '.';

			return;
		}

		EFX::alEffecti(this->identifier(), AL_PITCH_SHIFTER_FINE_TUNE, value);
	}

	int
	PitchShifter::shifterCoarseTune () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_PITCH_SHIFTER_COARSE_TUNE, &value);
		}

		return value;
	}

	int
	PitchShifter::shifterFineTune () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_PITCH_SHIFTER_FINE_TUNE, &value);
		}

		return value;
	}
}
