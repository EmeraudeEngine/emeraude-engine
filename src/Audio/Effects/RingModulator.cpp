/*
 * src/Audio/Effects/RingModulator.cpp
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

#include "RingModulator.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	RingModulator::RingModulator () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_RING_MODULATOR);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Ring Modulator effect !");
		}
	}

	void
	RingModulator::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_RING_MODULATOR_FREQUENCY, AL_RING_MODULATOR_DEFAULT_FREQUENCY);
		EFX::alEffectf(this->identifier(), AL_RING_MODULATOR_HIGHPASS_CUTOFF, AL_RING_MODULATOR_DEFAULT_HIGHPASS_CUTOFF);
		EFX::alEffecti(this->identifier(), AL_RING_MODULATOR_WAVEFORM, AL_RING_MODULATOR_DEFAULT_WAVEFORM);
	}

	void
	RingModulator::setFrequency (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_RING_MODULATOR_MIN_FREQUENCY || value > AL_RING_MODULATOR_MAX_FREQUENCY )
		{
			TraceWarning{ClassId} << "Frequency must be between " << AL_RING_MODULATOR_MIN_FREQUENCY << " and " << AL_RING_MODULATOR_MAX_FREQUENCY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_RING_MODULATOR_FREQUENCY, value);
	}

	void
	RingModulator::setHighPassCutOff (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_RING_MODULATOR_MIN_HIGHPASS_CUTOFF || value > AL_RING_MODULATOR_MAX_HIGHPASS_CUTOFF )
		{
			TraceWarning{ClassId} << "HighPass CutOff must be between " << AL_RING_MODULATOR_MIN_HIGHPASS_CUTOFF << " and " << AL_RING_MODULATOR_MAX_HIGHPASS_CUTOFF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_RING_MODULATOR_HIGHPASS_CUTOFF, value);
	}

	void
	RingModulator::setWaveForm (WaveForm value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case WaveForm::Sinusoid :
				def = AL_RING_MODULATOR_SINUSOID;
				break;

			case WaveForm::SawTooth :
				def = AL_RING_MODULATOR_SAWTOOTH;
				break;

			case WaveForm::Square :
				def = AL_RING_MODULATOR_SQUARE;
				break;
		}

		EFX::alEffecti(this->identifier(), AL_RING_MODULATOR_WAVEFORM, def);
	}

	float
	RingModulator::frequency () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_RING_MODULATOR_FREQUENCY, &value);
		}

		return value;
	}

	float
	RingModulator::highPassCutOff () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_RING_MODULATOR_HIGHPASS_CUTOFF, &value);
		}

		return value;
	}

	RingModulator::WaveForm
	RingModulator::waveForm () const noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return WaveForm::Sinusoid;
		}

		ALint value = 0;

		EFX::alGetEffecti(this->identifier(), AL_RING_MODULATOR_WAVEFORM, &value);

		switch ( value )
		{
			case AL_RING_MODULATOR_SAWTOOTH :
				return WaveForm::SawTooth;

			case AL_RING_MODULATOR_SQUARE :
				return WaveForm::Square;

			case AL_RING_MODULATOR_SINUSOID :
			default:
				return WaveForm::Sinusoid;
		}
	}
}
