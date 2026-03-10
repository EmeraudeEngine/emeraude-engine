/*
 * src/Libs/WaveFactory/FileFormatInterface.hpp
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
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Types.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Stream-based format interface for reading and writing audio data.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatInterface
	{
		public:

			FileFormatInterface (const FileFormatInterface &) noexcept = default;
			FileFormatInterface (FileFormatInterface &&) noexcept = default;
			FileFormatInterface & operator= (const FileFormatInterface &) noexcept = default;
			FileFormatInterface & operator= (FileFormatInterface &&) noexcept = default;
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Reads audio data from a byte stream into a wave.
			 * @param stream A reference to the input byte stream.
			 * @param wave A reference to the destination wave.
			 * @param options Read options (synthesis frequency, soundfont, etc.).
			 * @return bool
			 */
			virtual bool readStream (IO::ByteStream & stream, Wave< precision_t > & wave, const ReadOptions & options = {}) noexcept = 0;

			/**
			 * @brief Writes audio data from a wave into a byte stream.
			 * @param stream A reference to the output byte stream.
			 * @param wave A read-only reference to the source wave.
			 * @param options Write options (output format, etc.).
			 * @return bool
			 */
			virtual bool writeStream (IO::ByteStream & stream, const Wave< precision_t > & wave, const WriteOptions & options = {}) const noexcept = 0;

		protected:

			FileFormatInterface () noexcept = default;
	};
}