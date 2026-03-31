/*
 * src/PlatformSpecific/DiskInfo.windows.cpp
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

#include "DiskInfo.hpp"

/* Windows inclusions. */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

namespace EmEn::PlatformSpecific::DiskInfo
{
	std::vector< DriveInfo >
	listDrives () noexcept
	{
		std::vector< DriveInfo > drives;

		wchar_t driveStrings[512];
		const auto length = GetLogicalDriveStringsW(511, driveStrings);

		if ( length == 0 )
		{
			return drives;
		}

		for ( const wchar_t * drive = driveStrings; *drive != L'\0'; drive += wcslen(drive) + 1 )
		{
			/* Get drive type. */
			const auto driveType = GetDriveTypeW(drive);

			/* Skip network and unknown drives. */
			if ( driveType == DRIVE_UNKNOWN || driveType == DRIVE_NO_ROOT_DIR )
			{
				continue;
			}

			DriveInfo info;

			/* Convert drive letter to narrow string (e.g., "C:"). */
			char narrowDrive[8];
			WideCharToMultiByte(CP_UTF8, 0, drive, -1, narrowDrive, sizeof(narrowDrive), nullptr, nullptr);

			/* Remove trailing backslash for the mounted name ("C:\" → "C:"). */
			info.mounted = narrowDrive;

			if ( !info.mounted.empty() && info.mounted.back() == '\\' )
			{
				info.mounted.pop_back();
			}

			/* Get free/total space. */
			ULARGE_INTEGER freeBytesAvailable{};
			ULARGE_INTEGER totalBytes{};
			ULARGE_INTEGER freeBytes{};

			if ( GetDiskFreeSpaceExW(drive, &freeBytesAvailable, &totalBytes, &freeBytes) )
			{
				info.totalBytes = totalBytes.QuadPart;
				info.availableBytes = freeBytesAvailable.QuadPart;
				info.usedBytes = totalBytes.QuadPart - freeBytes.QuadPart;
			}

			/* Get volume info (name, filesystem type). */
			wchar_t volumeName[MAX_PATH]{};
			wchar_t fsName[MAX_PATH]{};

			if ( GetVolumeInformationW(drive, volumeName, MAX_PATH, nullptr, nullptr, nullptr, fsName, MAX_PATH) )
			{
				char narrowVolume[MAX_PATH];
				char narrowFS[MAX_PATH];
				WideCharToMultiByte(CP_UTF8, 0, volumeName, -1, narrowVolume, MAX_PATH, nullptr, nullptr);
				WideCharToMultiByte(CP_UTF8, 0, fsName, -1, narrowFS, MAX_PATH, nullptr, nullptr);

				info.filesystem = narrowVolume;
				info.fsType = narrowFS;
			}

			/* Removable detection. */
			info.removable = (driveType == DRIVE_REMOVABLE || driveType == DRIVE_CDROM);

			drives.emplace_back(std::move(info));
		}

		return drives;
	}
}
