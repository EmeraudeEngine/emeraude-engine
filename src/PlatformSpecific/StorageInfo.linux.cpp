/*
 * src/PlatformSpecific/StorageInfo.linux.cpp
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

#include "StorageInfo.hpp"

/* STL inclusions. */
#include <fstream>
#include <sstream>
#include <filesystem>

/* POSIX inclusions. */
#include <sys/statvfs.h>

namespace EmEn::PlatformSpecific::StorageInfo
{
	/**
	 * @brief Checks if a block device is removable via sysfs.
	 * @param devicePath The device path (e.g., "/dev/sdb1").
	 * @return bool True if removable.
	 */
	static bool
	isDeviceRemovable (const std::string & devicePath) noexcept
	{
		/* Extract the base block device name: "/dev/sdb1" → "sdb", "/dev/nvme0n1p2" → "nvme0n1". */
		auto devName = std::filesystem::path(devicePath).filename().string();

		/* Strip partition suffix: "sdb1" → "sdb", "nvme0n1p2" → "nvme0n1". */
		while ( !devName.empty() && std::isdigit(devName.back()) )
		{
			devName.pop_back();
		}

		/* For nvme, also strip the trailing 'p' (partition separator). */
		if ( !devName.empty() && devName.back() == 'p' && devName.find("nvme") != std::string::npos )
		{
			devName.pop_back();
		}

		const auto removablePath = std::filesystem::path("/sys/block") / devName / "removable";

		std::ifstream file(removablePath);

		if ( !file.is_open() )
		{
			return false;
		}

		int value = 0;
		file >> value;

		return value == 1;
	}

	std::vector< DriveInfo >
	listDrives () noexcept
	{
		std::vector< DriveInfo > drives;

		std::ifstream mountsFile("/proc/mounts");

		if ( !mountsFile.is_open() )
		{
			return drives;
		}

		std::string line;

		while ( std::getline(mountsFile, line) )
		{
			std::istringstream lineStream(line);
			std::string device;
			std::string mountPoint;
			std::string fsType;

			lineStream >> device >> mountPoint >> fsType;

			/* Skip virtual/pseudo filesystems (only keep real block devices). */
			if ( device.find("/dev/") != 0 )
			{
				continue;
			}

			/* Skip device mapper entries for snap/loop (common on Ubuntu). */
			if ( device.find("/dev/loop") == 0 )
			{
				continue;
			}

			struct statvfs stat{};

			if ( statvfs(mountPoint.c_str(), &stat) != 0 )
			{
				continue;
			}

			DriveInfo info;
			info.filesystem = device;
			info.mounted = mountPoint;
			info.fsType = fsType;
			info.totalBytes = static_cast< uint64_t >(stat.f_blocks) * stat.f_frsize;
			info.availableBytes = static_cast< uint64_t >(stat.f_bavail) * stat.f_frsize;
			info.usedBytes = info.totalBytes - (static_cast< uint64_t >(stat.f_bfree) * stat.f_frsize);
			info.removable = isDeviceRemovable(device);

			drives.emplace_back(std::move(info));
		}

		return drives;
	}
}
