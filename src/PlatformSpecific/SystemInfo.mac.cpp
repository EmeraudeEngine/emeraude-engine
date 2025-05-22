/*
 * src/PlatformSpecific/SystemInfo.mac.cpp
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

#include "SystemInfo.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_MACOS

/* STL inclusions. */
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio> // FIXME: Remove this

/* Third-party inclusions. */
#include <unistd.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#include <IOKit/IOKitLib.h>

namespace EmEn::PlatformSpecific
{
	bool
	SystemInfo::fetchOSInformation () noexcept
	{
		char OSVersion[32];
		size_t OSVersionLength = sizeof(OSVersion) - 1;
		int OSVersionName[] = {CTL_KERN, KERN_OSRELEASE};

		if ( sysctl(OSVersionName, 2, OSVersion, &OSVersionLength, nullptr, 0) == -1 )
		{
			return false;
		}

		// FIXME: Remove the unsafe sscanf() C method;
		uint32_t major, minor;

		if ( sscanf(OSVersion, "%u.%u", &major, &minor) != 2 )
		{
			return false;
		}

		if ( major >= 20 )
		{
			// macOS 11 and newer
			m_OSInformation.systemName = (std::stringstream{} << "Apple MacOS " << (major - 9) << '.' << minor).str();
			m_OSInformation.systemVersion = (std::stringstream{} << major << '.' << minor).str();
		}
		else
		{
			// macOS 10.1.1 and newer
			m_OSInformation.systemName = (std::stringstream{} << "Apple Mac OS X " << (major - 4) << '.' << minor).str();
			m_OSInformation.systemVersion = (std::stringstream{} << "10." << major << '.' << minor).str();
		}

		{
			io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMainPortDefault, "IOService:/");

			auto UUIDString = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);

			const char * buffer = CFStringGetCStringPtr(UUIDString, kCFStringEncodingMacRoman);

			if ( buffer != nullptr )
			{
				m_OSInformation.machineUUID.assign(buffer);
			}
			else
			{
				std::cerr << "Unable to get the machine UUID !" "\n";
			}

			CFRelease(UUIDString);

			IOObjectRelease(ioRegistryRoot);
		}

		return true;
	}

	size_t
	SystemInfo::getTotalMemory () noexcept
	{
		return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
	}

	size_t
	SystemInfo::getFreeMemory () noexcept
	{
		/* FIXME: _SC_AVPHYS_PAGES do not exists on MacOS. */
		return 0;//sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
	}

	size_t
	SystemInfo::getPeakRSS () noexcept
	{
		rusage rusage{};
		getrusage(RUSAGE_SELF, &rusage);

		return static_cast< size_t >(rusage.ru_maxrss);
	}

	size_t
	SystemInfo::getCurrentRSS () noexcept
	{
		mach_task_basic_info info{};
		mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

		if ( task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS )
		{
			/* Can't access? */
			return 0;
		}

		return static_cast< size_t >(info.resident_size);
	}

	std::filesystem::path
	SystemInfo::getRealApplicationDir () noexcept
	{
		char exePath[PATH_MAX];
		uint32_t len = sizeof(exePath);

		if ( _NSGetExecutablePath(exePath, &len) != 0 )
		{
			exePath[0] = '\0'; // buffer too small (!)
		}
		else
		{
			// resolve symlinks, ... if possible
			char * canonicalPath = realpath(exePath, nullptr);

			if ( canonicalPath != nullptr )
			{
				strncpy(exePath,canonicalPath,len);
				free(canonicalPath);
			}
		}

		std::string realPath{exePath};

		const auto position = realPath.find_last_of('/');

		return
			position == std::string::npos ?
			realPath :
			realPath.substr(0, position);
	}
}

#endif
