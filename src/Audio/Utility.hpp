/*
 * src/Audio/Utility.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <string>

/* Third-party inclusions. */
#include "AL/alc.h"

namespace EmEn::Audio
{
	/**
	 * @brief Returns a readable label for an ALC attribute key.
	 * @param key The ALC attribute key.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string alcKeyToLabel (ALCint key) noexcept;

	/**
	 * @brief Checks and reports the pending OpenAL errors.
	 * @param lastFunctionCalled The name of the last OpenAL function called.
	 * @param filename The source file of the call.
	 * @param line The source line of the call.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool alGetErrors (const std::string & lastFunctionCalled = {"NO_AC_FUNCTION_REGISTERED"}, const std::string & filename = {"UNKNOWN"}, int line = -1) noexcept;

	/**
	 * @brief Discards the pending OpenAL errors.
	 */
	EMEN_API void alFlushErrors () noexcept;

	/**
	 * @brief Checks and reports the pending ALC errors of a device.
	 * @param device A pointer to the ALC device.
	 * @param lastFunctionCalled The name of the last ALC function called.
	 * @param filename The source file of the call.
	 * @param line The source line of the call.
	 * @return bool
	 */
	[[nodiscard]]
	EMEN_API bool alcGetErrors (ALCdevice * device, const std::string & lastFunctionCalled = {"NO_ALC_FUNCTION_REGISTERED"}, const std::string & filename = {"UNKNOWN"}, int line = -1) noexcept;

	/**
	 * @brief Discards the pending ALC errors of a device.
	 * @param device A pointer to the ALC device.
	 */
	EMEN_API void alcFlushErrors (ALCdevice * device) noexcept;
}
