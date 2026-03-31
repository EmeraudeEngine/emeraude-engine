/*
 * src/PlatformSpecific/DiskInfo.mac.mm
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

/* macOS inclusions. */
#include <sys/mount.h>

#import <Foundation/Foundation.h>
#import <DiskArbitration/DiskArbitration.h>

namespace EmEn::PlatformSpecific::DiskInfo
{
	/**
	 * @brief Checks if a device is removable using DiskArbitration.
	 * @param bsdName The BSD device name (e.g., "disk2s1").
	 * @return bool True if removable.
	 */
	static bool
	isDeviceRemovable (const char * bsdName) noexcept
	{
		@autoreleasepool
		{
			DASessionRef session = DASessionCreate(kCFAllocatorDefault);

			if ( session == nullptr )
			{
				return false;
			}

			DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, bsdName);

			if ( disk == nullptr )
			{
				CFRelease(session);

				return false;
			}

			CFDictionaryRef desc = DADiskCopyDescription(disk);

			if ( desc == nullptr )
			{
				CFRelease(disk);
				CFRelease(session);

				return false;
			}

			bool removable = false;

			auto removableRef = static_cast< CFBooleanRef >(
				CFDictionaryGetValue(desc, kDADiskDescriptionMediaRemovableKey));

			if ( removableRef != nullptr )
			{
				removable = CFBooleanGetValue(removableRef);
			}

			/* Also check ejectable as a fallback for external USB drives. */
			if ( !removable )
			{
				auto ejectableRef = static_cast< CFBooleanRef >(
					CFDictionaryGetValue(desc, kDADiskDescriptionMediaEjectableKey));

				if ( ejectableRef != nullptr )
				{
					removable = CFBooleanGetValue(ejectableRef);
				}
			}

			CFRelease(desc);
			CFRelease(disk);
			CFRelease(session);

			return removable;
		}
	}

	std::vector< DriveInfo >
	listDrives () noexcept
	{
		std::vector< DriveInfo > drives;

		struct statfs * mounts = nullptr;
		const auto count = getmntinfo(&mounts, MNT_NOWAIT);

		if ( count <= 0 || mounts == nullptr )
		{
			return drives;
		}

		for ( int i = 0; i < count; i++ )
		{
			const auto & fs = mounts[i];

			/* Skip virtual filesystems (only keep real devices starting with /dev/). */
			if ( std::string(fs.f_mntfromname).find("/dev/") != 0 )
			{
				continue;
			}

			DriveInfo info;
			info.filesystem = fs.f_mntfromname;
			info.mounted = fs.f_mntonname;
			info.fsType = fs.f_fstypename;
			info.totalBytes = static_cast< uint64_t >(fs.f_blocks) * fs.f_bsize;
			info.availableBytes = static_cast< uint64_t >(fs.f_bavail) * fs.f_bsize;
			info.usedBytes = info.totalBytes - (static_cast< uint64_t >(fs.f_bfree) * fs.f_bsize);

			/* Extract BSD name from device path: "/dev/disk2s1" → "disk2s1". */
			const std::string devPrefix{"/dev/"};

			if ( info.filesystem.find(devPrefix) == 0 )
			{
				info.removable = isDeviceRemovable(info.filesystem.c_str() + devPrefix.size());
			}

			drives.emplace_back(std::move(info));
		}

		return drives;
	}
}
