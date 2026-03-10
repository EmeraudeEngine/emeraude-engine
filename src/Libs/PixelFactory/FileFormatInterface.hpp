/*
 * src/Libs/PixelFactory/FileFormatInterface.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Pixmap.hpp"
#include "Types.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief File format interface for reading and writing a pixmap via byte streams.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatInterface
	{
		public:

			FileFormatInterface (const FileFormatInterface &) noexcept = default;
			FileFormatInterface (FileFormatInterface &&) noexcept = default;
			FileFormatInterface & operator= (const FileFormatInterface &) noexcept = default;
			FileFormatInterface & operator= (FileFormatInterface &&) noexcept = default;
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Reads a pixmap from a byte stream.
			 * @note The output pixmap is in canonical form: top-left origin, native channel mode.
			 * @param stream A reference to the input byte stream.
			 * @param pixmap A reference to the destination pixmap.
			 * @return bool
			 */
			virtual bool readStream (IO::ByteStream & stream, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept = 0;

			/**
			 * @brief Writes a pixmap to a byte stream.
			 * @param stream A reference to the output byte stream.
			 * @param pixmap A read-only reference to the source pixmap.
			 * @param options Write options (format-specific settings, Y-axis inversion).
			 * @return bool
			 */
			virtual bool writeStream (IO::ByteStream & stream, const Pixmap< pixel_data_t, dimension_t > & pixmap, const WriteOptions & options = {}) const noexcept = 0;

		protected:

			FileFormatInterface () noexcept = default;
	};
}
