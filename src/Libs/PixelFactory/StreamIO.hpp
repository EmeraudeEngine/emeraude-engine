/*
 * src/Libs/PixelFactory/StreamIO.hpp
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
#include <iostream>
#include <type_traits>
#include <vector>

/* Local inclusions for usages. */
#include "FileFormatJpeg.hpp"
#include "FileFormatPNG.hpp"
#include "FileFormatTarga.hpp"
#include "IOCommon.hpp"
#include "Libs/IO/MemoryStream.hpp"
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory::StreamIO
{
	/**
	 * @brief Decodes a pixmap from a memory buffer.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @param data A reference to the source byte vector.
	 * @param format The image format to decode as.
	 * @param pixmap A reference to the destination pixmap.
	 * @param options Read post-processing options. Default: no transformations.
	 * @return bool
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	[[nodiscard]]
	bool
	read (const std::vector< std::byte > & data, typename Pixmap< pixel_data_t, dimension_t >::Format format, Pixmap< pixel_data_t, dimension_t > & pixmap, const ReadOptions & options = {}) requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	{
		if ( data.empty() )
		{
			std::cerr << "PixelFactory::StreamIO::read(), empty input buffer !" "\n";

			return false;
		}

		IO::MemoryStream stream{data};

		bool decoded = false;

		switch ( format )
		{
			case Pixmap< pixel_data_t, dimension_t >::Format::Jpeg :
			{
				FileFormatJpeg< pixel_data_t, dimension_t > fileFormat;

				decoded = fileFormat.readStream(stream, pixmap);
			}
				break;

			case Pixmap< pixel_data_t, dimension_t >::Format::PNG :
			{
				FileFormatPNG< pixel_data_t, dimension_t > fileFormat;

				decoded = fileFormat.readStream(stream, pixmap);
			}
				break;

			case Pixmap< pixel_data_t, dimension_t >::Format::Targa :
			{
				FileFormatTarga< pixel_data_t, dimension_t > fileFormat;

				decoded = fileFormat.readStream(stream, pixmap);
			}
				break;

			default:
				std::cerr << "PixelFactory::StreamIO::read(), unhandled format !" "\n";

				return false;
		}

		if ( !decoded )
		{
			return false;
		}

		/* Apply read post-processing options. */
		return applyReadOptions(pixmap, options);
	}

	/**
	 * @brief Encodes a pixmap into a memory buffer.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @param pixmap A reference to the source pixmap.
	 * @param format The image format to encode as.
	 * @param output A reference to the destination byte vector. Will be cleared before writing.
	 * @param options Write encoding options. Default: format defaults.
	 * @return bool
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	[[nodiscard]]
	bool
	write (const Pixmap< pixel_data_t, dimension_t > & pixmap, typename Pixmap< pixel_data_t, dimension_t >::Format format, std::vector< std::byte > & output, const WriteOptions & options = {}) requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	{
		output.clear();

		IO::MemoryStream stream{output};

		switch ( format )
		{
			case Pixmap< pixel_data_t, dimension_t >::Format::Jpeg :
			{
				FileFormatJpeg< pixel_data_t, dimension_t > fileFormat;

				return fileFormat.writeStream(stream, pixmap, options);
			}

			case Pixmap< pixel_data_t, dimension_t >::Format::PNG :
			{
				FileFormatPNG< pixel_data_t, dimension_t > fileFormat;

				return fileFormat.writeStream(stream, pixmap, options);
			}

			case Pixmap< pixel_data_t, dimension_t >::Format::Targa :
			{
				FileFormatTarga< pixel_data_t, dimension_t > fileFormat;

				return fileFormat.writeStream(stream, pixmap, options);
			}

			default:
				std::cerr << "PixelFactory::StreamIO::write(), unhandled format !" "\n";

				return false;
		}
	}
}
