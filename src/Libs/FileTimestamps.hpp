/*
 * src/Libs/FileTimestamps.hpp
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

#pragma once

/* STL inclusions. */
#include <filesystem>

namespace EmEn::Libs
{
	/**
	 * @brief Cross-platform file timestamps retrieval.
	 *
	 * This class provides access to file timestamps in a cross-platform manner.
	 * Due to platform differences, not all timestamps are available on all systems:
	 *
	 * - **atime** (access time): Available on all platforms via native APIs.
	 * - **mtime** (modification time): Available on all platforms via STL and native APIs.
	 * - **ctime**: Platform-dependent meaning:
	 *   - Linux/macOS: Last status change time (metadata modification)
	 *   - Windows: File creation time
	 * - **birthtime** (creation time): True creation time when available:
	 *   - Linux: Available on modern filesystems (ext4, btrfs, etc.) via statx
	 *   - macOS: Available via native APIs
	 *   - Windows: Same as ctime (creation time)
	 *
	 * All times are returned as milliseconds since Unix epoch (1970-01-01 00:00:00 UTC).
	 * If a timestamp is unavailable on the current platform, it will be set to 0.
	 *
	 * @note The class performs lazy fetching: timestamps are retrieved only when
	 *       one of the accessor methods is called for the first time.
	 */
	class FileTimestamps final
	{
		public:

			/**
			 * @brief Constructs a FileTimestamps object for the specified file.
			 * @param file The filesystem path to the file to query.
			 */
			explicit
			FileTimestamps (std::filesystem::path file) noexcept
				: m_file{std::move(file)}
			{

			}

			/**
			 * @brief Checks if file timestamps have been successfully fetched.
			 * @return true if timestamps were fetched, false otherwise.
			 */
			[[nodiscard]]
			bool
			isFileTimestampsFetched () const noexcept
			{
				return m_fetched;
			}

			/**
			 * @brief Returns the last access time in nanoseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Access time in nanoseconds, or 0 if unavailable.
			 */
			[[nodiscard]]
			uint64_t
			atimeNS () noexcept
			{
				if ( !m_fetched )
				{
					this->fetchInfo();
				}

				return m_atimeNS;
			}

			/**
			 * @brief Returns the last access time in milliseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Access time in milliseconds (with sub-millisecond precision), or 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			atimeMS () noexcept
			{
				return static_cast< double >(this->atimeNS()) / 1000000.0;
			}

			/**
			 * @brief Returns the last access time in seconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Access time in seconds (with sub-second precision), or 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			atimeS () noexcept
			{
				return static_cast< double >(this->atimeNS()) / 1000000000.0;
			}

			/**
			 * @brief Returns the last modification time in nanoseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Modification time in nanoseconds, or 0 if unavailable.
			 */
			[[nodiscard]]
			uint64_t
			mtimeNS () noexcept
			{
				if ( !m_fetched )
				{
					this->fetchInfo();
				}

				return m_mtimeNS;
			}

			/**
			 * @brief Returns the last modification time in milliseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Modification time in milliseconds (with sub-millisecond precision), or 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			mtimeMS () noexcept
			{
				return static_cast< double >(this->mtimeNS()) / 1000000.0;
			}

			/**
			 * @brief Returns the last modification time in seconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @return Modification time in seconds (with sub-second precision), or 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			mtimeS () noexcept
			{
				return static_cast< double >(this->mtimeNS()) / 1000000000.0;
			}

			/**
			 * @brief Returns the ctime in nanoseconds since Unix epoch.
			 *
			 * @note This triggers lazy fetching if not already done.
			 * @note The meaning of ctime is platform-dependent. Use birthTimeNS() for
			 *       true creation time when available.
			 * @return On Linux/macOS: last status change time. On Windows: creation time.
			 *       Returns 0 if unavailable.
			 */
			[[nodiscard]]
			uint64_t
			ctimeNS () noexcept
			{
				if ( !m_fetched )
				{
					this->fetchInfo();
				}

				return m_ctimeNS;
			}

			/**
			 * @brief Returns the ctime in milliseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @note The meaning of ctime is platform-dependent. Use birthTimeMS() for
			 *       true creation time when available.
			 * @return On Linux/macOS: last status change time. On Windows: creation time.
			 *       Returns 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			ctimeMS () noexcept
			{
				return static_cast< double >(this->ctimeNS()) / 1000000.0;
			}

			/**
			 * @brief Returns the ctime in seconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @note The meaning of ctime is platform-dependent. Use birthTimeS() for
			 *       true creation time when available.
			 * @return On Linux/macOS: last status change time. On Windows: creation time.
			 *       Returns 0.0 if unavailable.
			 */
			[[nodiscard]]
			double
			ctimeS () noexcept
			{
				return static_cast< double >(this->ctimeNS()) / 1000000000.0;
			}

			/**
			 * @brief Returns the file creation time (birth time) in nanoseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @note Availability:
			 *       - Linux: Requires kernel 4.11+ and modern filesystem (ext4, btrfs, etc.)
			 *       - macOS: Always available
			 *       - Windows: Always available (same as ctime)
			 * @return Creation time in nanoseconds, or 0 if unavailable on this platform.
			 */
			[[nodiscard]]
			uint64_t
			birthTimeNS () noexcept
			{
				if ( !m_fetched )
				{
					this->fetchInfo();
				}

				return m_birthTimeNS;
			}

			/**
			 * @brief Returns the file creation time (birth time) in milliseconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @note Availability:
			 *       - Linux: Requires kernel 4.11+ and modern filesystem (ext4, btrfs, etc.)
			 *       - macOS: Always available
			 *       - Windows: Always available (same as ctime)
			 * @return Creation time in milliseconds (with sub-millisecond precision), or 0.0 if unavailable on this platform.
			 */
			[[nodiscard]]
			double
			birthTimeMS () noexcept
			{
				return static_cast< double >(this->birthTimeNS()) / 1000000.0;
			}

			/**
			 * @brief Returns the file creation time (birth time) in seconds since Unix epoch.
			 * @note This triggers lazy fetching if not already done.
			 * @note Availability:
			 *       - Linux: Requires kernel 4.11+ and modern filesystem (ext4, btrfs, etc.)
			 *       - macOS: Always available
			 *       - Windows: Always available (same as ctime)
			 * @return Creation time in seconds (with sub-second precision), or 0.0 if unavailable on this platform.
			 */
			[[nodiscard]]
			double
			birthTimeS () noexcept
			{
				return static_cast< double >(this->birthTimeNS()) / 1000000000.0;
			}

		private:

			/**
			 * @brief Fetches file timestamps from the filesystem.
			 * @note This method is called automatically by accessor methods (lazy fetching).
			 * @return void
			 */
			void fetchInfo () noexcept;

			uint64_t m_atimeNS{0};
			uint64_t m_mtimeNS{0};
			uint64_t m_ctimeNS{0};
			uint64_t m_birthTimeNS{0};
			std::filesystem::path m_file;
			bool m_fetched{false};
	};
}
