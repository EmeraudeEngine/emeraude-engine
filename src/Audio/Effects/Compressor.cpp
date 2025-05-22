/*
 * src/Audio/Effects/Compressor.cpp
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

#include "Compressor.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Audio/OpenAL.EFX.hpp"
#include "Audio/Utility.hpp"

namespace EmEn::Audio::Effects
{
	using namespace EmEn::Libs;

	Compressor::Compressor () noexcept
	{
		if ( this->identifier() == 0 )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_EFFECT_TYPE, AL_EFFECT_COMPRESSOR);

		if ( alGetErrors("alEffecti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to generate OpenAL Compressor effect !");
		}
	}

	void
	Compressor::resetProperties () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_COMPRESSOR_ONOFF, AL_COMPRESSOR_DEFAULT_ONOFF);
	}

	void
	Compressor::enable (bool state) noexcept
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		EFX::alEffecti(this->identifier(), AL_COMPRESSOR_ONOFF, state ? AL_TRUE : AL_FALSE);
	}

	bool
	Compressor::isEnabled () const noexcept
	{
		ALint value = 0;

		if ( EFX::isAvailable() )
		{
			EFX::alGetEffecti(this->identifier(), AL_COMPRESSOR_ONOFF, &value);
		}

		return value > 0;
	}
}
