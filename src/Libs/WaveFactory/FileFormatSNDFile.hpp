/*
 * src/Libs/WaveFactory/FileFormatSNDFile.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>
#include <type_traits>

/* Third-party inclusions. */
#include "sndfile.h"

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/IO.hpp"
#include "Types.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Class for reading and writing audio formats via libsndfile.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @extends EmEn::Libs::WaveFactory::FileFormatInterface The base IO class.
	 * @note Supports WAV, FLAC, OGG, AIFF, and many other formats.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatSNDFile final : public FileFormatInterface< precision_t >
	{
		public:

			/**
			 * @brief Constructs a SNDFile format IO.
			 */
			FileFormatSNDFile () noexcept = default;

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Wave< precision_t > & wave) noexcept override
			{
				std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), precision format not handled for '" << filepath << "' !\n";

				return false;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & filepath, const Wave< precision_t > & wave) const noexcept override
			{
				std::cerr << "[WaveFactory::FileFormatSNDFile] writeFile(), precision format not handled for '" << filepath << "' !\n";

				return false;
			}
	};

	/**
	 * @brief Specialization for int16_t (16-bit PCM audio).
	 */
	template<>
	class FileFormatSNDFile< int16_t > final : public FileFormatInterface< int16_t >
	{
		public:

			/**
			 * @brief Constructs a SNDFile format IO.
			 */
			FileFormatSNDFile () noexcept = default;

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Wave< int16_t > & wave) noexcept override
			{
				if ( !IO::fileExists(filepath) )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), file '" << filepath << "' doesn't exist !\n";

					return false;
				}

				SF_INFO soundFileInfos;
				soundFileInfos.frames = 0;
				soundFileInfos.samplerate = 0;
				soundFileInfos.channels = 0;
				soundFileInfos.format = 0;
				soundFileInfos.sections = 0;
				soundFileInfos.seekable = 0;

				/* 1. Open file. */
				auto * file = sf_open(filepath.string().c_str(), SFM_READ, &soundFileInfos);

				if ( file == nullptr )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), unable to open sound file '" << filepath << "' !\n";

					return false;
				}

				auto isDataValid = true;

				/* 2. Read information. */
				const auto samples = static_cast< size_t >(soundFileInfos.frames);
				const auto channels = toChannels(soundFileInfos.channels);
				const auto frequency = toFrequency(soundFileInfos.samplerate);

				if ( channels == Channels::Invalid )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), invalid channels !\n";

					isDataValid = false;
				}

				if ( frequency == Frequency::Invalid )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), invalid frequency !\n";

					isDataValid = false;
				}

				if constexpr ( WaveFactoryDebugEnabled )
				{
					std::cout <<
						"[WaveFactory::FileFormatSNDFile] File loaded.\n"
						"\t" "Frames (Samples) : " << soundFileInfos.frames << "\n"
						"\t" "Sample rates (Frequency) : " << soundFileInfos.samplerate << " Hz\n"
						"\t" "Duration : " << ( static_cast< float >(soundFileInfos.frames) / static_cast< float >(soundFileInfos.samplerate) ) << " seconds\n"
						"\t" "Channels : " << soundFileInfos.channels << "\n"
						"\t" "Format (Bits) : " << soundFileInfos.format << "\n"
						"\t" "Sections : " << soundFileInfos.sections << "\n"
						"\t" "Seekable : " << soundFileInfos.seekable << '\n';
				}

				/* 3. Read data */
				if ( isDataValid )
				{
					if ( wave.initialize(samples, channels, frequency) )
					{
						/* NOTE: We use per-frame reading because we expect multichannel sound file. */
						if ( sf_readf_short(file, wave.data().data(), soundFileInfos.frames) == soundFileInfos.frames )
						{
							isDataValid = true;
						}
					}
					else
					{
						std::cerr << "[WaveFactory::FileFormatSNDFile] readFile(), unable to allocate memory for '" << filepath << "' !\n";

						isDataValid = false;
					}
				}

				sf_close(file);

				return isDataValid;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & filepath, const Wave< int16_t > & wave) const noexcept override
			{
				if ( IO::fileExists(filepath) )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] writeFile(), the file '" << filepath << "' already exists !\n";

					return false;
				}

				SF_INFO infos;
				infos.channels = static_cast< int >(wave.channels());
				infos.samplerate = static_cast< int >(wave.frequency());
				infos.format = SF_FORMAT_PCM_16 | SF_FORMAT_WAV;

				auto * file = sf_open(filepath.string().c_str(), SFM_WRITE, &infos);

				if ( file == nullptr )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] writeFile(), unable to open file '" << filepath << "' for writing !\n";

					return false;
				}

				sf_writef_short(file, wave.data().data(), static_cast< sf_count_t >(wave.data().size()));

				sf_close(file);

				return true;
			}
	};
}