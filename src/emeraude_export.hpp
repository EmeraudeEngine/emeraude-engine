/*
 * src/emeraude_export.hpp
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

/*
 * Public boundary annotations for Emeraude.dll — see docs/windows-export-api.md.
 *
 *  | CMake option              | EMEN_LEAN_API | EMEN_API |                                    |
 *  |---------------------------|---------------|----------|------------------------------------|
 *  | EMERAUDE_USE_FULL_EXPORTS | exported      | exported | whole annotated API                |
 *  | EMERAUDE_USE_LEAN_EXPORTS | exported      | inert    | consumed scopes only               |
 *  | neither                   | inert         | inert    | WINDOWS_EXPORT_ALL_SYMBOLS         |
 *
 * macOS: both macros ALWAYS expand to default visibility, whatever the option. A consumer built with
 * -fvisibility=hidden (CEF's flags, projet-alpha) otherwise holds HIDDEN copies of the typeinfo of every annotated
 * class it uses that has no key function in the engine (inline-only virtuals, non-polymorphic types), while the
 * engine's own copies are visible: libc++ (arm64) then compares the two by address, and typeid(), std::any_cast and
 * (without the system libc++abi's fallback) dynamic_cast fail across the engine/application boundary. Explicit
 * default visibility makes both images agree. See docs/caution-points.md, "macOS: typeid, std::any and
 * dynamic_cast across the engine boundary".
 */

// Shared expansion. Never annotate with it directly.
#if defined(_WIN32) || defined(__CYGWIN__)
	#ifdef Emeraude_EXPORTS
		#define EMEN_API_IMPL __declspec(dllexport)
	#else
		#define EMEN_API_IMPL __declspec(dllimport)
	#endif
#else
	#define EMEN_API_IMPL __attribute__((visibility("default")))
#endif

#if defined(EMERAUDE_USE_FULL_EXPORTS) || defined(EMERAUDE_USE_LEAN_EXPORTS) || defined(__APPLE__)
	#define EMEN_LEAN_API EMEN_API_IMPL
#else
	#define EMEN_LEAN_API
#endif

#if defined(EMERAUDE_USE_FULL_EXPORTS) || defined(__APPLE__)
	#define EMEN_API EMEN_API_IMPL
#else
	#define EMEN_API
#endif
