/*
 * src/Audio/Effects/Flanger.cpp
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

#include "Flanger.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	Flanger::Flanger () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_FLANGER);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Flanger effect !");
		}
	}

	void
	Flanger::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_FLANGER_WAVEFORM, AL_FLANGER_DEFAULT_WAVEFORM);
		EFX::alEffecti(this->identifier(), AL_FLANGER_PHASE, AL_FLANGER_DEFAULT_PHASE);
		EFX::alEffectf(this->identifier(), AL_FLANGER_RATE, AL_FLANGER_DEFAULT_RATE);
		EFX::alEffectf(this->identifier(), AL_FLANGER_DEPTH, AL_FLANGER_DEFAULT_DEPTH);
		EFX::alEffectf(this->identifier(), AL_FLANGER_FEEDBACK, AL_FLANGER_DEFAULT_FEEDBACK);
		EFX::alEffectf(this->identifier(), AL_FLANGER_DELAY, AL_FLANGER_DEFAULT_DELAY);
	}

	void
	Flanger::setWaveForm (WaveForm value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case WaveForm::Sinusoid :
				def = AL_FLANGER_WAVEFORM_SINUSOID;
				break;

			case WaveForm::Triangle :
				def = AL_FLANGER_WAVEFORM_TRIANGLE;
				break;
		}

		EFX::alEffecti(this->identifier(), AL_FLANGER_WAVEFORM, def);
	}

	void
	Flanger::setPhase (int value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FLANGER_MIN_PHASE || value > AL_FLANGER_MAX_PHASE )
		{
			TraceWarning{ClassId} << "Phase must be between " << AL_FLANGER_MIN_PHASE << " and " << AL_FLANGER_MAX_PHASE << '.';

			return;
		}

		EFX::alEffecti(this->identifier(), AL_FLANGER_PHASE, value);
	}

	void
	Flanger::setRate (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FLANGER_MIN_RATE || value > AL_FLANGER_MAX_RATE )
		{
			TraceWarning{ClassId} << "Rate must be between " << AL_FLANGER_MIN_RATE << " and " << AL_FLANGER_MAX_RATE << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_FLANGER_RATE, value);
	}

	void
	Flanger::setDepth (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FLANGER_MIN_DEPTH || value > AL_FLANGER_MAX_DEPTH )
		{
			TraceWarning{ClassId} << "Depth must be between " << AL_FLANGER_MIN_DEPTH << " and " << AL_FLANGER_MAX_DEPTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_FLANGER_DEPTH, value);
	}

	void
	Flanger::setFeedBack (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FLANGER_MIN_FEEDBACK || value > AL_FLANGER_MAX_FEEDBACK )
		{
			TraceWarning{ClassId} << "Feedback must be between " << AL_FLANGER_MIN_FEEDBACK << " and " << AL_FLANGER_MAX_FEEDBACK << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_FLANGER_FEEDBACK, value);
	}

	void
	Flanger::setDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_FLANGER_MIN_DELAY || value > AL_FLANGER_MAX_DELAY )
		{
			TraceWarning{ClassId} << "Delay must be between " << AL_FLANGER_MIN_DELAY << " and " << AL_FLANGER_MAX_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_FLANGER_DELAY, value);
	}

	Flanger::WaveForm
	Flanger::waveForm () const noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return WaveForm::Triangle;
		}

		ALint value = 0;

		EFX::alGetEffecti(this->identifier(), AL_FLANGER_WAVEFORM, &value);

		switch ( value )
		{
			case AL_FLANGER_WAVEFORM_SINUSOID :
				return WaveForm::Sinusoid;

			case AL_FLANGER_WAVEFORM_TRIANGLE :
			default:
				return WaveForm::Triangle;
		}
	}

	int
	Flanger::phase () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_FLANGER_PHASE, &value);
		}

		return value;
	}

	float
	Flanger::rate () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_FLANGER_RATE, &value);
		}

		return value;
	}

	float
	Flanger::depth () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_FLANGER_DEPTH, &value);
		}

		return value;
	}

	float
	Flanger::feedBack () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_FLANGER_FEEDBACK, &value);
		}

		return value;
	}

	float
	Flanger::delay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_FLANGER_DELAY, &value);
		}

		return value;
	}
}
