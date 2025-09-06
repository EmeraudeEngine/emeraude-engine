/*
 * src/Libs/WaveFactory/Processor.cpp
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

#include "Processor.hpp"

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

/* Third-party inclusions. */
#include <samplerate.h>

/* Local inclusions. */
#include "Types.hpp"

namespace EmEn::Libs::WaveFactory
{
	bool
	Processor::mixDown () noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << "No wave to process !" "\n";

			return false;
		}

		/* Checks channel and convert it to an integer. */
		if ( m_wave.channels() == Channels::Mono )
		{
			return true;
		}

		/* Gets the right channel count in a size_t. */
		const auto channelsCount = static_cast< size_t >(m_wave.channels());

		/* Reserve a temporary buffer. */
		const auto size = m_wave.m_data.size();

		std::vector< float > temporaryBuffer(size / channelsCount);

		/* Stat the copy of the mix down of the wave. */
		size_t monoIndex = 0;

		for ( size_t index = 0; index < size; index += channelsCount )
		{
			auto value = 0.0F;

			for ( size_t channel = 0; channel < channelsCount; channel++ )
			{
				value += m_wave.m_data[index + channel];
			}

			temporaryBuffer[monoIndex] = value / static_cast< float >(channelsCount);

			monoIndex++;
		}

		/* Swap data. */
		m_wave.m_data.swap(temporaryBuffer);
		m_wave.m_channels = Channels::Mono;

		return true;
	}

	bool
	Processor::resample (Frequency frequency) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to resample !" "\n";

			return false;
		}

		if ( m_wave.frequency() == frequency )
		{
			std::cout << __PRETTY_FUNCTION__ << ", the frequency is already at " << static_cast< int >(frequency) << " Hz !" "\n";

			return true;
		}

		const auto ratio = static_cast< double >(frequency) / static_cast< double >(m_wave.frequency());

		/* Reserve a temporary buffer. */
		const auto newSampleCount = static_cast< size_t >(std::ceil(static_cast< double >(m_wave.sampleCount()) * ratio));

		std::vector< float > temporaryBuffer(newSampleCount * static_cast< size_t >(m_wave.channels()), 0.0F);

		SRC_DATA data;
		/* A pointer to the input data samples. */
		data.data_in = m_wave.data();
		/* The number of frames of data pointed to by data_in. */
		data.input_frames = static_cast< long >(m_wave.sampleCount());
		/* A pointer to the output data samples. */
		data.data_out = temporaryBuffer.data();
		/* Maximum number of frame pointers to "data_out". */
		data.output_frames = static_cast< long >(newSampleCount);
		/* Equal to output_sample_rate / input_sample_rate. */
		data.src_ratio = ratio;

		const auto error = src_simple(&data, SRC_SINC_BEST_QUALITY, static_cast< int >(m_wave.channels()));

		if ( error > 0 )
		{
			std::cerr << src_strerror(error) << "\n";

			return false;
		}

		/* Swap data. */
		m_wave.m_data.swap(temporaryBuffer);
		m_wave.m_frequency = frequency;

		return true;
	}
}
