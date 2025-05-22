/*
 * src/Libs/Statistics.hpp
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

/* Information

struct timespec timer = begin_timer();

// Do stuffs ...

long delay = terminate_timer(timer);

printf("Time elapsed : %ld ns\n", delay);

*/

#pragma once

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_LINUX

extern "C"
{
	/* C standard libraries */
	#include <time.h>

	/**
	 * @brief Calls this function to execute a nanosecond-resolution timer.
	 * @param start_time
	 * @return void
	 */
	void begin_timer (timespec * start_time) noexcept;

	/**
	 * @brief Calls this function to end a timer, returning nanoseconds elapsed as a long.
	 * @param start_time
	 * @return long
	 */
	long terminate_timer (timespec start_time) noexcept;
}

#endif
