/*
 * src/Audio/Filters/HighPass.cpp
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

#include "HighPass.hpp"

/* Local inclusions. */
#include "Audio/OpenALExtensions.hpp"
#include "Audio/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio::Filters
{
	using namespace Libs;

	HighPass::HighPass () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		OpenAL::alFilteri(this->identifier(), AL_FILTER_TYPE, AL_FILTER_HIGHPASS);

		if ( alGetErrors("alFilteri()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL High-Pass filter !");
		}
	}

	void
	HighPass::resetProperties () noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		OpenAL::alFilterf(this->identifier(), AL_HIGHPASS_GAIN, AL_HIGHPASS_DEFAULT_GAIN);
		OpenAL::alFilterf(this->identifier(), AL_HIGHPASS_GAINLF, AL_HIGHPASS_DEFAULT_GAINLF);
	}

	void
	HighPass::setGain (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_HIGHPASS_MIN_GAIN || value > AL_HIGHPASS_MAX_GAIN )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_HIGHPASS_MIN_GAIN << " and " << AL_HIGHPASS_MAX_GAIN << '.';

			return;
		}

		OpenAL::alFilterf(this->identifier(), AL_HIGHPASS_GAIN, value);
	}

	void
	HighPass::setGainLF (float value) noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( value < AL_HIGHPASS_MIN_GAINLF || value > AL_HIGHPASS_MAX_GAINLF )
		{
			TraceWarning{ClassId} << "Gain must be between " << AL_HIGHPASS_MIN_GAINLF << " and " << AL_HIGHPASS_MAX_GAINLF << '.';

			return;
		}

		OpenAL::alFilterf(this->identifier(), AL_HIGHPASS_GAINLF, value);
	}

	float
	HighPass::gain () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetFilterf(this->identifier(), AL_HIGHPASS_GAIN, &value);
		}

		return value;
	}

	float
	HighPass::gainLF () const noexcept
	{
		ALfloat value = 0.0F;

		if ( OpenAL::isEFXAvailable() )
		{
			OpenAL::alGetFilterf(this->identifier(), AL_HIGHPASS_GAINLF, &value);
		}

		return value;
	}
}
