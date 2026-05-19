/*
 * src/emeraude_platform.hpp
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
 */

#pragma once

/*
 * Pure-preprocessor platform detection. This header is hand-written and
 * is NEVER touched by CMake's configure_file step — including it in a .hpp
 * does not trigger a recompile when CMake reconfigures, unlike
 * emeraude_config.hpp which is regenerated from emeraude_config.hpp.in.
 *
 * Use this header when a translation unit only needs architecture / OS
 * detection (IS_LINUX, IS_X86_ARCH, OSName, IsLinux, ...). Pull in
 * emeraude_config.hpp only when version numbers, engine name, or
 * build-time debug flags are required.
 */

#if defined(__i386__) || defined(_M_IX86) // Major detection of x86 32bit architecture.
	#define IS_X86_ARCH 1
	#define IS_ARM_ARCH 0
	#define IS_32BIT_PLATFORM 1
	#define IS_64BIT_PLATFORM 0
	#define PLATFORM_PRECISION "32bit"
	#define PLATFORM_ARCH "x86_32"
	#define PLATFORM_ARCH_NICENAME "Intel x86"
#elif defined(__x86_64__) || defined(__amd64__) || defined(_M_AMD64) || defined(_WIN64) // Major detection of x86 64bit architecture.
	#define IS_X86_ARCH 1
	#define IS_ARM_ARCH 0
	#define IS_32BIT_PLATFORM 0
	#define IS_64BIT_PLATFORM 1
	#define PLATFORM_PRECISION "64bit"
	#define PLATFORM_ARCH "x86_64"
	#define PLATFORM_ARCH_NICENAME "AMD64"
#elif defined(__arm__) // Major detection of ARM 32bit architecture.
	#define IS_X86_ARCH 0
	#define IS_ARM_ARCH 1
	#define IS_32BIT_PLATFORM 1
	#define IS_64BIT_PLATFORM 0
	#define PLATFORM_PRECISION "32bit"
	#define PLATFORM_ARCH "arm"
	#define PLATFORM_ARCH_NICENAME "ARMv7" // Should be the ARMv7
#elif defined(__aarch64__) || defined(__arm64__) // Major detection of ARM 64bit architecture.
	#define IS_X86_ARCH 0
	#define IS_ARM_ARCH 1
	#define IS_32BIT_PLATFORM 0
	#define IS_64BIT_PLATFORM 1
	#define PLATFORM_PRECISION "64bit"
	#define PLATFORM_ARCH "arm64"
	#define PLATFORM_ARCH_NICENAME "AArch64"
#else
	#error "Unsupported architecture !"
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) // Major detection of Linux OS
	#define IS_LINUX 1
	#define IS_WINDOWS 0
	#define IS_MACOS 0
	#define OS_NAME "linux"
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64) // Major detection of Windows OS
	#define IS_LINUX 0
	#define IS_WINDOWS 1
	#define IS_MACOS 0
	#define OS_NAME "windows"
#elif defined(__APPLE__) // Major detection of Apple OS
	#define IS_LINUX 0
	#define IS_WINDOWS 0
	#define IS_MACOS 1
	#define OS_NAME "macos"
#else
	#error "Unsupported OS !"
#endif

#define PLATFORM_TARGETED OS_NAME "-" PLATFORM_PRECISION "-" PLATFORM_ARCH

/* NOTE: Code will use the GCC macro for printing function signature. */
#ifdef _MSC_VER
	#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace EmEn
{
	constexpr bool IsX86Arch{IS_X86_ARCH};
	constexpr bool IsARMArch{IS_ARM_ARCH};
	constexpr bool Is32BitPlatform{IS_32BIT_PLATFORM};
	constexpr bool Is64BitPlatform{IS_64BIT_PLATFORM};
	constexpr auto PlatformPrecision{PLATFORM_PRECISION};
	constexpr auto PlatformArch{PLATFORM_ARCH};
	constexpr auto PlatformArchNiceName{PLATFORM_ARCH_NICENAME};
	constexpr auto PlatformTargeted{PLATFORM_TARGETED};

	constexpr bool IsLinux{IS_LINUX};
	constexpr bool IsWindows{IS_WINDOWS};
	constexpr bool IsMacOS{IS_MACOS};
	constexpr auto OSName{OS_NAME};
}