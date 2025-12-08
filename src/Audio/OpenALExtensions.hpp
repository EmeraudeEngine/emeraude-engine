/*
 * src/Audio/OpenAL.EFX.hpp
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

#pragma once

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include "AL/efx.h"
#include "AL/efx-presets.h"

/* Local inclusions. */
#include "Tracer.hpp"

/* NOTE: These types may not be defined in older OpenAL-Soft versions (< 1.24).
 * Define them manually for compatibility with OpenAL-Soft 1.23.x and earlier. */
#ifndef ALC_SOFT_system_events
#define ALC_SOFT_system_events 1
#define ALC_EVENT_SUPPORTED_SOFT 0x19A5
#define ALC_EVENT_NOT_SUPPORTED_SOFT 0x0000
#define ALC_EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT 0x19A6
#define ALC_EVENT_TYPE_DEVICE_ADDED_SOFT 0x19A7
#define ALC_EVENT_TYPE_DEVICE_REMOVED_SOFT 0x19A8
typedef ALCboolean (ALC_APIENTRY *LPALCEVENTISSUPPORTEDSOFT)(ALCenum eventType);
typedef ALCboolean (ALC_APIENTRY *LPALCEVENTCONTROLSOFT)(ALCsizei count, const ALCenum *types, ALCboolean enable);
typedef void (ALC_APIENTRY *LPALCEVENTCALLBACKSOFT)(void (ALC_APIENTRY *callback)(ALCenum eventType, ALCenum deviceType, ALCdevice *device, ALCsizei length, const ALCchar *message, void *userParam), void *userParam);
#endif

namespace EmEn::Audio::OpenAL
{
	constexpr auto TracerTag{"OpenAL.Extension"};

	/* NOTE: OpenAL 'ALC_EXT_EFX' extension. */
	inline LPALGENEFFECTS alGenEffects{nullptr};
	inline LPALDELETEEFFECTS alDeleteEffects{nullptr};
	inline LPALISEFFECT alIsEffect{nullptr};
	inline LPALEFFECTI alEffecti{nullptr};
	inline LPALEFFECTIV alEffectiv{nullptr};
	inline LPALEFFECTF alEffectf{nullptr};
	inline LPALEFFECTFV alEffectfv{nullptr};
	inline LPALGETEFFECTI alGetEffecti{nullptr};
	inline LPALGETEFFECTIV alGetEffectiv{nullptr};
	inline LPALGETEFFECTF alGetEffectf{nullptr};
	inline LPALGETEFFECTFV alGetEffectfv{nullptr};
	inline LPALGENFILTERS alGenFilters{nullptr};
	inline LPALDELETEFILTERS alDeleteFilters{nullptr};
	inline LPALISFILTER alIsFilter{nullptr};
	inline LPALFILTERI alFilteri{nullptr};
	inline LPALFILTERIV alFilteriv{nullptr};
	inline LPALFILTERF alFilterf{nullptr};
	inline LPALFILTERFV alFilterfv{nullptr};
	inline LPALGETFILTERI alGetFilteri{nullptr};
	inline LPALGETFILTERIV alGetFilteriv{nullptr};
	inline LPALGETFILTERF alGetFilterf{nullptr};
	inline LPALGETFILTERFV alGetFilterfv{nullptr};
	inline LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots{nullptr};
	inline LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots{nullptr};
	inline LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot{nullptr};
	inline LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti{nullptr};
	inline LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv{nullptr};
	inline LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf{nullptr};
	inline LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv{nullptr};
	inline LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti{nullptr};
	inline LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv{nullptr};
	inline LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf{nullptr};
	inline LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv{nullptr};

	inline ALint g_maxAuxiliarySends{0};
	inline bool g_isEFXAvailable{false};

	/**
	 * @brief Installs the OpenAL (ALC) extension 'ALC_EXT_EFX'.
	 * @param device A pointer to an output audio device.
	 * @return bool
	 */
	inline
	bool
	installExtensionEFX (ALCdevice * device) noexcept
	{
		if ( !alcIsExtensionPresent(device, "ALC_EXT_EFX") )
		{
			Tracer::warning(TracerTag, "The device doesn't support the 'ALC_EXT_EFX' extension !");

			return false;
		}

		g_isEFXAvailable = true;

		/* Checks max auxiliary sends. */
		alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &g_maxAuxiliarySends);

		alGenEffects = reinterpret_cast< LPALGENEFFECTS >(alcGetProcAddress(device, "alGenEffects"));
		alDeleteEffects = reinterpret_cast< LPALDELETEEFFECTS >(alcGetProcAddress(device, "alDeleteEffects"));
		alIsEffect = reinterpret_cast< LPALISEFFECT >(alcGetProcAddress(device, "alIsEffect"));
		alEffecti = reinterpret_cast< LPALEFFECTI >(alcGetProcAddress(device, "alEffecti"));
		alEffectiv = reinterpret_cast< LPALEFFECTIV >(alcGetProcAddress(device, "alEffectiv"));
		alEffectf = reinterpret_cast< LPALEFFECTF >(alcGetProcAddress(device, "alEffectf"));
		alEffectfv = reinterpret_cast< LPALEFFECTFV >(alcGetProcAddress(device, "alEffectfv"));
		alGetEffecti = reinterpret_cast< LPALGETEFFECTI >(alcGetProcAddress(device, "alGetEffecti"));
		alGetEffectiv = reinterpret_cast< LPALGETEFFECTIV >(alcGetProcAddress(device, "alGetEffectiv"));
		alGetEffectf = reinterpret_cast< LPALGETEFFECTF >(alcGetProcAddress(device, "alGetEffectf"));
		alGetEffectfv = reinterpret_cast< LPALGETEFFECTFV >(alcGetProcAddress(device, "alGetEffectfv"));

		alGenFilters = reinterpret_cast< LPALGENFILTERS >(alcGetProcAddress(device, "alGenFilters"));
		alDeleteFilters = reinterpret_cast< LPALDELETEFILTERS >(alcGetProcAddress(device, "alDeleteFilters"));
		alIsFilter = reinterpret_cast< LPALISFILTER >(alcGetProcAddress(device, "alIsFilter"));
		alFilteri = reinterpret_cast< LPALFILTERI >(alcGetProcAddress(device, "alFilteri"));
		alFilteriv = reinterpret_cast< LPALFILTERIV >(alcGetProcAddress(device, "alFilteriv"));
		alFilterf = reinterpret_cast< LPALFILTERF >(alcGetProcAddress(device, "alFilterf"));
		alFilterfv = reinterpret_cast< LPALFILTERFV >(alcGetProcAddress(device, "alFilterfv"));
		alGetFilteri = reinterpret_cast< LPALGETFILTERI >(alcGetProcAddress(device, "alGetFilteri"));
		alGetFilteriv = reinterpret_cast< LPALGETFILTERIV >(alcGetProcAddress(device, "alGetFilteriv"));
		alGetFilterf = reinterpret_cast< LPALGETFILTERF >(alcGetProcAddress(device, "alGetFilterf"));
		alGetFilterfv = reinterpret_cast< LPALGETFILTERFV >(alcGetProcAddress(device, "alGetFilterfv"));

		alGenAuxiliaryEffectSlots = reinterpret_cast< LPALGENAUXILIARYEFFECTSLOTS >(alcGetProcAddress(device, "alGenAuxiliaryEffectSlots"));
		alDeleteAuxiliaryEffectSlots = reinterpret_cast< LPALDELETEAUXILIARYEFFECTSLOTS >(alcGetProcAddress(device, "alDeleteAuxiliaryEffectSlots"));
		alIsAuxiliaryEffectSlot = reinterpret_cast< LPALISAUXILIARYEFFECTSLOT >(alcGetProcAddress(device, "alIsAuxiliaryEffectSlot"));
		alAuxiliaryEffectSloti = reinterpret_cast< LPALAUXILIARYEFFECTSLOTI >(alcGetProcAddress(device, "alAuxiliaryEffectSloti"));
		alAuxiliaryEffectSlotiv = reinterpret_cast< LPALAUXILIARYEFFECTSLOTIV >(alcGetProcAddress(device, "alAuxiliaryEffectSlotiv"));
		alAuxiliaryEffectSlotf = reinterpret_cast< LPALAUXILIARYEFFECTSLOTF >(alcGetProcAddress(device, "alAuxiliaryEffectSlotf"));
		alAuxiliaryEffectSlotfv = reinterpret_cast< LPALAUXILIARYEFFECTSLOTFV >(alcGetProcAddress(device, "alAuxiliaryEffectSlotfv"));
		alGetAuxiliaryEffectSloti = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTI >(alcGetProcAddress(device, "alGetAuxiliaryEffectSloti"));
		alGetAuxiliaryEffectSlotiv = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTIV >(alcGetProcAddress(device, "alGetAuxiliaryEffectSlotiv"));
		alGetAuxiliaryEffectSlotf = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTF >(alcGetProcAddress(device, "alGetAuxiliaryEffectSlotf"));
		alGetAuxiliaryEffectSlotfv = reinterpret_cast< LPALGETAUXILIARYEFFECTSLOTFV >(alcGetProcAddress(device, "alGetAuxiliaryEffectSlotfv"));

		Tracer::success(TracerTag, "The device support 'ALC_EXT_EFX' extension.");

		return true;
	}

	/**
	 * @brief Returns the OpenAL (ALC) extension 'ALC_EXT_EFX'.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	isEFXAvailable () noexcept
	{
		return g_isEFXAvailable;
	}

	/* NOTE: OpenAL 'ALC_SOFT_system_events' extension. */
	inline LPALCEVENTISSUPPORTEDSOFT alcEventIsSupportedSOFT{nullptr};
	inline LPALCEVENTCONTROLSOFT alcEventControlSOFT{nullptr};
	inline LPALCEVENTCALLBACKSOFT alcEventCallbackSOFT{nullptr};

	inline bool g_isSystemEventsAvailable{false};

	/**
	 * @brief Installs the OpenAL (ALC) extension 'ALC_SOFT_system_events'.
	 * @param device A pointer to an output audio device.
	 * @return bool
	 */
	inline
	bool
	installExtensionSystemEvents (ALCdevice * device) noexcept
	{
		if ( !alcIsExtensionPresent(device, "ALC_SOFT_system_events") )
		{
			Tracer::warning(TracerTag, "The device doesn't support the 'ALC_SOFT_system_events' extension !");

			return false;
		}

		g_isSystemEventsAvailable = true;

		alcEventIsSupportedSOFT = reinterpret_cast< LPALCEVENTISSUPPORTEDSOFT >(alcGetProcAddress(device, "alcEventIsSupportedSOFT"));
		alcEventControlSOFT = reinterpret_cast< LPALCEVENTCONTROLSOFT >(alcGetProcAddress(device, "alcEventControlSOFT"));
		alcEventCallbackSOFT = reinterpret_cast< LPALCEVENTCALLBACKSOFT >(alcGetProcAddress(device, "alcEventCallbackSOFT"));

		Tracer::success(TracerTag, "The device support 'ALC_SOFT_system_events' extension.");

		return true;
	}

	/**
	 * @brief Returns the OpenAL (ALC) extension 'ALC_SOFT_system_events'.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	isSystemEventAvailable () noexcept
	{
		return g_isSystemEventsAvailable;
	}

	/* NOTE: OpenAL 'AL_SOFT_events' extension. */
	inline LPALEVENTCONTROLSOFT alEventControlSOFT{nullptr};
	inline LPALEVENTCALLBACKSOFT alEventCallbackSOFT{nullptr};
	inline LPALGETPOINTERSOFT alGetPointerSOFT{nullptr};
	inline LPALGETPOINTERVSOFT alGetPointervSOFT{nullptr};

	inline bool g_isEventsAvailable{false};

	/**
	 * @brief Installs the OpenAL (AL) extension 'AL_SOFT_events'.
	 * @return bool
	 */
	inline
	bool
	installExtensionEvents () noexcept
	{
		if ( !alIsExtensionPresent("AL_SOFT_events") )
		{
			Tracer::warning(TracerTag, "The device doesn't support the 'AL_SOFT_events' extension !");

			return false;
		}

		g_isEventsAvailable = true;

		alEventControlSOFT = reinterpret_cast< LPALEVENTCONTROLSOFT >(alGetProcAddress("alEventControlSOFT"));
		alEventCallbackSOFT = reinterpret_cast< LPALEVENTCALLBACKSOFT >(alGetProcAddress("alEventCallbackSOFT"));
		alGetPointerSOFT = reinterpret_cast< LPALGETPOINTERSOFT >(alGetProcAddress("alGetPointerSOFT"));
		alGetPointervSOFT = reinterpret_cast< LPALGETPOINTERVSOFT >(alGetProcAddress("alGetPointervSOFT"));

		Tracer::success(TracerTag, "The device support 'AL_SOFT_events' extension.");

		return true;
	}

	/**
	 * @brief Returns the OpenAL (AL) extension 'AL_SOFT_events'.
	 * @return bool
	 */
	[[nodiscard]]
	inline
	bool
	isEventsAvailable () noexcept
	{
		return g_isEventsAvailable;
	}
}
