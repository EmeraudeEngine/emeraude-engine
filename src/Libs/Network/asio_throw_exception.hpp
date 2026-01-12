/*
 * src/Libs/Network/asio_throw_exception.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <iostream>

/* ASIO configuration - Must be defined before including any ASIO headers. */
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE 1
#endif

#ifndef ASIO_NO_EXCEPTIONS
#define ASIO_NO_EXCEPTIONS 1
#endif

/* Disable coroutines as they require exceptions (co_composed.hpp uses throw). */
#ifndef ASIO_DISABLE_CO_AWAIT
#define ASIO_DISABLE_CO_AWAIT 1
#endif

/* Include ASIO config to get ASIO_SOURCE_LOCATION_DEFAULTED_PARAM macro. */
#include "asio/detail/config.hpp"

namespace asio::detail
{
	/**
	 * @brief Overrides the asio::detail::throw_exception() method to use the library without C++ exception mechanism.
	 * @tparam exception_t The exception type.
	 * @param exception A reference to the ASIO exception.
	 * @note This function is required when ASIO_NO_EXCEPTIONS is defined.
	 * The signature must match the declaration in asio/detail/throw_exception.hpp.
	 */
	template< typename exception_t >
	[[noreturn]]
	void
	throw_exception (const exception_t & exception ASIO_SOURCE_LOCATION_DEFAULTED_PARAM)
	{
		std::cerr << "[ASIO] Fatal error: " << exception.what() << '\n';

		std::abort();
	}
}
