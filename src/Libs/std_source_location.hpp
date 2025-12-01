/*
 * src/Libs/std_source_location.hpp
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

#include "emeraude_config.hpp"

/* TODO: This file is deprecated now. */

#if IS_WINDOWS
	#if defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
		#include <source_location>
	#else
		#define DECLARE_FAKE_SOURCE_LOCATION
	#endif
#else
	#if __cplusplus >= 202002L /* C++20 feature */
		#include <source_location>

		#if !__has_builtin(__builtin_source_location)
			#define DECLARE_FAKE_SOURCE_LOCATION
		#endif
	#else
		#define DECLARE_FAKE_SOURCE_LOCATION
	#endif
#endif

#ifdef DECLARE_FAKE_SOURCE_LOCATION
namespace std
{
	class source_location
	{
		public:

			source_location () = default;

			const char *
			file_name () const noexcept
			{
				return "std::source_location unavailable !";
			}

			int
			line () const noexcept
			{
				return -1;
			}

			int
			column () const noexcept
			{
				return -1;
			}

			const char *
			function_name () const noexcept
			{
				return "std::source_location unavailable !";
			}

			static
			source_location
			current () noexcept
			{
				return {};
			}
	};
}
#endif
