/*
 * src/Libs/WaveFactory/StreamIO.hpp
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
#include "Libs/IO/MemoryStream.hpp"
#include "FileFormatSNDFile.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory::StreamIO
{
	/**
	 * @brief Decodes audio data from a memory buffer into a wave.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @param data A reference to the source byte vector.
	 * @param wave A reference to the destination wave.
	 * @param options Read options (synthesis frequency, soundfont, etc.).
	 * @return bool
	 * @note Only supports formats handled by libsndfile (WAV, FLAC, OGG, etc.).
	 * JSON and MIDI are not supported here as they require file-specific context.
	 */
	template< typename precision_t = int16_t >
	[[nodiscard]]
	bool
	read (const std::vector< std::byte > & data, Wave< precision_t > & wave, const ReadOptions & options = {}) noexcept
		requires (std::is_arithmetic_v< precision_t >)
	{
		if ( data.empty() )
		{
			std::cerr << "[WaveFactory::StreamIO] read(), empty input buffer !\n";

			return false;
		}

		IO::MemoryStream stream{data};

		FileFormatSNDFile< precision_t > fileFormat;

		return fileFormat.readStream(stream, wave, options);
	}

	/**
	 * @brief Encodes a wave into a memory buffer.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @param wave A reference to the source wave.
	 * @param output A reference to the destination byte vector. Will be cleared before writing.
	 * @param options Write options (output format, etc.).
	 * @return bool
	 */
	template< typename precision_t = int16_t >
	[[nodiscard]]
	bool
	write (const Wave< precision_t > & wave, std::vector< std::byte > & output, const WriteOptions & options = {}) noexcept
		requires (std::is_arithmetic_v< precision_t >)
	{
		output.clear();

		IO::MemoryStream stream{output};

		FileFormatSNDFile< precision_t > fileFormat;

		return fileFormat.writeStream(stream, wave, options);
	}
}
