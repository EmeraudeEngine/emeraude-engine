/*
 * src/Libs/Compression/LZMA/Decompressor.cpp
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

#include "Decompressor.hpp"

/* STL inclusions. */
#include <iostream>
#include <array>

namespace EmEn::Libs::Compression::LZMA
{
	Decompressor::Decompressor (uint32_t maxThreads) noexcept
	{
		if ( maxThreads > 0 )
		{
			m_ready = this->initMT(maxThreads);
		}
		else
		{
			m_ready = this->init();
		}
	}

	Decompressor::~Decompressor ()
	{
		lzma_end(&m_stream);
	}

	bool
	Decompressor::decompressString (const std::string & input, std::string & output) noexcept
	{
		std::stringstream sourceStream;
		sourceStream << input;

		std::stringstream outputStream;

		if ( !this->decompress(sourceStream, outputStream) )
		{
			return false;
		}

		output = outputStream.str();

		return true;
	}

	bool
	Decompressor::init () noexcept
	{
		const auto ret = lzma_stream_decoder(&m_stream, UINT64_MAX, LZMA_CONCATENATED);

		if ( ret != LZMA_OK )
		{
			switch ( ret )
			{
				case LZMA_MEM_ERROR :
					std::cerr << "LZMA decoder init error : Memory allocation failed !" "\n";
					break;

				case LZMA_OPTIONS_ERROR :
					std::cerr << "LZMA decoder init error : Specified filter chain is not supported !" "\n";
					break;

				default :
					std::cerr << "LZMA decoder init error : Unknown error, possibly a bug !" "\n";
					break;
			}

			return false;
		}

		return true;
	}

	bool
	Decompressor::initMT (uint32_t maxThreads) noexcept
	{
		lzma_mt settings{};
		settings.flags = LZMA_CONCATENATED;
		settings.threads = std::max(std::min(1U, lzma_cputhreads()), maxThreads);
		settings.timeout = 0;
		settings.memlimit_threading = UINT64_MAX;
		settings.memlimit_stop = UINT64_MAX;

		const auto ret = lzma_stream_decoder_mt(&m_stream, &settings);

		if ( ret != LZMA_OK )
		{
			switch ( ret )
			{
				case LZMA_MEM_ERROR :
					std::cerr << "LZMA encoder init (MT) error : Memory allocation failed !" "\n";
					break;

				case LZMA_OPTIONS_ERROR :
					std::cerr << "LZMA encoder init (MT) error : Specified filter chain is not supported !" "\n";
					break;

				case LZMA_UNSUPPORTED_CHECK :
					std::cerr << "LZMA encoder init (MT) error : Specified integrity check is not supported !" "\n";
					break;

				default :
					std::cerr << "LZMA encoder init (MT) error : Unknown error, possibly a bug !" "\n";
					break;
			}

			return false;
		}

		return true;
	}

	bool
	Decompressor::decompress (std::stringstream & input, std::stringstream & output) noexcept
	{
		if ( !m_ready )
		{
			return false;
		}

		lzma_action action = LZMA_RUN;

		std::array< uint8_t, 8192 > inbuf{0};
		std::array< uint8_t, 8192 > outbuf{0};

		m_stream.next_in = nullptr;
		m_stream.avail_in = 0;
		m_stream.next_out = outbuf.data();
		m_stream.avail_out = outbuf.size();

		while ( true )
		{
			if ( m_stream.avail_in == 0 && !input.eof() )
			{
				m_stream.next_in = inbuf.data();

				if ( input.read(reinterpret_cast< char * >(inbuf.data()), inbuf.size()).bad() )
				{
					std::cerr << "LZMA decoder error: Read error !" "\n";

					return false;
				}

				m_stream.avail_in = input.gcount();

				if ( input.eof() )
				{
					action = LZMA_FINISH;
				}
			}

			const auto ret = lzma_code(&m_stream, action);

			if ( m_stream.avail_out == 0 || ret == LZMA_STREAM_END )
			{
				if ( output.write(reinterpret_cast< char * >(outbuf.data()), static_cast< long >(outbuf.size() - m_stream.avail_out)).bad() )
				{
					std::cerr << "LZMA decoder error: Write error !" "\n";

					return false;
				}

				m_stream.next_out = outbuf.data();
				m_stream.avail_out = outbuf.size();
			}

			if ( ret != LZMA_OK )
			{
				if ( ret == LZMA_STREAM_END )
				{
					return true;
				}

				switch ( ret )
				{
					case LZMA_MEM_ERROR :
						std::cerr << "LZMA decoder error: Memory allocation failed !" "\n";
						break;

					case LZMA_FORMAT_ERROR :
						std::cerr << "LZMA decoder error: The input is not in the .xz format !" "\n";
						break;

					case LZMA_OPTIONS_ERROR :
						std::cerr << "LZMA decoder error: Unsupported compression options !" "\n";
						break;

					case LZMA_DATA_ERROR :
						std::cerr << "LZMA decoder error: Compressed file is corrupt !" "\n";
						break;

					case LZMA_BUF_ERROR :
						std::cerr << "LZMA decoder error: Compressed file is truncated or otherwise corrupt !" "\n";
						break;

					default :
						std::cerr << "LZMA decoder error: Unknown error, possibly a bug !" "\n";
						break;
				}

				return false;
			}
		}
	}
}
