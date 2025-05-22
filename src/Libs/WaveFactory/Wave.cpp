/*
 * src/Libs/WaveFactory/Wave.cpp
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

#include "Wave.hpp"

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>

/* Third-party inclusions. */
#ifdef SNDFILE_ENABLED
	#include "sndfile.h"
#endif

/* Local inclusions. */
#include "Libs/IO/IO.hpp"
#include "Types.hpp"

namespace EmEn::Libs::WaveFactory
{
#ifdef SNDFILE_ENABLED

	template<>
	bool
	Wave< int16_t >::readFile (const std::filesystem::path & filepath) noexcept
	{
		if ( !IO::fileExists(filepath) )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", file '" << filepath << "' doesn't exist !" "\n";

			return false;
		}

		SF_INFO soundFileInfos;
		soundFileInfos.frames = 0;
		soundFileInfos.samplerate = 0;
		soundFileInfos.channels = 0;
		soundFileInfos.format = 0;
		soundFileInfos.sections = 0;
		soundFileInfos.seekable = 0;

		/* 1. Check file. */
		auto * file = sf_open(filepath.string().c_str(), SFM_READ, &soundFileInfos);

		if ( file == nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to open sound file '" << filepath << "' !" "\n";

			return false;
		}

		auto isDataValid = true;

		/* 2. Read information. */
		const auto samples = static_cast< size_t >(soundFileInfos.frames);
		auto channels = toChannels(soundFileInfos.channels);
		const auto frequency = toFrequency(soundFileInfos.samplerate);

		if ( channels == Channels::Invalid )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid channels !" "\n";

			isDataValid = false;
		}

		if ( frequency == Frequency::Invalid )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid frequency !" "\n";

			isDataValid = false;
		}

		if constexpr ( WaveFactoryDebugEnabled )
		{
			std::cout <<
				"[libsndfile_DEBUG] File loaded." "\n"
				"\t" "Frames (Samples) : " << soundFileInfos.frames << "\n"
				"\t" "Sample rates (Frequency) : " << soundFileInfos.samplerate << " Hz" "\n"
				"\t" "Duration : " << ( static_cast< float >(soundFileInfos.frames) / static_cast< float >(soundFileInfos.samplerate) ) << " seconds" "\n"
				"\t" "Channels : " << soundFileInfos.channels << "\n"
				"\t" "Format (Bits) : " << soundFileInfos.format << "\n"
				"\t" "Sections : " << soundFileInfos.sections << "\n"
				"\t" "Seekable : " << soundFileInfos.seekable << '\n';
		}

		/* 3. Read data */
		if ( isDataValid )
		{
			if ( this->initialize(samples, channels, frequency) )
			{
				/* NOTE: We use per-frame reading because we expect multichannel sound file. */
				if ( sf_readf_short(file, m_data.data(), soundFileInfos.frames) == soundFileInfos.frames )
				{
					isDataValid = true;
				}
			}
			else
			{
				std::cerr << __PRETTY_FUNCTION__ << ", unable to allocate memory for " << filepath << "\n";

				isDataValid = false;
			}
		}

		sf_close(file);

		return isDataValid;
	}

	template<>
	bool
	Wave< int16_t >::writeFile (const std::filesystem::path & filepath) noexcept
	{
		SF_INFO infos;
		infos.channels = static_cast< int >(m_channels);
		infos.samplerate = static_cast< int >(m_frequency);
		infos.format = SF_FORMAT_PCM_16 | SF_FORMAT_WAV;

		auto * file = sf_open(filepath.string().c_str(), SFM_WRITE, &infos);

		if ( file == nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", unable to open file " << filepath << " for writing !" "\n";

			return false;
		}

		sf_writef_short(file, m_data.data(), static_cast< sf_count_t >(m_data.size()));

		sf_close(file);

		return true;
	}
#endif
}
