/*
 * src/Audio/Effects/VocalMorpher.cpp
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

#include "VocalMorpher.hpp"

/* Local inclusions. */
#include "Audio/OpenALExtensions.hpp"
#include "Audio/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio::Effects
{
	using namespace Libs;

	VocalMorpher::VocalMorpher () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_VOCAL_MORPHER);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Vocal Morpher effect !");
		}
	}

	void
	VocalMorpher::resetProperties () noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA, AL_VOCAL_MORPHER_DEFAULT_PHONEMEA);
		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING, AL_VOCAL_MORPHER_DEFAULT_PHONEMEA_COARSE_TUNING);
		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB, AL_VOCAL_MORPHER_DEFAULT_PHONEMEB);
		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING, AL_VOCAL_MORPHER_DEFAULT_PHONEMEB_COARSE_TUNING);
		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_WAVEFORM, AL_VOCAL_MORPHER_DEFAULT_WAVEFORM);
	}

	void
	VocalMorpher::setPhonemeA (Phoneme value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA, static_cast< ALint >(value));
	}

	void
	VocalMorpher::setPhonemeACoarseTuning (int value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_VOCAL_MORPHER_MIN_PHONEMEA_COARSE_TUNING || value > AL_VOCAL_MORPHER_MAX_PHONEMEA_COARSE_TUNING )
		{
			TraceWarning{ClassId} << "Phoneme A coarse tuning must be between " << AL_VOCAL_MORPHER_MIN_PHONEMEA_COARSE_TUNING << " and " << AL_VOCAL_MORPHER_MAX_PHONEMEA_COARSE_TUNING << '.';

			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING, value);
	}

	void
	VocalMorpher::setPhonemeB (Phoneme value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB, static_cast< ALint >(value));
	}

	void
	VocalMorpher::setPhonemeBCoarseTuning (int value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_VOCAL_MORPHER_MIN_PHONEMEB_COARSE_TUNING || value > AL_VOCAL_MORPHER_MAX_PHONEMEB_COARSE_TUNING )
		{
			TraceWarning{ClassId} << "Phoneme B coarse tuning must be between " << AL_VOCAL_MORPHER_MIN_PHONEMEB_COARSE_TUNING << " and " << AL_VOCAL_MORPHER_MAX_PHONEMEB_COARSE_TUNING << '.';

			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING, value);
	}

	void
	VocalMorpher::setWaveForm (WaveForm value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		ALint def = 0;

		switch ( value )
		{
			case WaveForm::Sinusoid :
				def = AL_VOCAL_MORPHER_WAVEFORM_SINUSOID;
				break;

			case WaveForm::Triangle :
				def = AL_VOCAL_MORPHER_WAVEFORM_TRIANGLE;
				break;

			case WaveForm::SawTooth :
				def = AL_VOCAL_MORPHER_WAVEFORM_SAWTOOTH;
				break;
		}

		OpenAL::alEffecti(this->identifier(), AL_VOCAL_MORPHER_WAVEFORM, def);
	}

	void
	VocalMorpher::setRate (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_VOCAL_MORPHER_MIN_RATE || value > AL_VOCAL_MORPHER_MAX_RATE )
		{
			TraceWarning{ClassId} << "Rate must be between " << AL_VOCAL_MORPHER_MIN_RATE << " and " << AL_VOCAL_MORPHER_MAX_RATE << '.';

			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_VOCAL_MORPHER_RATE, value);
	}

	VocalMorpher::Phoneme
	VocalMorpher::phonemeA () const noexcept
	{
		ALint value = 0;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA, &value);
		}

		return static_cast< Phoneme >(value);
	}

	int
	VocalMorpher::phonemeACoarseTuning () const noexcept
	{
		ALint value = 0;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEA_COARSE_TUNING, &value);
		}

		return value;
	}

	VocalMorpher::Phoneme
	VocalMorpher::phonemeB () const noexcept
	{
		ALint value = 0;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB, &value);
		}

		return static_cast< Phoneme >(value);
	}

	int
	VocalMorpher::phonemeBCoarseTuning () const noexcept
	{
		ALint value = 0;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffecti(this->identifier(), AL_VOCAL_MORPHER_PHONEMEB_COARSE_TUNING, &value);
		}

		return value;
	}

	VocalMorpher::WaveForm
	VocalMorpher::waveForm () const noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return WaveForm::Sinusoid;
		}

		ALint value = 0;

		OpenAL::alGetEffecti(this->identifier(), AL_VOCAL_MORPHER_WAVEFORM, &value);

		switch ( value )
		{
			case AL_VOCAL_MORPHER_WAVEFORM_TRIANGLE :
				return WaveForm::Triangle;

			case AL_VOCAL_MORPHER_WAVEFORM_SAWTOOTH :
				return WaveForm::SawTooth;

			case AL_VOCAL_MORPHER_WAVEFORM_SINUSOID :
			default:
				return WaveForm::Sinusoid;
		}
	}

	float
	VocalMorpher::rate () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffectf(this->identifier(), AL_VOCAL_MORPHER_RATE, &value);
		}

		return value;
	}
}
