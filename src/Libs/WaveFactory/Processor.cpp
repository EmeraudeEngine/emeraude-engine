/*
 * src/Libs/WaveFactory/Processor.cpp
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

#include "Processor.hpp"

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <utility>
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
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

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

		/* Start the copy of the mix down of the wave. */
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
		data.data_in = m_wave.samplePointer(0);
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

		/* Resize to actual generated frames (may differ slightly from estimate). */
		temporaryBuffer.resize(static_cast< size_t >(data.output_frames_gen) * static_cast< size_t >(m_wave.channels()));

		/* Swap data. */
		m_wave.m_data.swap(temporaryBuffer);
		m_wave.m_frequency = frequency;

		return true;
	}

	/* ============================================
	 * Structural transformations
	 * ============================================ */

	bool
	Processor::trim (float thresholdDb) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		const auto thresholdLinear = std::pow(10.0F, thresholdDb / 20.0F);
		const auto channelCount = static_cast< size_t >(m_wave.channels());
		const auto sampleCount = m_wave.sampleCount();
		auto & waveData = m_wave.m_data;

		/* Find first non-silent sample. */
		size_t startSample = 0;

		for ( size_t sample = 0; sample < sampleCount; ++sample )
		{
			bool isSilent = true;

			for ( size_t channel = 0; channel < channelCount; ++channel )
			{
				if ( std::abs(waveData[sample * channelCount + channel]) > thresholdLinear )
				{
					isSilent = false;

					break;
				}
			}

			if ( !isSilent )
			{
				startSample = sample;

				break;
			}
		}

		/* Find last non-silent sample. */
		size_t endSample = sampleCount;

		for ( size_t sample = sampleCount; sample > startSample; --sample )
		{
			bool isSilent = true;

			for ( size_t channel = 0; channel < channelCount; ++channel )
			{
				if ( std::abs(waveData[(sample - 1) * channelCount + channel]) > thresholdLinear )
				{
					isSilent = false;

					break;
				}
			}

			if ( !isSilent )
			{
				endSample = sample;

				break;
			}
		}

		/* Nothing to trim. */
		if ( startSample == 0 && endSample == sampleCount )
		{
			return true;
		}

		/* Extract the non-silent portion. */
		return this->crop(startSample, endSample);
	}

	bool
	Processor::crop (size_t startSample, size_t endSample) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		const auto channelCount = static_cast< size_t >(m_wave.channels());
		const auto sampleCount = m_wave.sampleCount();

		/* Handle default end value. */
		if ( endSample == 0 || endSample > sampleCount )
		{
			endSample = sampleCount;
		}

		if ( startSample >= endSample )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid crop range !" "\n";

			return false;
		}

		const auto newSampleCount = endSample - startSample;

		std::vector< float > temporaryBuffer(newSampleCount * channelCount);

		std::copy(
			m_wave.m_data.begin() + static_cast< ptrdiff_t >(startSample * channelCount),
			m_wave.m_data.begin() + static_cast< ptrdiff_t >(endSample * channelCount),
			temporaryBuffer.begin()
		);

		m_wave.m_data.swap(temporaryBuffer);

		return true;
	}

	bool
	Processor::pad (size_t samplesBefore, size_t samplesAfter) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		if ( samplesBefore == 0 && samplesAfter == 0 )
		{
			return true;
		}

		const auto channelCount = static_cast< size_t >(m_wave.channels());
		const auto currentSize = m_wave.m_data.size();
		const auto newSize = currentSize + (samplesBefore + samplesAfter) * channelCount;

		std::vector< float > temporaryBuffer(newSize, 0.0F);

		/* Copy original data with offset. */
		std::copy(
			m_wave.m_data.begin(),
			m_wave.m_data.end(),
			temporaryBuffer.begin() + static_cast< ptrdiff_t >(samplesBefore * channelCount)
		);

		m_wave.m_data.swap(temporaryBuffer);

		return true;
	}

	bool
	Processor::concat (const Wave< float > & other) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		if ( !other.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", other wave is invalid !" "\n";

			return false;
		}

		if ( m_wave.channels() != other.channels() || m_wave.frequency() != other.frequency() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", waves must have same channels and frequency !" "\n";

			return false;
		}

		/* Append the other wave's data. */
		m_wave.m_data.reserve(m_wave.m_data.size() + other.m_data.size());

		/* NOTE: GCC 14 generates a false positive -Wstringop-overflow warning here.
		 * The code is correct: isValid() checks guarantee non-null data, and reserve()
		 * ensures sufficient capacity. This is a known GCC analyzer limitation. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
		m_wave.m_data.insert(m_wave.m_data.end(), other.m_data.begin(), other.m_data.end());
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

		return true;
	}

	bool
	Processor::split (size_t position, Wave< float > & secondPart) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		const auto channelCount = static_cast< size_t >(m_wave.channels());
		const auto sampleCount = m_wave.sampleCount();

		if ( position >= sampleCount )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", split position out of range !" "\n";

			return false;
		}

		const auto splitIndex = position * channelCount;

		/* Create second part. */
		secondPart.m_channels = m_wave.m_channels;
		secondPart.m_frequency = m_wave.m_frequency;
		secondPart.m_data.assign(m_wave.m_data.begin() + static_cast< ptrdiff_t >(splitIndex), m_wave.m_data.end());

		/* Truncate first part. */
		m_wave.m_data.resize(splitIndex);

		return true;
	}

	/* ============================================
	 * Channel conversions
	 * ============================================ */

	bool
	Processor::toStereo () noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		if ( m_wave.channels() != Channels::Mono )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", wave is not mono !" "\n";

			return false;
		}

		const auto sampleCount = m_wave.sampleCount();

		std::vector< float > temporaryBuffer(sampleCount * 2);

		for ( size_t sample = 0; sample < sampleCount; ++sample )
		{
			temporaryBuffer[sample * 2] = m_wave.m_data[sample];
			temporaryBuffer[sample * 2 + 1] = m_wave.m_data[sample];
		}

		m_wave.m_data.swap(temporaryBuffer);
		m_wave.m_channels = Channels::Stereo;

		return true;
	}

	bool
	Processor::extractChannel (size_t channelIndex) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		const auto channelCount = static_cast< size_t >(m_wave.channels());

		if ( channelIndex >= channelCount )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", channel index out of range !" "\n";

			return false;
		}

		const auto sampleCount = m_wave.sampleCount();

		std::vector< float > temporaryBuffer(sampleCount);

		for ( size_t sample = 0; sample < sampleCount; ++sample )
		{
			temporaryBuffer[sample] = m_wave.m_data[sample * channelCount + channelIndex];
		}

		m_wave.m_data.swap(temporaryBuffer);
		m_wave.m_channels = Channels::Mono;

		return true;
	}

	bool
	Processor::swapChannels () noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		if ( m_wave.channels() != Channels::Stereo )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", wave is not stereo !" "\n";

			return false;
		}

		const auto sampleCount = m_wave.sampleCount();

		for ( size_t sample = 0; sample < sampleCount; ++sample )
		{
			std::swap(m_wave.m_data[sample * 2], m_wave.m_data[sample * 2 + 1]);
		}

		return true;
	}

	/* ============================================
	 * Analysis functions
	 * ============================================ */

	float
	Processor::getPeakLevel () const noexcept
	{
		if ( !m_wave.isValid() )
		{
			return SilenceDB;
		}

		float peakValue = 0.0F;

		for ( const auto & sample : m_wave.m_data )
		{
			peakValue = std::max(peakValue, std::abs(sample));
		}

		if ( peakValue < 1e-10F )
		{
			return SilenceDB;
		}

		return 20.0F * std::log10(peakValue);
	}

	float
	Processor::getRMSLevel () const noexcept
	{
		if ( !m_wave.isValid() || m_wave.m_data.empty() )
		{
			return SilenceDB;
		}

		float sumSquares = 0.0F;

		for ( const auto & sample : m_wave.m_data )
		{
			sumSquares += sample * sample;
		}

		const auto rms = std::sqrt(sumSquares / static_cast< float >(m_wave.m_data.size()));

		if ( rms < 1e-10F )
		{
			return SilenceDB;
		}

		return 20.0F * std::log10(rms);
	}

	float
	Processor::getDuration () const noexcept
	{
		if ( !m_wave.isValid() )
		{
			return 0.0F;
		}

		return static_cast< float >(m_wave.sampleCount()) / static_cast< float >(m_wave.frequency());
	}

	std::vector< std::pair< size_t, size_t > >
	Processor::detectSilence (float thresholdDb, float minDurationMs) const noexcept
	{
		std::vector< std::pair< size_t, size_t > > silenceZones;

		if ( !m_wave.isValid() )
		{
			return silenceZones;
		}

		const auto thresholdLinear = std::pow(10.0F, thresholdDb / 20.0F);
		const auto channelCount = static_cast< size_t >(m_wave.channels());
		const auto sampleCount = m_wave.sampleCount();
		const auto sampleRate = static_cast< float >(m_wave.frequency());
		const auto minSamples = static_cast< size_t >(minDurationMs * sampleRate / 1000.0F);

		bool inSilence = false;
		size_t silenceStart = 0;

		for ( size_t sample = 0; sample < sampleCount; ++sample )
		{
			/* Check if all channels are below threshold. */
			bool isSilent = true;

			for ( size_t channel = 0; channel < channelCount; ++channel )
			{
				if ( std::abs(m_wave.m_data[sample * channelCount + channel]) > thresholdLinear )
				{
					isSilent = false;

					break;
				}
			}

			if ( isSilent && !inSilence )
			{
				/* Start of silence zone. */
				inSilence = true;
				silenceStart = sample;
			}
			else if ( !isSilent && inSilence )
			{
				/* End of silence zone. */
				inSilence = false;

				if ( sample - silenceStart >= minSamples )
				{
					silenceZones.emplace_back(silenceStart, sample);
				}
			}
		}

		/* Check if wave ends in silence. */
		if ( inSilence && sampleCount - silenceStart >= minSamples )
		{
			silenceZones.emplace_back(silenceStart, sampleCount);
		}

		return silenceZones;
	}

	/* ============================================
	 * Quality transformations
	 * ============================================ */

	bool
	Processor::normalize (float targetDb) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		/* Find peak level. */
		float peakValue = 0.0F;

		for ( const auto & sample : m_wave.m_data )
		{
			peakValue = std::max(peakValue, std::abs(sample));
		}

		if ( peakValue < 1e-10F )
		{
			/* Wave is silent, nothing to normalize. */
			return true;
		}

		/* Calculate gain to reach target level. */
		const auto targetLinear = std::pow(10.0F, targetDb / 20.0F);
		const auto gain = targetLinear / peakValue;

		/* Apply gain. */
		for ( auto & sample : m_wave.m_data )
		{
			sample *= gain;
		}

		return true;
	}

	bool
	Processor::convertBitDepth (int bits, bool dither) noexcept
	{
		if ( !m_wave.isValid() )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", no wave to process !" "\n";

			return false;
		}

		/* Validate bit depth. */
		if ( bits != 8 && bits != 16 && bits != 24 && bits != 32 )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", invalid bit depth (must be 8, 16, 24, or 32) !" "\n";

			return false;
		}

		/* Calculate quantization levels. */
		const auto levels = static_cast< float >(1 << (bits - 1));
		const auto ditherAmount = 1.0F / levels;

		/* Setup random generator for dithering. */
		std::random_device randomDevice;
		std::mt19937 generator{randomDevice()};
		std::uniform_real_distribution< float > distribution(-ditherAmount, ditherAmount);

		for ( auto & sample : m_wave.m_data )
		{
			/* Add triangular dither if requested. */
			if ( dither )
			{
				sample += distribution(generator) + distribution(generator);
			}

			/* Quantize. */
			sample = std::round(sample * levels) / levels;

			/* Clamp to valid range. */
			sample = std::clamp(sample, -1.0F, 1.0F);
		}

		return true;
	}
}
