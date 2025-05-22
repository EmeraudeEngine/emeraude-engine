/*
 * src/Audio/Effects/Reverb.cpp
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

#include "Reverb.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	Reverb::Reverb () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Reverb effect !");
		}
	}

	void
	Reverb::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_DENSITY, AL_REVERB_DEFAULT_DENSITY);
		EFX::alEffectf(this->identifier(), AL_REVERB_DIFFUSION, AL_REVERB_DEFAULT_DIFFUSION);
		EFX::alEffectf(this->identifier(), AL_REVERB_GAIN, AL_REVERB_DEFAULT_GAIN);
		EFX::alEffectf(this->identifier(), AL_REVERB_GAINHF, AL_REVERB_DEFAULT_GAINHF);
		EFX::alEffectf(this->identifier(), AL_REVERB_DECAY_TIME, AL_REVERB_DEFAULT_DECAY_TIME);
		EFX::alEffectf(this->identifier(), AL_REVERB_DECAY_HFRATIO, AL_REVERB_DEFAULT_DECAY_HFRATIO);
		EFX::alEffectf(this->identifier(), AL_REVERB_REFLECTIONS_GAIN, AL_REVERB_DEFAULT_REFLECTIONS_GAIN);
		EFX::alEffectf(this->identifier(), AL_REVERB_REFLECTIONS_DELAY, AL_REVERB_DEFAULT_REFLECTIONS_DELAY);
		EFX::alEffectf(this->identifier(), AL_REVERB_LATE_REVERB_GAIN, AL_REVERB_DEFAULT_LATE_REVERB_GAIN);
		EFX::alEffectf(this->identifier(), AL_REVERB_LATE_REVERB_DELAY, AL_REVERB_DEFAULT_LATE_REVERB_DELAY);
		EFX::alEffectf(this->identifier(), AL_REVERB_AIR_ABSORPTION_GAINHF, AL_REVERB_DEFAULT_AIR_ABSORPTION_GAINHF);
		EFX::alEffectf(this->identifier(), AL_REVERB_ROOM_ROLLOFF_FACTOR, AL_REVERB_DEFAULT_ROOM_ROLLOFF_FACTOR);
		EFX::alEffecti(this->identifier(), AL_REVERB_DECAY_HFLIMIT, AL_REVERB_DEFAULT_DECAY_HFLIMIT);
	}

	void
	Reverb::setDensity (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_DENSITY || value > AL_REVERB_MAX_DENSITY )
		{
			TraceWarning{ClassId} << "Density must be between " << AL_REVERB_MIN_DENSITY << " and " << AL_REVERB_MAX_DENSITY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_DENSITY, value);
	}

	void
	Reverb::setDiffusion (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_DIFFUSION || value > AL_REVERB_MAX_DIFFUSION )
		{
			TraceWarning{ClassId} << "Diffusion must be between " << AL_REVERB_MIN_DIFFUSION << " and " << AL_REVERB_MAX_DIFFUSION << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_DIFFUSION, value);
	}

	void
	Reverb::setGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_GAIN || value > AL_REVERB_MAX_GAIN )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_REVERB_MIN_GAIN << " and " << AL_REVERB_MAX_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_GAIN, value);
	}

	void
	Reverb::setGainHF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_GAINHF || value > AL_REVERB_MAX_GAINHF )
		{
			TraceWarning{ClassId} << "Gain HF must be between " << AL_REVERB_MIN_GAINHF << " and " << AL_REVERB_MAX_GAINHF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_GAINHF, value);
	}

	void
	Reverb::setDecayTime (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_DECAY_TIME || value > AL_REVERB_MAX_DECAY_TIME )
		{
			TraceWarning{ClassId} << "Decay time must be between " << AL_REVERB_MIN_DECAY_TIME << " and " << AL_REVERB_MAX_DECAY_TIME << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_DECAY_TIME, value);
	}

	void
	Reverb::setDecayHFRatio (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_DECAY_HFRATIO || value > AL_REVERB_MAX_DECAY_HFRATIO )
		{
			TraceWarning{ClassId} << "Decay HF ratio must be between " << AL_REVERB_MIN_DECAY_HFRATIO << " and " << AL_REVERB_MAX_DECAY_HFRATIO << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_DECAY_HFRATIO, value);
	}

	void
	Reverb::setReflectionsGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_REFLECTIONS_GAIN || value > AL_REVERB_MAX_REFLECTIONS_GAIN )
		{
			TraceWarning{ClassId} << "Reflections gain must be between " << AL_REVERB_MIN_REFLECTIONS_GAIN << " and " << AL_REVERB_MAX_REFLECTIONS_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_REFLECTIONS_GAIN, value);
	}

	void
	Reverb::setReflectionsDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_REFLECTIONS_DELAY || value > AL_REVERB_MAX_REFLECTIONS_DELAY )
		{
			TraceWarning{ClassId} << "Reflections gain must be between " << AL_REVERB_MIN_REFLECTIONS_DELAY << " and " << AL_REVERB_MAX_REFLECTIONS_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_REFLECTIONS_DELAY, value);
	}

	void
	Reverb::setLateGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_LATE_REVERB_GAIN || value > AL_REVERB_MAX_LATE_REVERB_GAIN )
		{
			TraceWarning{ClassId} << "Late reverb gain must be between " << AL_REVERB_MIN_LATE_REVERB_GAIN << " and " << AL_REVERB_MAX_LATE_REVERB_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_LATE_REVERB_GAIN, value);
	}

	void
	Reverb::setLateDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_LATE_REVERB_DELAY || value > AL_REVERB_MAX_LATE_REVERB_DELAY )
		{
			TraceWarning{ClassId} << "Late reverb delay must be between " << AL_REVERB_MIN_LATE_REVERB_DELAY << " and " << AL_REVERB_MAX_LATE_REVERB_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_LATE_REVERB_DELAY, value);
	}

	void
	Reverb::setAirAbsorptionGainHF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_AIR_ABSORPTION_GAINHF || value > AL_REVERB_MAX_AIR_ABSORPTION_GAINHF )
		{
			TraceWarning{ClassId} << "Air absorption gain HF must be between " << AL_REVERB_MIN_AIR_ABSORPTION_GAINHF << " and " << AL_REVERB_MAX_AIR_ABSORPTION_GAINHF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_REVERB_AIR_ABSORPTION_GAINHF, value);
	}

	void
	Reverb::setRoomRollOffFactor (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_REVERB_MIN_ROOM_ROLLOFF_FACTOR || value > AL_REVERB_MAX_ROOM_ROLLOFF_FACTOR )
		{
			TraceWarning{ClassId} << "Room roll-off factor must be between " << AL_REVERB_MIN_ROOM_ROLLOFF_FACTOR << " and " << AL_REVERB_MAX_ROOM_ROLLOFF_FACTOR << '.';

			return;
		}


		EFX::alEffectf(this->identifier(), AL_REVERB_ROOM_ROLLOFF_FACTOR, value);
	}

	void
	Reverb::setDecayHFLimit (bool state) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_REVERB_DECAY_HFLIMIT, state ? AL_TRUE : AL_FALSE);
	}

	float
	Reverb::density () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_DENSITY, &value);
		}

		return value;
	}

	float
	Reverb::diffusion () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_DIFFUSION, &value);
		}

		return value;
	}

	float
	Reverb::gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_GAIN, &value);
		}

		return value;
	}

	float
	Reverb::gainHF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_GAINHF, &value);
		}

		return value;
	}

	float
	Reverb::decayTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_DECAY_TIME, &value);
		}

		return value;
	}

	float
	Reverb::decayHFRatio () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_DECAY_HFRATIO, &value);
		}

		return value;
	}

	float
	Reverb::reflectionsGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_REFLECTIONS_GAIN, &value);
		}

		return value;
	}

	float
	Reverb::reflectionsDelay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_REFLECTIONS_DELAY, &value);
		}

		return value;
	}

	float
	Reverb::lateGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_LATE_REVERB_GAIN, &value);
		}

		return value;
	}

	float
	Reverb::lateDelay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_LATE_REVERB_DELAY, &value);
		}

		return value;
	}

	float
	Reverb::airAbsorptionGainHF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_AIR_ABSORPTION_GAINHF, &value);
		}

		return value;
	}

	float
	Reverb::roomRollOffFactor () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_REVERB_ROOM_ROLLOFF_FACTOR, &value);
		}

		return value;
	}

	bool
	Reverb::decayHFLimit () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_REVERB_DECAY_HFLIMIT, &value);
		}

		return value > 0;
	}
}
