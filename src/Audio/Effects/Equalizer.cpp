/*
 * src/Audio/Effects/Equalizer.cpp
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

#include "Equalizer.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	Equalizer::Equalizer () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Equalize effect !");
		}
	}

	void
	Equalizer::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_LOW_GAIN, AL_EQUALIZER_DEFAULT_LOW_GAIN);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_LOW_CUTOFF, AL_EQUALIZER_DEFAULT_LOW_CUTOFF);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_GAIN, AL_EQUALIZER_DEFAULT_MID1_GAIN);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_CENTER, AL_EQUALIZER_DEFAULT_MID1_CENTER);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_WIDTH, AL_EQUALIZER_DEFAULT_MID1_WIDTH);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_GAIN, AL_EQUALIZER_DEFAULT_MID2_GAIN);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_CENTER, AL_EQUALIZER_DEFAULT_MID2_CENTER);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_WIDTH, AL_EQUALIZER_DEFAULT_MID2_WIDTH);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_HIGH_GAIN, AL_EQUALIZER_DEFAULT_HIGH_GAIN);
		EFX::alEffectf(this->identifier(), AL_EQUALIZER_HIGH_CUTOFF, AL_EQUALIZER_DEFAULT_HIGH_CUTOFF);
	}

	void
	Equalizer::setLowGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_LOW_GAIN || value > AL_EQUALIZER_MAX_LOW_GAIN )
		{
			TraceWarning{ClassId} << "Low gain must be between " << AL_EQUALIZER_MIN_LOW_GAIN << " and " << AL_EQUALIZER_MAX_LOW_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_LOW_GAIN, value);
	}

	void
	Equalizer::setLowCutOff (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_LOW_CUTOFF || value > AL_EQUALIZER_MAX_LOW_CUTOFF )
		{
			TraceWarning{ClassId} << "Low cut-off must be between " << AL_EQUALIZER_MIN_LOW_CUTOFF << " and " << AL_EQUALIZER_MAX_LOW_CUTOFF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_LOW_CUTOFF, value);
	}

	void
	Equalizer::setMid1Gain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}
		if ( value < AL_EQUALIZER_MIN_MID1_GAIN || value > AL_EQUALIZER_MAX_MID1_GAIN )
		{
			TraceWarning{ClassId} << "Mid1 gain must be between " << AL_EQUALIZER_MIN_MID1_GAIN << " and " << AL_EQUALIZER_MAX_MID1_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_GAIN, value);
	}

	void
	Equalizer::setMid1Center (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_MID1_CENTER || value > AL_EQUALIZER_MAX_MID1_CENTER )
		{
			TraceWarning{ClassId} << "Mid1 center must be between " << AL_EQUALIZER_MIN_MID1_CENTER << " and " << AL_EQUALIZER_MAX_MID1_CENTER << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_CENTER, value);
	}

	void
	Equalizer::setMid1Width (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_MID1_WIDTH || value > AL_EQUALIZER_MAX_MID1_WIDTH )
		{
			TraceWarning{ClassId} << "Mid1 width must be between " << AL_EQUALIZER_MIN_MID1_WIDTH << " and " << AL_EQUALIZER_MAX_MID1_WIDTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID1_WIDTH, value);
	}

	void
	Equalizer::setMid2Gain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_MID2_GAIN || value > AL_EQUALIZER_MAX_MID2_GAIN )
		{
			TraceWarning{ClassId} << "Mid2 gain must be between " << AL_EQUALIZER_MIN_MID2_GAIN << " and " << AL_EQUALIZER_MAX_MID2_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_GAIN, value);
	}

	void
	Equalizer::setMid2Center (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}
		if ( value < AL_EQUALIZER_MIN_MID2_CENTER || value > AL_EQUALIZER_MAX_MID2_CENTER )
		{
			TraceWarning{ClassId} << "Mid2 center must be between " << AL_EQUALIZER_MIN_MID2_CENTER << " and " << AL_EQUALIZER_MAX_MID2_CENTER << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_CENTER, value);
	}

	void
	Equalizer::setMid2Width (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_MID2_WIDTH || value > AL_EQUALIZER_MAX_MID2_WIDTH )
		{
			TraceWarning{ClassId} << "Mid2 width must be between " << AL_EQUALIZER_MIN_MID2_WIDTH << " and " << AL_EQUALIZER_MAX_MID2_WIDTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_MID2_WIDTH, value);
	}

	void
	Equalizer::setHighGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_HIGH_GAIN || value > AL_EQUALIZER_MAX_HIGH_GAIN )
		{
			TraceWarning{ClassId} << "High gain must be between " << AL_EQUALIZER_MIN_HIGH_GAIN << " and " << AL_EQUALIZER_MAX_HIGH_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_HIGH_GAIN, value);
	}

	void
	Equalizer::setHighCutOff (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EQUALIZER_MIN_HIGH_CUTOFF || value > AL_EQUALIZER_MAX_HIGH_CUTOFF )
		{
			TraceWarning{ClassId} << "High cut-off must be between " << AL_EQUALIZER_MIN_HIGH_CUTOFF << " and " << AL_EQUALIZER_MAX_HIGH_CUTOFF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EQUALIZER_HIGH_CUTOFF, value);
	}

	float
	Equalizer::lowGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_LOW_GAIN, &value);
		}

		return value;
	}

	float
	Equalizer::lowCutOff () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_LOW_CUTOFF, &value);
		}

		return value;
	}

	float
	Equalizer::mid1Gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID1_GAIN, &value);
		}

		return value;
	}

	float
	Equalizer::mid1Center () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID1_CENTER, &value);
		}

		return value;
	}

	float
	Equalizer::mid1Width () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID1_WIDTH, &value);
		}

		return value;
	}

	float
	Equalizer::mid2Gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID2_GAIN, &value);
		}

		return value;
	}

	float
	Equalizer::mid2Center () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID2_CENTER, &value);
		}

		return value;
	}

	float
	Equalizer::mid2Width () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_MID2_WIDTH, &value);
		}

		return value;
	}

	float
	Equalizer::highGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_HIGH_GAIN, &value);
		}

		return value;
	}

	float
	Equalizer::highCutOff () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EQUALIZER_HIGH_CUTOFF, &value);
		}

		return value;
	}
}
