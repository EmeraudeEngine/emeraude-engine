/*
 * src/Libs/IO/ByteStream.hpp
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
#include <cstddef>
#include <cstdint>

namespace EmEn::Libs::IO
{
	/**
	 * @brief Abstract byte stream interface for reading and writing binary data.
	 * @note Implementations provide file-backed or memory-backed I/O.
	 * A given stream instance operates in either read or write mode, not both.
	 * Shared by PixelFactory, WaveFactory, VertexFactory, and other I/O subsystems.
	 */
	class ByteStream
	{
		public:

			ByteStream (const ByteStream &) noexcept = default;

			ByteStream (ByteStream &&) noexcept = default;

			ByteStream & operator= (const ByteStream &) noexcept = default;

			ByteStream & operator= (ByteStream &&) noexcept = default;

			virtual ~ByteStream () = default;

			/**
			 * @brief Reads bytes from the stream into the destination buffer.
			 * @param data Pointer to the destination buffer.
			 * @param size Number of bytes to read.
			 * @return bool True if all bytes were read successfully.
			 */
			virtual bool read (void * data, size_t size) noexcept = 0;

			/**
			 * @brief Writes bytes from the source buffer into the stream.
			 * @param data Pointer to the source buffer.
			 * @param size Number of bytes to write.
			 * @return bool True if all bytes were written successfully.
			 */
			virtual bool write (const void * data, size_t size) noexcept = 0;

			/**
			 * @brief Returns whether the stream is open and operational.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isOpen () const noexcept = 0;

			/**
			 * @brief Returns the total size of the underlying data source in bytes.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t size () const noexcept = 0;

			/**
			 * @brief Returns the current read/write position in the stream.
			 * @note Required by libraries that need seekable I/O (e.g., libsndfile).
			 * @return int64_t Current position, or -1 if unsupported.
			 */
			[[nodiscard]]
			virtual int64_t tell () noexcept
			{
				return -1;
			}

			/**
			 * @brief Seeks to a position in the stream.
			 * @note Required by libraries that need seekable I/O (e.g., libsndfile).
			 * @param offset The offset to seek to.
			 * @param whence 0 = from beginning (SEEK_SET), 1 = from current (SEEK_CUR), 2 = from end (SEEK_END).
			 * @return int64_t The new position after seeking, or -1 on error.
			 */
			[[nodiscard]]
			virtual int64_t seek (int64_t offset, int whence) noexcept
			{
				(void)offset;
				(void)whence;

				return -1;
			}

			/**
			 * @brief Returns whether the stream is backed by a memory buffer.
			 * @note Used by format handlers that can optimize for direct buffer access (e.g., libjpeg).
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isMemoryBacked () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns a pointer to the underlying read data buffer.
			 * @note Only valid for memory-backed streams in read mode. Returns nullptr otherwise.
			 * @return const std::byte *
			 */
			[[nodiscard]]
			virtual const std::byte * data () const noexcept
			{
				return nullptr;
			}

		protected:

			ByteStream () noexcept = default;
	};
}
