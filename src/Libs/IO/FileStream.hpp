/*
 * src/Libs/IO/FileStream.hpp
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
#include <filesystem>
#include <fstream>

/* Local inclusions for inheritances. */
#include "ByteStream.hpp"

namespace EmEn::Libs::IO
{
	/**
	 * @brief File-backed byte stream implementation.
	 * @note Opens a file in either read or write mode (binary).
	 */
	class FileStream final : public ByteStream
	{
		public:

			/** @brief Stream mode for file access. */
			enum class Mode : uint8_t
			{
				Read,
				Write
			};

			/**
			 * @brief Constructs a file stream.
			 * @param filepath Path to the file.
			 * @param mode Read or Write mode.
			 */
			FileStream (const std::filesystem::path & filepath, Mode mode) noexcept
				: m_mode(mode)
			{
				if ( mode == Mode::Read )
				{
					m_input.open(filepath, std::ios::binary);

					if ( m_input.is_open() )
					{
						m_input.seekg(0, std::ios::end);

						const auto pos = m_input.tellg();

						m_size = pos >= 0 ? static_cast< size_t >(pos) : 0;

						m_input.seekg(0, std::ios::beg);
					}
				}
				else
				{
					m_output.open(filepath, std::ios::binary);
				}
			}

			/** @copydoc ByteStream::read() */
			bool
			read (void * data, size_t size) noexcept override
			{
				if ( m_mode != Mode::Read || !m_input.is_open() )
				{
					return false;
				}

				m_input.read(static_cast< char * >(data), static_cast< std::streamsize >(size));

				return !m_input.fail();
			}

			/** @copydoc ByteStream::write() */
			bool
			write (const void * data, size_t size) noexcept override
			{
				if ( m_mode != Mode::Write || !m_output.is_open() )
				{
					return false;
				}

				m_output.write(static_cast< const char * >(data), static_cast< std::streamsize >(size));

				return !m_output.fail();
			}

			/** @copydoc ByteStream::isOpen() */
			[[nodiscard]]
			bool
			isOpen () const noexcept override
			{
				return m_mode == Mode::Read ? m_input.is_open() : m_output.is_open();
			}

			/** @copydoc ByteStream::size() */
			[[nodiscard]]
			size_t
			size () const noexcept override
			{
				return m_size;
			}

			/** @copydoc ByteStream::tell() */
			[[nodiscard]]
			int64_t
			tell () noexcept override
			{
				if ( m_mode == Mode::Read )
				{
					const auto pos = m_input.tellg();

					return pos >= 0 ? static_cast< int64_t >(pos) : -1;
				}

				const auto pos = m_output.tellp();

				return pos >= 0 ? static_cast< int64_t >(pos) : -1;
			}

			/** @copydoc ByteStream::seek() */
			[[nodiscard]]
			int64_t
			seek (int64_t offset, int whence) noexcept override
			{
				auto direction = std::ios::beg;

				if ( whence == 1 )
				{
					direction = std::ios::cur;
				}
				else if ( whence == 2 )
				{
					direction = std::ios::end;
				}

				if ( m_mode == Mode::Read )
				{
					m_input.seekg(offset, direction);
				}
				else
				{
					m_output.seekp(offset, direction);
				}

				return this->tell();
			}

		private:

			Mode m_mode;
			std::ifstream m_input;
			std::ofstream m_output;
			size_t m_size{0};
	};
}
