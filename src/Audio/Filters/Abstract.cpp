/*
 * src/Audio/Filters/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Audio/Manager.hpp"
#include "Audio/Utility.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio::Filters
{
	using namespace EmEn::Libs;

	constexpr auto TracerTag{"AbstractFilter"};

	Abstract::Abstract () noexcept
	{
		if ( !Manager::instance()->usable() )
		{
			return;
		}

		if ( !EFX::isAvailable() )
		{
			Tracer::warning(TracerTag, "OpenAL EFX is not supported !");

			return;
		}

		alFlushErrors();

		EFX::alGenFilters(1, this->identifierPointer());

		if ( alGetErrors("alGenFilters()", __FILE__, __LINE__) || this->identifier() <= 0 )
		{
			Tracer::error(TracerTag, "Unable to generate a filter identifier !");
		}
	}

	Abstract::~Abstract ()
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( this->isCreated() )
		{
			EFX::alDeleteFilters(1, this->identifierPointer());
		}

		this->resetIdentifier();
	}

	bool
	Abstract::isCreated () const noexcept
	{
		if ( !EFX::isAvailable() || this->identifier() <= 0 )
		{
			return false;
		}

		return EFX::alIsFilter(this->identifier()) == AL_TRUE;
	}
}
