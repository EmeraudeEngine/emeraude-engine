/*
 * src/Audio/Utility.hpp
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

/* STL inclusions. */
#include <string>

/* Third-party inclusions. */
#include "AL/alc.h"

namespace EmEn::Audio
{
	/**
	 * brief
	 * @param key
	 * @return std::string
	 */
	[[nodiscard]]
	std::string alcKeyToLabel (ALCint key) noexcept;

	/**
	 * brief
	 * @param lastFunctionCalled
	 * @param filename
	 * @param line
	 * @return bool
	 */
	[[nodiscard]]
	bool alGetErrors (const std::string & lastFunctionCalled = {"NO_AC_FUNCTION_REGISTERED"}, const std::string & filename = {"UNKNOWN"}, int line = -1) noexcept;

	/**
	 * @brief
	 * @return void
	 */
	void alFlushErrors () noexcept;

	/**
	 * brief
	 * @param device
	 * @param lastFunctionCalled
	 * @param filename
	 * @param line
	 * @return bool
	 */
	[[nodiscard]]
	bool alcGetErrors (ALCdevice * device, const std::string & lastFunctionCalled = {"NO_ALC_FUNCTION_REGISTERED"}, const std::string & filename = {"UNKNOWN"}, int line = -1) noexcept;

	/**
	 * brief
	 * @param device
	 * @return void
	 */
	void alcFlushErrors (ALCdevice * device) noexcept;
}
