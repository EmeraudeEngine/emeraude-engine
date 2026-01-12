/*
 * src/Audio/Effects/AutoWah.cpp
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

#include "AutoWah.hpp"

/* Local inclusions. */
#include "Audio/OpenALExtensions.hpp"
#include "Audio/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio::Effects
{
	using namespace Libs;

	AutoWah::AutoWah () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		OpenAL::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_AUTOWAH);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Auto-Wah effect !");
		}
	}

	void
	AutoWah::resetProperties () noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_ATTACK_TIME, AL_AUTOWAH_DEFAULT_ATTACK_TIME);
		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_RELEASE_TIME, AL_AUTOWAH_DEFAULT_RELEASE_TIME);
		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_RESONANCE, AL_AUTOWAH_DEFAULT_RESONANCE);
		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_PEAK_GAIN, AL_AUTOWAH_DEFAULT_PEAK_GAIN);
	}

	void
	AutoWah::setAttackTime (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_AUTOWAH_MIN_ATTACK_TIME || value > AL_AUTOWAH_MAX_ATTACK_TIME )
		{
			TraceWarning{ClassId} << "Attack time must be between " << AL_AUTOWAH_MIN_ATTACK_TIME << " and " << AL_AUTOWAH_MAX_ATTACK_TIME << '.';

			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_ATTACK_TIME, value);
	}

	void
	AutoWah::setReleaseTime (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_AUTOWAH_MIN_RELEASE_TIME || value > AL_AUTOWAH_MAX_RELEASE_TIME )
		{
			TraceWarning{ClassId} << "Release time must be between " << AL_AUTOWAH_MIN_RELEASE_TIME << " and " << AL_AUTOWAH_MAX_RELEASE_TIME << '.';

			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_RELEASE_TIME, value);
	}

	void
	AutoWah::setResonance (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_AUTOWAH_MIN_RESONANCE || value > AL_AUTOWAH_MAX_RESONANCE )
		{
			TraceWarning{ClassId} << "Resonance must be between " << AL_AUTOWAH_MIN_RESONANCE << " and " << AL_AUTOWAH_MAX_RESONANCE << '.';

			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_RESONANCE, value);
	}

	void
	AutoWah::setPeakGain (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_AUTOWAH_MIN_PEAK_GAIN || value > AL_AUTOWAH_MAX_PEAK_GAIN )
		{
			TraceWarning{ClassId} << "Peak gain must be between " << AL_AUTOWAH_MIN_PEAK_GAIN << " and " << AL_AUTOWAH_MAX_PEAK_GAIN << '.';

			return;
		}

		OpenAL::alEffectf(this->identifier(), AL_AUTOWAH_PEAK_GAIN, value);
	}

	float
	AutoWah::attackTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffectf(this->identifier(), AL_AUTOWAH_ATTACK_TIME, &value);
		}

		return value;
	}

	float
	AutoWah::releaseTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffectf(this->identifier(), AL_AUTOWAH_RELEASE_TIME, &value);
		}

		return value;
	}

	float
	AutoWah::resonance () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffectf(this->identifier(), AL_AUTOWAH_RESONANCE, &value);
		}

		return value;
	}

	float
	AutoWah::peakGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetEffectf(this->identifier(), AL_AUTOWAH_PEAK_GAIN, &value);
		}

		return value;
	}
}
