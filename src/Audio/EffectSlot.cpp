/*
 * src/Audio/EffectSlot.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "EffectSlot.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <string>

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/efx.h"

/* Local inclusions. */
#include "Audio/AbstractObject.hpp"
#include "Audio/Effects/Abstract.hpp"
#include "Audio/Filters/Abstract.hpp"
#include "Manager.hpp"
#include "NameableTrait.hpp"
#include "OpenALExtensions.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Audio
{
	using namespace Base;

	EffectSlot::EffectSlot () noexcept
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			Tracer::warning(ClassId, "OpenAL EFX is not supported !");

			return;
		}

		if ( !Manager::isAudioSystemAvailable() )
		{
			*(this->identifierPointer()) = 1;

			return;
		}

		alFlushErrors();

		OpenAL::alGenAuxiliaryEffectSlots(1, this->identifierPointer());

		if ( alGetErrors("alGenAuxiliaryEffectSlots()", __FILE__, __LINE__) || this->identifier() <= 0 )
		{
			Tracer::error(ClassId, "Unable to generate auxiliary effect slot !");
		}
	}

	EffectSlot::~EffectSlot ()
	{
		if ( !OpenAL::isEFXAvailable() )
		{
			return;
		}

		if ( this->isCreated() )
		{
			this->clearEffect();

			OpenAL::alDeleteAuxiliaryEffectSlots(1, this->identifierPointer());
		}

		this->clearFilter();

		this->resetIdentifier();
	}

	bool
	EffectSlot::isCreated () const noexcept
	{
		if ( this->identifier() <= 0 )
		{
			return false;
		}

		return OpenAL::alIsAuxiliaryEffectSlot(this->identifier()) == AL_TRUE;
	}

	bool
	EffectSlot::setEffect (const std::shared_ptr< Effects::Abstract > & effect) noexcept
	{
		alFlushErrors();

		if ( effect == nullptr )
		{
			Tracer::error(ClassId, "Effect is a null pointer !");

			return false;
		}

		if ( !effect->isCreated() )
		{
			return false;
		}

		OpenAL::alAuxiliaryEffectSloti(this->identifier(), AL_EFFECTSLOT_EFFECT, static_cast< ALint >(effect->identifier()));

		if ( alGetErrors("alAuxiliaryEffectSloti()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to bind effect to effect slot !");

			return false;
		}

		m_effect = effect;

		return true;
	}

	bool
	EffectSlot::setFilter (const std::shared_ptr< Filters::Abstract > & filter) noexcept
	{
		if ( filter == nullptr )
		{
			Tracer::error(ClassId, "Filter is a null pointer !");

			return false;
		}

		if ( !filter->isCreated() )
		{
			return false;
		}

		m_filter = filter;

		return true;
	}

	bool
	EffectSlot::enable (const AbstractObject * source, int send) const noexcept
	{
		if ( source == nullptr )
		{
			Tracer::error(ClassId, "The source is a null pointer !");

			return false;
		}

		alFlushErrors();

		/* NOTE: The send is the one asked for, with or without a filter. It was 0 without a filter and 1 with one,
		 * whatever the caller's channel: two effects of one source landed on the same send. */
		const auto slotName = static_cast< ALint >(this->identifier());
		const auto filterName = m_filter != nullptr ? static_cast< ALint >(m_filter->identifier()) : AL_FILTER_NULL;

		alSource3i(source->identifier(), AL_AUXILIARY_SEND_FILTER, slotName, send, filterName);

		return !alGetErrors("alSource3i()", __FILE__, __LINE__);
	}

	void
	EffectSlot::disable (const AbstractObject * source, int send) noexcept
	{
		if ( source == nullptr )
		{
			return;
		}

		alSource3i(source->identifier(), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, send, AL_FILTER_NULL);
	}

	void
	EffectSlot::setGain (float gain) noexcept
	{
		if ( !OpenAL::isEFXAvailable() || !Manager::isAudioSystemAvailable() || !std::isfinite(gain) )
		{
			return;
		}

		OpenAL::alAuxiliaryEffectSlotf(this->identifier(), AL_EFFECTSLOT_GAIN, std::clamp(gain, 0.0F, 1.0F));
	}

	void
	EffectSlot::clearEffect () noexcept
	{
		OpenAL::alAuxiliaryEffectSloti(this->identifier(), AL_EFFECTSLOT_EFFECT, AL_NONE);

		m_effect.reset();
	}
}
