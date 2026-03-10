/*
 * src/Libs/WaveFactory/FileIO.hpp
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
#include <filesystem>
#include <iostream>
#include <type_traits>

/* Local inclusions. */
#include "Libs/IO/FileStream.hpp"
#include "Libs/IO/IO.hpp"
#include "FileFormatJSON.hpp"
#include "FileFormatMIDI.hpp"
#include "FileFormatSNDFile.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory::FileIO
{
	/**
	 * @brief Reads a sound file into a wave structure.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @param filepath A reference to a filesystem path.
	 * @param wave A reference to the destination wave.
	 * @param options Read options (synthesis frequency, soundfont, etc.).
	 * @return bool
	 */
	template< typename precision_t = int16_t >
	[[nodiscard]]
	bool
	read (const std::filesystem::path & filepath, Wave< precision_t > & wave, const ReadOptions & options = {}) noexcept
		requires (std::is_arithmetic_v< precision_t >)
	{
		if ( !IO::fileExists(filepath) )
		{
			std::cerr << "[WaveFactory::FileIO] read(), the file '" << filepath << "' doesn't exist !\n";

			return false;
		}

		IO::FileStream stream{filepath, IO::FileStream::Mode::Read};

		if ( !stream.isOpen() )
		{
			std::cerr << "[WaveFactory::FileIO] read(), unable to open '" << filepath << "' !\n";

			return false;
		}

		const auto extension = IO::getFileExtension(filepath, true);

		if ( extension == "json" )
		{
			FileFormatJSON< precision_t > fileFormat;

			return fileFormat.readStream(stream, wave, options);
		}

		if ( extension == "mid" || extension == "midi" )
		{
			FileFormatMIDI< precision_t > fileFormat;

			return fileFormat.readStream(stream, wave, options);
		}

		/* All other audio formats are handled by libsndfile. */
		FileFormatSNDFile< precision_t > fileFormat;

		return fileFormat.readStream(stream, wave, options);
	}

	/**
	 * @brief Writes a wave structure to a sound file.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @param wave A reference to the source wave.
	 * @param filepath A reference to a filesystem path.
	 * @param overwrite Overwrite existing file. Default false.
	 * @param options Write options (output format, etc.).
	 * @return bool
	 */
	template< typename precision_t = int16_t >
	[[nodiscard]]
	bool
	write (const Wave< precision_t > & wave, const std::filesystem::path & filepath, bool overwrite = false, const WriteOptions & options = {}) noexcept
	requires (std::is_arithmetic_v< precision_t >)
	{
		if ( IO::fileExists(filepath) && !overwrite )
		{
			std::cerr << "[WaveFactory::FileIO] write(), the file '" << filepath << "' already exists !\n";

			return false;
		}

		const auto extension = IO::getFileExtension(filepath, true);

		if ( extension == "json" )
		{
			std::cerr << "[WaveFactory::FileIO] write(), JSON format is read-only (procedural audio) !\n";

			return false;
		}

		if ( extension == "mid" || extension == "midi" )
		{
			std::cerr << "[WaveFactory::FileIO] write(), MIDI format is read-only !\n";

			return false;
		}

		IO::FileStream stream{filepath, IO::FileStream::Mode::Write};

		if ( !stream.isOpen() )
		{
			std::cerr << "[WaveFactory::FileIO] write(), unable to open '" << filepath << "' for writing !\n";

			return false;
		}

		/* Infer audio format from file extension. */
		WriteOptions effectiveOptions = options;

		if ( extension == "flac" )
		{
			effectiveOptions.format = AudioFormat::FLAC;
		}
		else if ( extension == "ogg" || extension == "oga" )
		{
			effectiveOptions.format = AudioFormat::OGG;
		}
		else
		{
			effectiveOptions.format = AudioFormat::WAV;
		}

		/* All writable audio formats are handled by libsndfile. */
		FileFormatSNDFile< precision_t > fileFormat;

		return fileFormat.writeStream(stream, wave, effectiveOptions);
	}
}
