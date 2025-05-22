/*
 * src/Audio/Effects/EAXReverb.cpp
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

#include "EAXReverb.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	EAXReverb::EAXReverb () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL EAX Reverb effect !");
		}
	}

	void
	EAXReverb::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DENSITY, AL_EAXREVERB_DEFAULT_DENSITY);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DIFFUSION, AL_EAXREVERB_DEFAULT_DIFFUSION);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAIN, AL_EAXREVERB_DEFAULT_GAIN);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAINHF, AL_EAXREVERB_DEFAULT_GAINHF);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAINLF, AL_EAXREVERB_DEFAULT_GAINLF);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_TIME, AL_EAXREVERB_DEFAULT_DECAY_TIME);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_HFRATIO, AL_EAXREVERB_DEFAULT_DECAY_HFRATIO);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_LFRATIO, AL_EAXREVERB_DEFAULT_DECAY_LFRATIO);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_GAIN, AL_EAXREVERB_DEFAULT_REFLECTIONS_GAIN);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_DELAY, AL_EAXREVERB_DEFAULT_REFLECTIONS_DELAY);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_PAN, AL_EAXREVERB_DEFAULT_REFLECTIONS_PAN_XYZ);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_GAIN, AL_EAXREVERB_DEFAULT_LATE_REVERB_GAIN);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_DELAY, AL_EAXREVERB_DEFAULT_LATE_REVERB_DELAY);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_PAN, AL_EAXREVERB_DEFAULT_LATE_REVERB_PAN_XYZ);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ECHO_TIME, AL_EAXREVERB_DEFAULT_ECHO_TIME);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ECHO_DEPTH, AL_EAXREVERB_DEFAULT_ECHO_DEPTH);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_MODULATION_TIME, AL_EAXREVERB_DEFAULT_MODULATION_TIME);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_MODULATION_DEPTH, AL_EAXREVERB_DEFAULT_MODULATION_DEPTH);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_AIR_ABSORPTION_GAINHF, AL_EAXREVERB_DEFAULT_AIR_ABSORPTION_GAINHF);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_HFREFERENCE, AL_EAXREVERB_DEFAULT_HFREFERENCE);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LFREFERENCE, AL_EAXREVERB_DEFAULT_LFREFERENCE);
		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, AL_EAXREVERB_DEFAULT_ROOM_ROLLOFF_FACTOR);
		EFX::alEffecti(this->identifier(), AL_EAXREVERB_DECAY_HFLIMIT, AL_EAXREVERB_DEFAULT_DECAY_HFLIMIT);
	}

	void
	EAXReverb::setDensity (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_DENSITY || value > AL_EAXREVERB_MAX_DENSITY )
		{
			TraceWarning{ClassId} << "Density must be between " << AL_EAXREVERB_MIN_DENSITY << " and " << AL_EAXREVERB_MAX_DENSITY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DENSITY, value);
	}

	void
	EAXReverb::setDiffusion (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_DIFFUSION || value > AL_EAXREVERB_MAX_DIFFUSION )
		{
			TraceWarning{ClassId} << "Diffusion must be between " << AL_EAXREVERB_MIN_DIFFUSION << " and " << AL_EAXREVERB_MAX_DIFFUSION << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DIFFUSION, value);
	}

	void
	EAXReverb::setGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_GAIN || value > AL_EAXREVERB_MAX_GAIN )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_EAXREVERB_MIN_GAIN << " and " << AL_EAXREVERB_MAX_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAIN, value);
	}

	void
	EAXReverb::setGainHF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_GAINHF || value > AL_EAXREVERB_MAX_GAINHF )
		{
			TraceWarning{ClassId} << "Gain HF must be between " << AL_EAXREVERB_MIN_GAINHF << " and " << AL_EAXREVERB_MAX_GAINHF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAINHF, value);
	}

	void
	EAXReverb::setGainLF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_GAINLF || value > AL_EAXREVERB_MAX_GAINLF )
		{
			TraceWarning{ClassId} << "Gain LF must be between " << AL_EAXREVERB_MIN_GAINLF << " and " << AL_EAXREVERB_MAX_GAINLF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_GAINLF, value);
	}

	void
	EAXReverb::setDecayTime (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_DECAY_TIME || value > AL_EAXREVERB_MAX_DECAY_TIME )
		{
			TraceWarning{ClassId} << "Decay time must be between " << AL_EAXREVERB_MIN_DECAY_TIME << " and " << AL_EAXREVERB_MAX_DECAY_TIME << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_TIME, value);
	}

	void
	EAXReverb::setDecayHFRatio (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_DECAY_HFRATIO || value > AL_EAXREVERB_MAX_DECAY_HFRATIO )
		{
			TraceWarning{ClassId} << "Decay HF ratio must be between " << AL_EAXREVERB_MIN_DECAY_HFRATIO << " and " << AL_EAXREVERB_MAX_DECAY_HFRATIO << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_HFRATIO, value);
	}

	void
	EAXReverb::setDecayLFRatio (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_DECAY_LFRATIO || value > AL_EAXREVERB_MAX_DECAY_LFRATIO )
		{
			TraceWarning{ClassId} << "Decay LF ratio must be between " << AL_EAXREVERB_MIN_DECAY_LFRATIO << " and " << AL_EAXREVERB_MAX_DECAY_LFRATIO << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_DECAY_LFRATIO, value);
	}

	void
	EAXReverb::setReflectionsGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_REFLECTIONS_GAIN || value > AL_EAXREVERB_MAX_REFLECTIONS_GAIN )
		{
			TraceWarning{ClassId} << "Reflections gain must be between " << AL_EAXREVERB_MIN_REFLECTIONS_GAIN << " and " << AL_EAXREVERB_MAX_REFLECTIONS_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_GAIN, value);
	}

	void
	EAXReverb::setReflectionsDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_REFLECTIONS_DELAY || value > AL_EAXREVERB_MAX_REFLECTIONS_DELAY )
		{
			TraceWarning{ClassId} << "Reflections delay must be between " << AL_EAXREVERB_MIN_REFLECTIONS_DELAY << " and " << AL_EAXREVERB_MAX_REFLECTIONS_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_DELAY, value);
	}

	void
	EAXReverb::setReflectionsPan (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_PAN, value);
	}

	void
	EAXReverb::setLateGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_LATE_REVERB_GAIN || value > AL_EAXREVERB_MAX_LATE_REVERB_GAIN )
		{
			TraceWarning{ClassId} << "Late reverb gain must be between " << AL_EAXREVERB_MIN_LATE_REVERB_GAIN << " and " << AL_EAXREVERB_MAX_LATE_REVERB_GAIN << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_GAIN, value);
	}

	void
	EAXReverb::setLateDelay (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_LATE_REVERB_DELAY || value > AL_EAXREVERB_MAX_LATE_REVERB_DELAY )
		{
			TraceWarning{ClassId} << "Late reverb delay must be between " << AL_EAXREVERB_MIN_LATE_REVERB_DELAY << " and " << AL_EAXREVERB_MAX_LATE_REVERB_DELAY << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_DELAY, value);
	}

	void
	EAXReverb::setLatePan (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_PAN, value);
	}

	void
	EAXReverb::setEchoTime (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_ECHO_TIME || value > AL_EAXREVERB_MAX_ECHO_TIME )
		{
			TraceWarning{ClassId} << "Echo time must be between " << AL_EAXREVERB_MIN_ECHO_TIME << " and " << AL_EAXREVERB_MAX_ECHO_TIME << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ECHO_TIME, value);
	}

	void
	EAXReverb::setEchoDepth (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_ECHO_DEPTH || value > AL_EAXREVERB_MAX_ECHO_DEPTH )
		{
			TraceWarning{ClassId} << "Echo depth must be between " << AL_EAXREVERB_MIN_ECHO_DEPTH << " and " << AL_EAXREVERB_MAX_ECHO_DEPTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ECHO_DEPTH, value);
	}

	void
	EAXReverb::setModulationTime (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_MODULATION_TIME || value > AL_EAXREVERB_MAX_MODULATION_TIME )
		{
			TraceWarning{ClassId} << "Modulation time must be between " << AL_EAXREVERB_MIN_MODULATION_TIME << " and " << AL_EAXREVERB_MAX_MODULATION_TIME << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_MODULATION_TIME, value);
	}

	void
	EAXReverb::setModulationDepth (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_MODULATION_DEPTH || value > AL_EAXREVERB_MAX_MODULATION_DEPTH )
		{
			TraceWarning{ClassId} << "Modulation depth must be between " << AL_EAXREVERB_MIN_MODULATION_DEPTH << " and " << AL_EAXREVERB_MAX_MODULATION_DEPTH << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_MODULATION_DEPTH, value);
	}

	void
	EAXReverb::setAirAbsorptionGainHF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}
		if ( value < AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF || value > AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF )
		{
			TraceWarning{ClassId} << "Air absorption gain HF must be between " << AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF << " and " << AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_AIR_ABSORPTION_GAINHF, value);
	}

	void
	EAXReverb::setHFReference (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_HFREFERENCE || value > AL_EAXREVERB_MAX_HFREFERENCE )
		{
			TraceWarning{ClassId} << "HF reference must be between " << AL_EAXREVERB_MIN_HFREFERENCE << " and " << AL_EAXREVERB_MAX_HFREFERENCE << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_HFREFERENCE, value);
	}

	void
	EAXReverb::setLFReference (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_LFREFERENCE || value > AL_EAXREVERB_MAX_LFREFERENCE )
		{
			TraceWarning{ClassId} << "LF reference must be between " << AL_EAXREVERB_MIN_LFREFERENCE << " and " << AL_EAXREVERB_MAX_LFREFERENCE << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_LFREFERENCE, value);
	}

	void
	EAXReverb::setRoomRollOffFactor (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR || value > AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR )
		{
			TraceWarning{ClassId} << "Room Roll-off factor must be between " << AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR << " and " << AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR << '.';

			return;
		}

		EFX::alEffectf(this->identifier(), AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, value);
	}

	void
	EAXReverb::setDecayHFLimit (bool state) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EAXREVERB_DECAY_HFLIMIT, state ? AL_TRUE : AL_FALSE);
	}

	float
	EAXReverb::density () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_DENSITY, &value);
		}

		return value;
	}

	float
	EAXReverb::diffusion () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_DIFFUSION, &value);
		}

		return value;
	}

	float
	EAXReverb::gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_GAIN, &value);
		}

		return value;
	}

	float
	EAXReverb::gainHF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_GAINHF, &value);
		}

		return value;
	}

	float
	EAXReverb::gainLF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_GAINLF, &value);
		}

		return value;
	}

	float
	EAXReverb::decayTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_DECAY_TIME, &value);
		}

		return value;
	}

	float
	EAXReverb::decayHFRatio () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_DECAY_HFRATIO, &value);
		}

		return value;
	}

	float
	EAXReverb::decayLFRatio () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_DECAY_LFRATIO, &value);
		}

		return value;
	}


	float
	EAXReverb::reflectionsGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_GAIN, &value);
		}

		return value;
	}

	float
	EAXReverb::reflectionsDelay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_DELAY, &value);
		}

		return value;
	}

	float
	EAXReverb::reflectionsPan () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_REFLECTIONS_PAN, &value);
		}

		return value;
	}

	float
	EAXReverb::lateGain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_GAIN, &value);
		}

		return value;
	}

	float
	EAXReverb::lateDelay () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_DELAY, &value);
		}

		return value;
	}

	float
	EAXReverb::latePan () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_LATE_REVERB_PAN, &value);
		}

		return value;
	}

	float
	EAXReverb::echoTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_ECHO_TIME, &value);
		}

		return value;
	}

	float
	EAXReverb::echoDepth () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_ECHO_DEPTH, &value);
		}

		return value;
	}

	float
	EAXReverb::modulationTime () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_MODULATION_TIME, &value);
		}

		return value;
	}

	float
	EAXReverb::modulationDepth () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_ECHO_DEPTH, &value);
		}

		return value;
	}

	float
	EAXReverb::airAbsorptionGainHF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_AIR_ABSORPTION_GAINHF, &value);
		}

		return value;
	}

	float
	EAXReverb::HFReference () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_HFREFERENCE, &value);
		}

		return value;
	}

	float
	EAXReverb::LFReference () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_LFREFERENCE, &value);
		}

		return value;
	}

	float
	EAXReverb::roomRollOffFactor () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffectf(this->identifier(), AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, &value);
		}

		return value;
	}

	bool
	EAXReverb::decayHFLimit () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_EAXREVERB_DECAY_HFLIMIT, &value);
		}

		return value > 0;
	}
}
