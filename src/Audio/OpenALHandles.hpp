/*
 * src/Audio/OpenALHandles.hpp
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


#pragma once

/* STL inclusions. */
#include <memory>

/* Third-party inclusions. */
#include "AL/alc.h"

/*
 * The owning holders of the OpenAL native handles (Ave Robustus II P3, owner decision 2026-10-16: native handles in RAII
 * holders). A service still closes its handles itself in onTerminate(), in its order and with its checks, by taking
 * them back (release()); the holder closes whatever is left when the object dies — a service whose onInitialize() failed
 * half-way (it is never terminated), or one destroyed without its terminate().
 */
namespace EmEn::Audio
{
	/** @brief Closes an output (or loopback) device. */
	struct ALCDeviceCloser final
	{
		void
		operator() (ALCdevice * device) const noexcept
		{
			static_cast< void >(alcCloseDevice(device));
		}
	};

	/** @brief Closes a capture device. */
	struct ALCCaptureDeviceCloser final
	{
		void
		operator() (ALCdevice * device) const noexcept
		{
			static_cast< void >(alcCaptureCloseDevice(device));
		}
	};

	/** @brief Destroys a context, made non-current first when it is the current one (a current context cannot go). */
	struct ALCContextDestroyer final
	{
		void
		operator() (ALCcontext * context) const noexcept
		{
			if ( alcGetCurrentContext() == context )
			{
				static_cast< void >(alcMakeContextCurrent(nullptr));
			}

			alcDestroyContext(context);
		}
	};

	/** @brief An owned output (or loopback) device. Declare it BEFORE the contexts created on it: destroyed after them. */
	using ALCDeviceHandle = std::unique_ptr< ALCdevice, ALCDeviceCloser >;

	/** @brief An owned capture device. */
	using ALCCaptureDeviceHandle = std::unique_ptr< ALCdevice, ALCCaptureDeviceCloser >;

	/** @brief An owned context. Declare it AFTER its device: destroyed before it. */
	using ALCContextHandle = std::unique_ptr< ALCcontext, ALCContextDestroyer >;
}
