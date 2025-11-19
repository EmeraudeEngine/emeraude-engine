/*
 * src/Libs/FileTimestamps.cpp
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

#include "FileTimestamps.hpp"

/* Project configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <chrono>
#include <system_error> /* For std::error_code */

/* Platform-specific includes. */
#if IS_LINUX
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
	/* NOTE: statx is available since Linux kernel 4.11 (2017). */
	#ifndef STATX_BTIME
		#define STATX_BTIME 0x0800U
	#endif
#elif IS_MACOS
	#include <sys/stat.h>
	#include <sys/attr.h>
	#include <unistd.h>
#elif IS_WINDOWS
	#include <windows.h>
#endif

namespace EmEn::Libs
{
	/**
	 * @brief Converts a timespec to nanoseconds since Unix epoch.
	 * @param ts The timespec structure.
	 * @return Nanoseconds since Unix epoch.
	 */
	static
	inline
	uint64_t
	timespecToNanoseconds (const timespec & ts) noexcept
	{
		return static_cast< uint64_t >(ts.tv_sec) * 1000000000ULL + static_cast< uint64_t >(ts.tv_nsec);
	}

#if IS_WINDOWS
	/**
	 * @brief Converts a Windows FILETIME to nanoseconds since Unix epoch.
	 * @param ft The FILETIME structure.
	 * @return Nanoseconds since Unix epoch.
	 */
	static
	inline
	uint64_t
	filetimeToNanoseconds (const FILETIME & ft) noexcept
	{
		/* FILETIME is in 100-nanosecond intervals since 1601-01-01.
		 * Unix epoch (1970-01-01) is 116444736000000000 * 100ns later. */
		constexpr uint64_t epochDifference = 116444736000000000ULL;
		const uint64_t fileTime = (static_cast< uint64_t >(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

		if ( fileTime < epochDifference )
		{
			return 0;
		}

		/* Convert to nanoseconds (multiply by 100). */
		return (fileTime - epochDifference) * 100ULL;
	}
#endif

	void
	FileTimestamps::fetchInfo () noexcept
	{
		m_fetched = true;

		if ( !std::filesystem::exists(m_file) )
		{
			return;
		}

		/* First, try to get mtime using STL (most portable). */
		std::error_code ec;
		const auto ftime = std::filesystem::last_write_time(m_file, ec);

		if ( !ec )
		{
			const auto sctp = std::chrono::time_point_cast< std::chrono::system_clock::duration >(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
			const auto nanoseconds = std::chrono::duration_cast< std::chrono::nanoseconds >(sctp.time_since_epoch()).count();

			m_mtimeNS = static_cast< uint64_t >(nanoseconds);
		}
		else
		{
			m_mtimeNS = 0;
		}

		/* Now fetch platform-specific timestamps. */
#if IS_LINUX
		/* Try statx first for birthtime support (kernel 4.11+). */
		struct statx stx{};
		const int result = statx(AT_FDCWD, m_file.c_str(), AT_SYMLINK_NOFOLLOW, STATX_ATIME | STATX_MTIME | STATX_CTIME | STATX_BTIME, &stx);

		if ( result == 0 )
		{
			/* Check which fields are valid. */
			if ( stx.stx_mask & STATX_ATIME )
			{
				m_atimeNS = timespecToNanoseconds({stx.stx_atime.tv_sec, stx.stx_atime.tv_nsec});
			}

			if ( stx.stx_mask & STATX_MTIME )
			{
				m_mtimeNS = timespecToNanoseconds({stx.stx_mtime.tv_sec, stx.stx_mtime.tv_nsec});
			}

			if ( stx.stx_mask & STATX_CTIME )
			{
				m_ctimeNS = timespecToNanoseconds({stx.stx_ctime.tv_sec, stx.stx_ctime.tv_nsec});
			}

			if ( stx.stx_mask & STATX_BTIME )
			{
				m_birthTimeNS = timespecToNanoseconds({stx.stx_btime.tv_sec, stx.stx_btime.tv_nsec});
			}
		}
		else
		{
			/* Fall back to regular stat (no birthtime). */
			struct stat st{};

			if ( stat(m_file.c_str(), &st) == 0 )
			{
				m_atimeNS = timespecToNanoseconds(st.st_atim);
				m_mtimeNS = timespecToNanoseconds(st.st_mtim);
				m_ctimeNS = timespecToNanoseconds(st.st_ctim);
				/* birthTimeNS remains 0 (unavailable). */
			}
		}

#elif IS_MACOS
		/* macOS has birthtime in stat structure. */
		struct stat st{};

		if ( stat(m_file.c_str(), &st) == 0 )
		{
			m_atimeNS = timespecToNanoseconds(st.st_atimespec);
			m_mtimeNS = timespecToNanoseconds(st.st_mtimespec);
			m_ctimeNS = timespecToNanoseconds(st.st_ctimespec);
			m_birthTimeNS = timespecToNanoseconds(st.st_birthtimespec);
		}

#elif IS_WINDOWS
		/* Windows provides all times through GetFileAttributesEx or FindFirstFile. */
		WIN32_FILE_ATTRIBUTE_DATA fileInfo{};

		if ( GetFileAttributesExW(m_file.c_str(), GetFileExInfoStandard, &fileInfo) )
		{
			m_atimeNS = filetimeToNanoseconds(fileInfo.ftLastAccessTime);
			m_mtimeNS = filetimeToNanoseconds(fileInfo.ftLastWriteTime);
			m_ctimeNS = filetimeToNanoseconds(fileInfo.ftCreationTime);
			m_birthTimeNS = m_ctimeNS; /* On Windows, creation time IS birthtime. */
		}
#endif
	}
}
