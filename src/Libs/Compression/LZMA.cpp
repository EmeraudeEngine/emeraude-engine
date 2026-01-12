/*
 * src/Libs/Compression/LZMA.cpp
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

#include "LZMA.hpp"

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Third-party inclusions. */
#include "lzma.h"

namespace EmEn::Libs::Compression::LZMA
{
	bool
	compressString (const std::string & input, std::string & output, int level) noexcept
	{
		output.clear();
		output.resize(input.size() + (input.size() >> 2) + 128);

		size_t outPosition = 0;

		const auto outputCode = lzma_easy_buffer_encode(
			level,
			LZMA_CHECK_CRC32,
			nullptr,
			reinterpret_cast< const uint8_t * >(input.data()),
			input.size(),
			reinterpret_cast< uint8_t * >(output.data()),
			&outPosition,
			output.size()
		);

		if ( outputCode != LZMA_OK )
		{
			std::cerr << "Unable to init a LZMA compression !" "\n";

			return false;
		}

		output.resize(outPosition);

		return true;
	}

	bool
	decompressString (const std::string & input, std::string & output) noexcept
	{
		static constexpr size_t kMemLimit = 1 << 30;  // 1 GB.

		lzma_stream stream = LZMA_STREAM_INIT;

		output.clear();
		output.resize(8192);

		size_t resultUsed = 0;

		if ( lzma_stream_decoder(&stream, kMemLimit, LZMA_CONCATENATED) != LZMA_OK )
		{
			return false;
		}

		size_t avail0 = output.size();
		stream.next_in = reinterpret_cast< const uint8_t * >(input.data());
		stream.avail_in = input.size();
		stream.next_out = reinterpret_cast< uint8_t * >(output.data());
		stream.avail_out = avail0;

		while ( true )
		{
			const lzma_ret outputCode = lzma_code(&stream, stream.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);

			if ( outputCode == LZMA_STREAM_END )
			{
				resultUsed += avail0 - stream.avail_out;

				if ( stream.avail_in != 0 )  // Guaranteed by lzma_stream_decoder().
				{
					return false;
				}

				output.resize(resultUsed);

				lzma_end(&stream);

				return true;
			}

			if ( outputCode != LZMA_OK )
			{
				return false;
			}

			if ( stream.avail_out == 0 )
			{
				resultUsed += avail0 - stream.avail_out;

				output.resize(output.size() << 1);

				stream.next_out = reinterpret_cast< uint8_t * >(output.data() + resultUsed);
				stream.avail_out = avail0 = output.size() - resultUsed;
			}
		}
	}
}
