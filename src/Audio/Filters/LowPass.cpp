/*
 * src/Audio/Filters/LowPass.cpp
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

#include "LowPass.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Filters
{
	using namespace EmEn::Libs;

	LowPass::LowPass () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alFilteri(this->identifier(), AL_FILTER_TYPE, AL_FILTER_LOWPASS);

		if ( alGetErrors("alFilteri()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Low-Pass filter !");
		}
	}

	void
	LowPass::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alFilterf(this->identifier(), AL_LOWPASS_GAIN, AL_LOWPASS_DEFAULT_GAIN);
		EFX::alFilterf(this->identifier(), AL_LOWPASS_GAINHF, AL_LOWPASS_DEFAULT_GAINHF);
	}

	void
	LowPass::setGain (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_LOWPASS_MIN_GAIN || value > AL_LOWPASS_MAX_GAIN )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_LOWPASS_MIN_GAIN << " and " << AL_LOWPASS_MAX_GAIN << '.';

			return;
		}

		EFX::alFilterf(this->identifier(), AL_LOWPASS_GAIN, value);
	}

	void
	LowPass::setGainHF (float value) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( value < AL_LOWPASS_MIN_GAINHF || value > AL_LOWPASS_MAX_GAINHF )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_LOWPASS_MIN_GAINHF << " and " << AL_LOWPASS_MAX_GAINHF << '.';

			return;
		}

		EFX::alFilterf(this->identifier(), AL_LOWPASS_GAINHF, value);
	}

	float
	LowPass::gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetFilterf(this->identifier(), AL_LOWPASS_GAIN, &value);
		}

		return value;
	}

	float
	LowPass::gainHF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( EFX::isAvailable() )
		{
			EFX::alGetFilterf(this->identifier(), AL_LOWPASS_GAINHF, &value);
		}

		return value;
	}
}
