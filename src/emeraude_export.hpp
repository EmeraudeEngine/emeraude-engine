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
 * EMERAUDE_API — public boundary annotation for the engine shared library (Emeraude.dll).
 * See docs/windows-export-api.md for the full migration procedure and rationale.
 *
 * Two modes, selected by the EMERAUDE_USE_EXPLICIT_EXPORTS build option (CMake):
 *
 *  - OFF (default): the macro expands to NOTHING. The DLL's exported surface is produced by
 *    CMake's WINDOWS_EXPORT_ALL_SYMBOLS, exactly as before. Annotations are harmless no-ops,
 *    so the public API can be migrated class-by-class without ever breaking the build.
 *
 *  - ON: the macro becomes __declspec(dllexport) while building the DLL (CMake auto-defines
 *    Emeraude_EXPORTS for the SHARED target) and __declspec(dllimport) for consumers
 *    (projet-alpha); WINDOWS_EXPORT_ALL_SYMBOLS is dropped. This is the end state required on
 *    MSVC, where WINDOWS_EXPORT_ALL_SYMBOLS + precompiled headers leak PCH marker symbols into
 *    the auto-generated exports.def and fail to link (LNK2001 on a bogus '__' symbol).
 *
 * Annotate public CLASSES (exports every member) and out-of-line free functions with
 * EMERAUDE_API. A dll-interface class whose base is not itself EMERAUDE_API triggers C4275 on
 * MSVC — its bases must be annotated too (see the doc).
 */
#if defined(EMERAUDE_USE_EXPLICIT_EXPORTS)
	#if defined(_WIN32) || defined(__CYGWIN__)
		#ifdef Emeraude_EXPORTS
			#define EMERAUDE_API __declspec(dllexport)
		#else
			#define EMERAUDE_API __declspec(dllimport)
		#endif
	#else
		#define EMERAUDE_API __attribute__((visibility("default")))
	#endif
#else
	#define EMERAUDE_API
#endif
