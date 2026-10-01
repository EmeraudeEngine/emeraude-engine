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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "StorageInfo.hpp"

/* STL inclusions. */
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

/* Third-party inclusions. */
/* POSIX inclusions. */
#include <sys/statvfs.h>

namespace EmEn::PlatformSpecific::StorageInfo
{
	namespace
	{
		/**
		 * @brief Checks if a block device is removable via sysfs.
		 * @param devicePath The device path (e.g., "/dev/sdb1").
		 * @return bool True if removable.
		 */
		bool
		isDeviceRemovable (const std::string & devicePath) noexcept
		{
			/* Extract the base block device name: "/dev/sdb1" → "sdb", "/dev/nvme0n1p2" → "nvme0n1". */
			auto devName = std::filesystem::path(devicePath).filename().string();

			/* Strip partition suffix: "sdb1" → "sdb", "nvme0n1p2" → "nvme0n1". */
			while ( !devName.empty() && std::isdigit(static_cast< unsigned char >(devName.back())) != 0 )
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

		/**
		 * @brief Decodes the octal escapes of a /proc/mounts field (a space is "\040", a tab "\011", a newline "\012", a
		 * backslash "\134"): a mount point with a space was passed escaped to statvfs() and skipped.
		 * @param field The escaped field.
		 * @return std::string
		 */
		std::string
		decodeMountField (const std::string & field) noexcept
		{
			std::string decoded;
			decoded.reserve(field.size());

			for ( size_t index = 0; index < field.size(); ++index )
			{
				const auto isOctal = [&field] (size_t position) {
					return position < field.size() && field[position] >= '0' && field[position] <= '7';
				};

				if ( field[index] == '\\' && isOctal(index + 1) && isOctal(index + 2) && isOctal(index + 3) )
				{
					const auto value = ((field[index + 1] - '0') * 64) + ((field[index + 2] - '0') * 8) + (field[index + 3] - '0');

					decoded += static_cast< char >(value);
					index += 3;
				}
				else
				{
					decoded += field[index];
				}
			}

			return decoded;
		}
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

			mountPoint = decodeMountField(mountPoint);

			/* Skip virtual/pseudo filesystems (only keep real block devices). */
			if ( !device.starts_with("/dev/") )
			{
				continue;
			}

			/* Skip device mapper entries for snap/loop (common on Ubuntu). */
			if ( device.starts_with("/dev/loop") )
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
