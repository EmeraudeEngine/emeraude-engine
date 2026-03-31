/*
 * src/PlatformSpecific/DiskInfo.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>

namespace EmEn::PlatformSpecific::DiskInfo
{
	/**
	 * @brief Describes a mounted drive/volume on the system.
	 */
	struct DriveInfo
	{
		std::string filesystem;      /* Device path (e.g., "/dev/sda1") or description (Windows). */
		std::string mounted;         /* Mount point (e.g., "/", "/mnt/usb") or drive letter (e.g., "C:"). */
		std::string fsType;          /* Filesystem type (e.g., "ext4", "NTFS", "apfs", "exfat"). */
		uint64_t totalBytes{0};      /* Total size in bytes. */
		uint64_t usedBytes{0};       /* Used space in bytes. */
		uint64_t availableBytes{0};  /* Available space in bytes. */
		bool removable{false};       /* True if the drive is removable (USB, SD card). */
	};

	/**
	 * @brief Lists all mounted drives on the system.
	 * @note Platform-specific: uses /proc/mounts + statvfs (Linux),
	 * GetLogicalDriveStrings + GetDiskFreeSpaceEx (Windows),
	 * getmntinfo + DiskArbitration (macOS).
	 * @return std::vector< DriveInfo > List of mounted drives (excludes virtual/pseudo filesystems).
	 */
	[[nodiscard]]
	std::vector< DriveInfo > listDrives () noexcept;
}
