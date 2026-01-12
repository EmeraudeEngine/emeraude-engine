/*
 * src/Libs/Compression/ZLIB.cpp
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

#include "ZLIB.hpp"

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Third-party inclusions. */
#include <zlib.h>

namespace EmEn::Libs::Compression::ZLIB
{
	size_t
	compressStream (std::istream & sourceStream, std::ostream & targetStream, size_t chunkSize, int level) noexcept
	{
		auto uLongf_chunkSize = static_cast< uLongf >(chunkSize);
		sourceStream.seekg(0, std::istream::end);
		const std::streamoff finalPosition = sourceStream.tellg();
		sourceStream.seekg(0, std::istream::beg);

		if ( finalPosition <= 0 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", nothing to compress !" "\n";

			return 0;
		}

		/* Gets length of stream. */
		auto streamSize = static_cast< uLongf >(finalPosition);

		size_t steppes = 0;

		if ( streamSize < uLongf_chunkSize )
		{
			uLongf_chunkSize = streamSize;

			steppes = 1;
		}
		else
		{
			steppes = streamSize / uLongf_chunkSize;

			if ( streamSize % uLongf_chunkSize > 0 )
			{
				steppes++;
			}
		}

		/* Compression result size. */
		size_t compressedSize = 0;

		/* Uncompressed source stream will go here. */
		std::string source{};
		source.resize(uLongf_chunkSize);

		/* Compressed buffer will go here and read back to target stream. */
		std::string destination{};

		for ( size_t step = 0; step < steppes; step++ )
		{
			/* Read chunk size byte of uncompressed data. */
			const auto readSize = static_cast< std::streamsize >(uLongf_chunkSize * sizeof(char));

			sourceStream.read(source.data(), readSize);

			/* Prepare the size of the destination buffer by estimate it. */
			auto destinationSize = compressBound(uLongf_chunkSize);

			destination.resize(destinationSize);

			/* Compression and retrieve the real size of the destination buffer. */
			auto * dest = reinterpret_cast< Bytef * >(destination.data());
			const auto * src = reinterpret_cast< const Bytef * >(source.c_str());

			const auto error = compress2(dest, &destinationSize, src, uLongf_chunkSize, level);

			if ( error > 0 )
			{
				std::cerr << __PRETTY_FUNCTION__ << ", " << zError(error) << "\n";

				return 0;
			}

			/* Write the base size and the compressed size. */
			targetStream.write(reinterpret_cast< const char * >(&uLongf_chunkSize), sizeof(size_t));
			targetStream.write(reinterpret_cast< const char * >(&destinationSize), sizeof(size_t));

			/* Write compressed data. */
			const auto writeSize = static_cast< std::streamsize >(destinationSize * sizeof(char));

			targetStream.write(destination.c_str(), writeSize);

			/* Adding compressed data chunk size to the total. */
			compressedSize += destinationSize;

			/* Compute data left to compress and reduce
			 * the chunk size if we are at the end of the stream. */
			if ( streamSize > uLongf_chunkSize )
			{
				streamSize -= uLongf_chunkSize;
			}

			if ( streamSize < uLongf_chunkSize )
			{
				uLongf_chunkSize = streamSize;
				source.resize(uLongf_chunkSize);
			}
		}

		return compressedSize;
	}

	bool
	decompressStream (std::istream & sourceStream, std::ostream & targetStream) noexcept
	{
		/* Compressed source stream will go here. */
		std::string source;
		/* Uncompressed buffer will go here
		 * and read back to target stream. */
		std::string destination;

		while ( sourceStream.good() )
		{
			/* Read to size_t variable the uncompressed size. */
			uLongf baseSize = 0;

			sourceStream.read(reinterpret_cast< char * >(&baseSize), sizeof(size_t));

			/* If the chunk header says nothing,
			 * we are at the end of the stream. */
			if ( baseSize == 0 )
			{
				return true;
			}

			/* Read to size_t variable the compressed size. */
			uLongf compressedSize = 0;

			sourceStream.read(reinterpret_cast< char * >(&compressedSize), sizeof(size_t));

			/* Resize both of buffer before writing into. */
			source.resize(compressedSize);
			destination.resize(baseSize);

			/* Read some bytes of compressed data. */
			const auto readSize = static_cast< std::streamsize >(compressedSize * sizeof(char));

			if ( sourceStream.read(source.data(), readSize) )
			{
				auto * dest = reinterpret_cast< Bytef * >(destination.data());
				const auto * src = reinterpret_cast< const Bytef * >(source.c_str());

				if ( uncompress(dest, &baseSize, src, compressedSize) > 0 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to uncompress stream." "\n";

					return false;
				}

				/* Write uncompressed data to destination string. */
				const auto writeSize = static_cast< std::streamsize >(baseSize * sizeof(char));

				targetStream.write(destination.c_str(), writeSize);
			}
			else
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to read compressed stream." "\n";

				return false;
			}
		}

		std::cerr << __PRETTY_FUNCTION__ << ", stream seems broken." "\n";

		return false;
	}

	/**
	 * @brief Compresses a string with ZLIB algorithm.
	 * @param input A reference to a string.
	 * @param output A writable reference to a string.
	 * @param level The compression level from 0 to 9. Default 9 (high).
	 * @return bool
	 */
	bool
	compressString (const std::string & input, std::string & output, int level) noexcept
	{
		std::stringstream sourceStream;
		sourceStream << input;

		std::stringstream outputStream;

		if ( !compressStream(sourceStream, outputStream, level) )
		{
			return false;
		}

		output = outputStream.str();

		return true;
	}

	/**
	 * @brief Decompresses a string with ZLIB algorithm.
	 * @param input A reference to a string.
	 * @param output A writable reference to a string.
	 * @return bool
	 */
	bool
	decompressString (const std::string & input, std::string & output) noexcept
	{
		std::stringstream sourceStream;
		sourceStream << input;

		std::stringstream outputStream;

		if ( !decompressStream(sourceStream, outputStream) )
		{
			return false;
		}

		output = outputStream.str();

		return true;
	}
}
