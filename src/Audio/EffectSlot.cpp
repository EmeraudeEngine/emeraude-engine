/*
 * src/Audio/EffectSlot.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "EffectSlot.hpp"

/* Local inclusions. */
#include "OpenAL.EFX.hpp"
#include "Manager.hpp"
#include "Utility.hpp"
#include "Tracer.hpp"

namespace Emeraude::Audio
{
	using namespace Libraries;

	EffectSlot::EffectSlot () noexcept
	{
		if ( !EFX::isAvailable() )
		{
			Tracer::warning(ClassId, "OpenAL EFX is not supported !");

			return;
		}

		if ( !Manager::instance()->usable() )
		{
			*(this->identifierPointer()) = 1;

			return;
		}

		alFlushErrors();

		EFX::alGenAuxiliaryEffectSlots(1, this->identifierPointer());

		if ( alGetErrors("alGenAuxiliaryEffectSlots()", __FILE__, __LINE__) || this->identifier() <= 0 )
		{
			Tracer::error(ClassId, "Unable to generate auxiliary effect slot !");
		}
	}

	EffectSlot::~EffectSlot ()
	{
		if ( !EFX::isAvailable() )
		{
			return;
		}

		if ( this->isCreated() )
		{
			this->clearEffect();

			EFX::alDeleteAuxiliaryEffectSlots(1, this->identifierPointer());
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

		return EFX::alIsAuxiliaryEffectSlot(this->identifier()) == AL_TRUE;
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

		EFX::alAuxiliaryEffectSloti(this->identifier(), AL_EFFECTSLOT_EFFECT, static_cast< ALint >(effect->identifier()));

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
	EffectSlot::enable (const AbstractObject * source) const noexcept
	{
		alFlushErrors();

		auto slotName = static_cast< ALint >(this->identifier());

		if ( m_filter == nullptr )
		{
			alSource3i(source->identifier(), AL_AUXILIARY_SEND_FILTER, slotName, 0, AL_FILTER_NULL);
		}
		else
		{
			auto filterName = static_cast< ALint >(m_filter->identifier());

			alSource3i(source->identifier(), AL_AUXILIARY_SEND_FILTER, slotName, 1, filterName);
		}

		return !alGetErrors("alSource3i()", __FILE__, __LINE__);
	}

	void
	EffectSlot::disable (const AbstractObject * source) const noexcept
	{
		alSource3i(source->identifier(), AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL);
	}

	void
	EffectSlot::clearEffect () noexcept
	{
		EFX::alAuxiliaryEffectSloti(this->identifier(), AL_EFFECTSLOT_EFFECT, AL_NONE);

		m_effect.reset();
	}
}
