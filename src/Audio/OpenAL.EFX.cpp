/*
 * src/Audio/OpenAL.EFX.cpp
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

#include "OpenAL.EFX.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Audio
{
	/* Effect objects */
	LPALGENEFFECTS EFX::alGenEffects{nullptr};
	LPALDELETEEFFECTS EFX::alDeleteEffects{nullptr};
	LPALISEFFECT EFX::alIsEffect{nullptr};
	LPALEFFECTI EFX::alEffecti{nullptr};
	LPALEFFECTIV EFX::alEffectiv{nullptr};
	LPALEFFECTF EFX::alEffectf{nullptr};
	LPALEFFECTFV EFX::alEffectfv{nullptr};
	LPALGETEFFECTI EFX::alGetEffecti{nullptr};
	LPALGETEFFECTIV EFX::alGetEffectiv{nullptr};
	LPALGETEFFECTF EFX::alGetEffectf{nullptr};
	LPALGETEFFECTFV EFX::alGetEffectfv{nullptr};

	/* Filter objects */
	LPALGENFILTERS EFX::alGenFilters{nullptr};
	LPALDELETEFILTERS EFX::alDeleteFilters{nullptr};
	LPALISFILTER EFX::alIsFilter{nullptr};
	LPALFILTERI EFX::alFilteri{nullptr};
	LPALFILTERIV EFX::alFilteriv{nullptr};
	LPALFILTERF EFX::alFilterf{nullptr};
	LPALFILTERFV EFX::alFilterfv{nullptr};
	LPALGETFILTERI EFX::alGetFilteri{nullptr};
	LPALGETFILTERIV EFX::alGetFilteriv{nullptr};
	LPALGETFILTERF EFX::alGetFilterf{nullptr};
	LPALGETFILTERFV EFX::alGetFilterfv{nullptr};

	/* Auxiliary slot object */
	LPALGENAUXILIARYEFFECTSLOTS EFX::alGenAuxiliaryEffectSlots{nullptr};
	LPALDELETEAUXILIARYEFFECTSLOTS EFX::alDeleteAuxiliaryEffectSlots{nullptr};
	LPALISAUXILIARYEFFECTSLOT EFX::alIsAuxiliaryEffectSlot{nullptr};
	LPALAUXILIARYEFFECTSLOTI EFX::alAuxiliaryEffectSloti{nullptr};
	LPALAUXILIARYEFFECTSLOTIV EFX::alAuxiliaryEffectSlotiv{nullptr};
	LPALAUXILIARYEFFECTSLOTF EFX::alAuxiliaryEffectSlotf{nullptr};
	LPALAUXILIARYEFFECTSLOTFV EFX::alAuxiliaryEffectSlotfv{nullptr};
	LPALGETAUXILIARYEFFECTSLOTI EFX::alGetAuxiliaryEffectSloti{nullptr};
	LPALGETAUXILIARYEFFECTSLOTIV EFX::alGetAuxiliaryEffectSlotiv{nullptr};
	LPALGETAUXILIARYEFFECTSLOTF EFX::alGetAuxiliaryEffectSlotf{nullptr};
	LPALGETAUXILIARYEFFECTSLOTFV EFX::alGetAuxiliaryEffectSlotfv{nullptr};

	EFX::EFX (ALCdevice * device) noexcept
		: m_device(device)
	{
		/* Checks EFX extensions */
		if ( alcIsExtensionPresent(m_device, "ALC_EXT_EFX") == ALC_FALSE )
		{
			Tracer::warning("EFX", "This platform doesn't support OpenAL EFX extension !");

			return;
		}

		/* Checks max auxiliary sends. */
		alcGetIntegerv(m_device, ALC_MAX_AUXILIARY_SENDS, 1, &s_maxAuxiliarySends);

		if ( s_maxAuxiliarySends == 0 )
		{
			return;
		}

		s_isAvailable = true;

		/* Register EFX functions */
		alGenEffects = reinterpret_cast< LPALGENEFFECTS >(alGetProcAddress("alGenEffects"));
		alDeleteEffects = reinterpret_cast< LPALDELETEEFFECTS >(alGetProcAddress("alDeleteEffects"));
		alIsEffect = reinterpret_cast< LPALISEFFECT >(alGetProcAddress("alIsEffect"));
		alEffecti = reinterpret_cast< LPALEFFECTI >(alGetProcAddress("alEffecti"));
		alEffectiv = reinterpret_cast< LPALEFFECTIV >(alGetProcAddress("alEffectiv"));
		alEffectf = reinterpret_cast< LPALEFFECTF >(alGetProcAddress("alEffectf"));
		alEffectfv = reinterpret_cast< LPALEFFECTFV >(alGetProcAddress("alEffectfv"));
		alGetEffecti = reinterpret_cast< LPALGETEFFECTI >(alGetProcAddress("alGetEffecti"));
		alGetEffectiv = reinterpret_cast< LPALGETEFFECTIV >(alGetProcAddress("alGetEffectiv"));
		alGetEffectf = reinterpret_cast< LPALGETEFFECTF >(alGetProcAddress("alGetEffectf"));
		alGetEffectfv = reinterpret_cast< LPALGETEFFECTFV >(alGetProcAddress("alGetEffectfv"));

		alGenFilters = reinterpret_cast< LPALGENFILTERS >(alGetProcAddress("alGenFilters"));
		alDeleteFilters = reinterpret_cast< LPALDELETEFILTERS >(alGetProcAddress("alDeleteFilters"));
		alIsFilter = reinterpret_cast< LPALISFILTER >(alGetProcAddress("alIsFilter"));
		alFilteri = reinterpret_cast< LPALFILTERI >(alGetProcAddress("alFilteri"));
		alFilteriv = reinterpret_cast< LPALFILTERIV >(alGetProcAddress("alFilteriv"));
		alFilterf = reinterpret_cast< LPALFILTERF >(alGetProcAddress("alFilterf"));
		alFilterfv = reinterpret_cast< LPALFILTERFV >(alGetProcAddress("alFilterfv"));
		alGetFilteri = reinterpret_cast< LPALGETFILTERI >(alGetProcAddress("alGetFilteri"));
		alGetFilteriv = reinterpret_cast< LPALGETFILTERIV >(alGetProcAddress("alGetFilteriv"));
		alGetFilterf = reinterpret_cast< LPALGETFILTERF >(alGetProcAddress("alGetFilterf"));
		alGetFilterfv = reinterpret_cast< LPALGETFILTERFV >(alGetProcAddress("alGetFilterfv"));

		alGenAuxiliaryEffectSlots = reinterpret_cast< LPALGENAUXILIARYEFFECTSLOTS >(alGetProcAddress("alGenAuxiliaryEffectSlots"));
		alDeleteAuxiliaryEffectSlots = reinterpret_cast< LPALDELETEAUXILIARYEFFECTSLOTS >(alGetProcAddress("alDeleteAuxiliaryEffectSlots"));
		alIsAuxiliaryEffectSlot = reinterpret_cast< LPALISAUXILIARYEFFECTSLOT >(alGetProcAddress("alIsAuxiliaryEffectSlot"));
		alAuxiliaryEffectSloti = reinterpret_cast< LPALAUXILIARYEFFECTSLOTI >(alGetProcAddress("alAuxiliaryEffectSloti"));
		alAuxiliaryEffectSlotiv = reinterpret_cast< LPALAUXILIARYEFFECTSLOTIV >(alGetProcAddress("alAuxiliaryEffectSlotiv"));
		alAuxiliaryEffectSlotf = reinterpret_cast< LPALAUXILIARYEFFECTSLOTF >(alGetProcAddress("alAuxiliaryEffectSlotf"));
		alAuxiliaryEffectSlotfv = reinterpret_cast< LPALAUXILIARYEFFECTSLOTFV >(alGetProcAddress("alAuxiliaryEffectSlotfv"));
		alGetAuxiliaryEffectSloti = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTI >(alGetProcAddress("alGetAuxiliaryEffectSloti"));
		alGetAuxiliaryEffectSlotiv = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTIV >(alGetProcAddress("alGetAuxiliaryEffectSlotiv"));
		alGetAuxiliaryEffectSlotf = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTF >(alGetProcAddress("alGetAuxiliaryEffectSlotf"));
		alGetAuxiliaryEffectSlotfv = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTFV >(alGetProcAddress("alGetAuxiliaryEffectSlotfv"));
	}
}
