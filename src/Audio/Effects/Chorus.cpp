/*
 * src/Audio/Effects/Chorus.cpp
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

#include "Chorus.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	Chorus::Chorus () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_CHORUS);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Chorus effect !");
		}
	}

	void
	Chorus::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_CHORUS_WAVEFORM, AL_CHORUS_DEFAULT_WAVEFORM);
		EFX::alEffecti(this->identifier(), AL_CHORUS_PHASE, AL_CHORUS_DEFAULT_PHASE);
		EFX::alEffectf(this->identifier(), AL_CHORUS_RATE, AL_CHORUS_DEFAULT_RATE);
		EFX::alEffectf(this->identifier(), AL_CHORUS_DEPTH, AL_CHORUS_DEFAULT_DEPTH);
		EFX::alEffectf(this->identifier(), AL_CHORUS_FEEDBACK, AL_CHORUS_DEFAULT_FEEDBACK);
		EFX::alEffectf(this->identifier(), AL_CHORUS_DELAY, AL_CHORUS_DEFAULT_DELAY);
	}

	void
	Chorus::setWaveForm (WaveForm value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case WaveForm::Sinusoid :
				def = AL_CHORUS_WAVEFORM_SINUSOID;
				break;

			case WaveForm::Triangle :
				def = AL_CHORUS_WAVEFORM_TRIANGLE;
				break;
		}

		EFX::alEffecti(this->identifier(), AL_CHORUS_WAVEFORM, def);
	}

	void
	Chorus::setPhase (int value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_CHORUS_MIN_PHASE || value > AL_CHORUS_MAX_PHASE )
		{
			TraceWarning{ClassId} << "Phase must be between " << AL_CHORUS_MIN_PHASE << " and " << AL_CHORUS_MAX_PHASE << '.';

			return;
		}

		EFX::alEffecti(this->identifier(), AL_CHORUS_PHASE, value);
	}

	void
	Chorus::setRate (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_CHORUS_MIN_RATE || value > AL_CHORUS_MAX_RATE )
		{
			TraceWarning{ClassId} << "Rate must be between " << AL_CHORUS_MIN_RATE << " and " << AL_CHORUS_MAX_RATE << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_CHORUS_RATE, value);
	}

	void
	Chorus::setDepth (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_CHORUS_MIN_DEPTH || value > AL_CHORUS_MAX_DEPTH )
		{
			TraceWarning{ClassId} << "Depth must be between " << AL_CHORUS_MIN_DEPTH << " and " << AL_CHORUS_MAX_DEPTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_CHORUS_DEPTH, value);
	}

	void
	Chorus::setFeedBack (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_CHORUS_MIN_FEEDBACK || value > AL_CHORUS_MAX_FEEDBACK )
		{
			TraceWarning{ClassId} << "Feedback must be between " << AL_CHORUS_MIN_FEEDBACK << " and " << AL_CHORUS_MAX_FEEDBACK << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_CHORUS_FEEDBACK, value);
	}

	void
	Chorus::setDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_CHORUS_MIN_DELAY || value > AL_CHORUS_MAX_DELAY )
		{
			TraceWarning{ClassId} << "Delay must be between " << AL_CHORUS_MIN_DELAY << " and " << AL_CHORUS_MAX_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_CHORUS_DELAY, value);
	}

	Chorus::WaveForm
	Chorus::waveForm () const noexcept
	{
		if ( EFX::isAvailable() )
		{
			ALint value = 0;

			EFX::alGetEffecti(this->identifier(), AL_CHORUS_WAVEFORM, &value);

			switch ( value )
			{
				case AL_CHORUS_WAVEFORM_SINUSOID :
					return WaveForm::Sinusoid;

				case AL_CHORUS_WAVEFORM_TRIANGLE :
				default:
					return WaveForm::Triangle;
			}
		}

		return WaveForm::Triangle;
	}

	int
	Chorus::phase () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_CHORUS_PHASE, &value);
		}

		return value;
	}

	float
	Chorus::rate () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_CHORUS_RATE, &value);
		}

		return value;
	}

	float
	Chorus::depth () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_CHORUS_DEPTH, &value);
		}

		return value;
	}

	float
	Chorus::feedBack () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_CHORUS_FEEDBACK, &value);
		}

		return value;
	}

	float
	Chorus::delay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_CHORUS_DELAY, &value);
		}

		return value;
	}
}
