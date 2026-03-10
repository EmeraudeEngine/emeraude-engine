/*
 * src/Libs/IO/MemoryStream.hpp
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
#include <cstring>
#include <vector>

/* Local inclusions for inheritances. */
#include "ByteStream.hpp"

namespace EmEn::Libs::IO
{
	/**
	 * @brief Memory-backed byte stream implementation.
	 * @note In read mode, wraps an existing data buffer with a read cursor.
	 * In write mode, writes to a provided output vector with random access support.
	 */
	class MemoryStream final : public ByteStream
	{
		public:

			/**
			 * @brief Constructs a read-mode memory stream from a byte vector.
			 * @param data A reference to the source data vector.
			 */
			explicit
			MemoryStream (const std::vector< std::byte > & data) noexcept
				: m_readData(data.data())
				, m_dataSize(data.size())
			{

			}

			/**
			 * @brief Constructs a read-mode memory stream from a raw pointer and size.
			 * @param data Pointer to the source data.
			 * @param size Size of the source data in bytes.
			 */
			MemoryStream (const std::byte * data, size_t size) noexcept
				: m_readData(data)
				, m_dataSize(size)
			{

			}

			/**
			 * @brief Constructs a write-mode memory stream with an output vector.
			 * @param output A reference to the destination vector. Supports random access writes.
			 */
			explicit
			MemoryStream (std::vector< std::byte > & output) noexcept
				: m_writeBuffer(&output)
			{

			}

			/** @copydoc ByteStream::read() */
			bool
			read (void * data, size_t size) noexcept override
			{
				if ( m_readData == nullptr || m_position + size > m_dataSize )
				{
					return false;
				}

				std::memcpy(data, m_readData + m_position, size);

				m_position += size;

				return true;
			}

			/** @copydoc ByteStream::write() */
			bool
			write (const void * data, size_t size) noexcept override
			{
				if ( m_writeBuffer == nullptr )
				{
					return false;
				}

				const auto endPos = m_writePosition + size;

				/* Extend buffer if writing past current size. */
				if ( endPos > m_writeBuffer->size() )
				{
					m_writeBuffer->resize(endPos);
				}

				std::memcpy(m_writeBuffer->data() + m_writePosition, data, size);

				m_writePosition += size;

				return true;
			}

			/** @copydoc ByteStream::isOpen() */
			[[nodiscard]]
			bool
			isOpen () const noexcept override
			{
				return m_readData != nullptr || m_writeBuffer != nullptr;
			}

			/** @copydoc ByteStream::size() */
			[[nodiscard]]
			size_t
			size () const noexcept override
			{
				if ( m_readData != nullptr )
				{
					return m_dataSize;
				}

				if ( m_writeBuffer != nullptr )
				{
					return m_writeBuffer->size();
				}

				return 0;
			}

			/** @copydoc ByteStream::tell() */
			[[nodiscard]]
			int64_t
			tell () noexcept override
			{
				if ( m_readData != nullptr )
				{
					return static_cast< int64_t >(m_position);
				}

				if ( m_writeBuffer != nullptr )
				{
					return static_cast< int64_t >(m_writePosition);
				}

				return -1;
			}

			/** @copydoc ByteStream::seek() */
			[[nodiscard]]
			int64_t
			seek (int64_t offset, int whence) noexcept override
			{
				const auto totalSize = static_cast< int64_t >(this->size());
				int64_t newPos = 0;

				if ( whence == 0 ) /* SEEK_SET */
				{
					newPos = offset;
				}
				else if ( whence == 1 ) /* SEEK_CUR */
				{
					newPos = this->tell() + offset;
				}
				else if ( whence == 2 ) /* SEEK_END */
				{
					newPos = totalSize + offset;
				}

				if ( newPos < 0 )
				{
					return -1;
				}

				if ( m_readData != nullptr )
				{
					if ( newPos > totalSize )
					{
						return -1;
					}

					m_position = static_cast< size_t >(newPos);
				}
				else if ( m_writeBuffer != nullptr )
				{
					m_writePosition = static_cast< size_t >(newPos);

					/* Extend buffer if seeking past end. */
					if ( m_writePosition > m_writeBuffer->size() )
					{
						m_writeBuffer->resize(m_writePosition);
					}
				}

				return newPos;
			}

			/** @copydoc ByteStream::isMemoryBacked() */
			[[nodiscard]]
			bool
			isMemoryBacked () const noexcept override
			{
				return true;
			}

			/** @copydoc ByteStream::data() */
			[[nodiscard]]
			const std::byte *
			data () const noexcept override
			{
				return m_readData;
			}

		private:

			const std::byte * m_readData{nullptr};
			size_t m_dataSize{0};
			size_t m_position{0};
			std::vector< std::byte > * m_writeBuffer{nullptr};
			size_t m_writePosition{0};
	};
}
