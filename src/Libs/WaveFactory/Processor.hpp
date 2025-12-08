/*
 * src/Libs/WaveFactory/Processor.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "Types.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief The Processor class. Used to perform transformations on existing waves.
	 * @note The internal wave format uses floating point number (float) for precision during processing.
	 */
	class Processor final
	{
		public:

			/**
			 * @brief Constructs a default wave processor.
			 */
			Processor () noexcept = default;

			/**
			 * @brief Constructs a wave processor with an existing wave.
			 * @tparam precision_t The precision of the input wave. Default int16_t.
			 * @param wave A reference to the input wave.
			 */
			template< typename precision_t = int16_t >
			explicit
			Processor (const Wave< precision_t > & wave) noexcept
			requires (std::is_arithmetic_v< precision_t >)
			{
				this->fromWave(wave);
			}

			/**
			 * @brief Destructs the wave processor.
			 */
			~Processor () noexcept = default;

			/**
			 * @brief Deleted copy constructor.
			 */
			Processor (const Processor & copy) = delete;

			/**
			 * @brief Deleted move constructor.
			 */
			Processor (Processor && copy) = delete;

			/**
			 * @brief Deleted assignment operator.
			 */
			Processor & operator= (const Processor & other) = delete;

			/**
			 * @brief Deleted assignment operator.
			 */
			Processor & operator= (Processor && other) = delete;

			/**
			 * @brief Loads a wave into the processor for transformation.
			 * @tparam precision_t The precision of the input wave. Default int16_t.
			 * @param wave A reference to a wave.
			 * @return bool
			 */
			template< typename precision_t = int16_t >
			bool
			fromWave (const Wave< precision_t > & wave) noexcept
			requires (std::is_arithmetic_v< precision_t >)
			{
				if ( !wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", input wave is invalid !" "\n";

					return false;
				}

				m_wave = dataConversion< precision_t, float >(wave);

				return m_wave.isValid();
			}

			/**
			 * @brief Exports the processed wave to the desired format.
			 * @tparam precision_t The precision of the output wave. Default int16_t.
			 * @param wave A reference to the output wave.
			 * @return bool
			 */
			template< typename precision_t = int16_t >
			bool
			toWave (Wave< precision_t > & wave) noexcept
			requires (std::is_arithmetic_v< precision_t >)
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", no processed wave available !" "\n";

					return false;
				}

				wave = dataConversion< float, precision_t >(m_wave);

				return wave.isValid();
			}

			/**
			 * @brief Converts the wave from multichannel to mono channel.
			 * @return bool
			 */
			bool mixDown () noexcept;

			/**
			 * @brief Resamples a wave to a new frequency.
			 * @param frequency The new frequency of the wave.
			 * @return bool
			 */
			bool resample (Frequency frequency) noexcept;

			/**
			 * @brief Returns whether the processor has valid wave data.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_wave.isValid();
			}

			/**
			 * @brief Returns the internal wave for read access.
			 * @return const Wave< float > &
			 */
			[[nodiscard]]
			const Wave< float > &
			wave () const noexcept
			{
				return m_wave;
			}

			/* ============================================
			 * Structural transformations
			 * ============================================ */

			/**
			 * @brief Removes silence from the beginning and end of the wave.
			 * @param thresholdDb Silence threshold in dB (default -60 dB).
			 * @return bool
			 */
			bool trim (float thresholdDb = -60.0F) noexcept;

			/**
			 * @brief Extracts a portion of the wave.
			 * @param startSample Starting sample index.
			 * @param endSample Ending sample index (0 = end of wave).
			 * @return bool
			 */
			bool crop (size_t startSample, size_t endSample = 0) noexcept;

			/**
			 * @brief Adds silence before and/or after the wave.
			 * @param samplesBefore Number of silence samples to add before.
			 * @param samplesAfter Number of silence samples to add after.
			 * @return bool
			 */
			bool pad (size_t samplesBefore, size_t samplesAfter = 0) noexcept;

			/**
			 * @brief Concatenates another wave to this one.
			 * @note Both waves must have the same frequency and channel count.
			 * @param other The wave to append.
			 * @return bool
			 */
			bool concat (const Wave< float > & other) noexcept;

			/**
			 * @brief Splits the wave at a given position.
			 * @param position Sample index where to split.
			 * @param secondPart Output wave receiving the second part.
			 * @return bool
			 */
			bool split (size_t position, Wave< float > & secondPart) noexcept;

			/* ============================================
			 * Channel conversions
			 * ============================================ */

			/**
			 * @brief Converts mono wave to stereo by duplicating the channel.
			 * @return bool
			 */
			bool toStereo () noexcept;

			/**
			 * @brief Extracts a specific channel from a multichannel wave.
			 * @param channelIndex The channel index to extract (0-based).
			 * @return bool
			 */
			bool extractChannel (size_t channelIndex) noexcept;

			/**
			 * @brief Swaps left and right channels in a stereo wave.
			 * @return bool
			 */
			bool swapChannels () noexcept;

			/* ============================================
			 * Analysis functions
			 * ============================================ */

			/**
			 * @brief Returns the peak level in dB.
			 * @return float Peak level in dB (0 dB = full scale).
			 */
			[[nodiscard]]
			float getPeakLevel () const noexcept;

			/**
			 * @brief Returns the RMS (Root Mean Square) level in dB.
			 * @return float RMS level in dB.
			 */
			[[nodiscard]]
			float getRMSLevel () const noexcept;

			/**
			 * @brief Returns the duration of the wave in seconds.
			 * @return float Duration in seconds.
			 */
			[[nodiscard]]
			float getDuration () const noexcept;

			/**
			 * @brief Detects silence zones in the wave.
			 * @param thresholdDb Silence threshold in dB (default -60 dB).
			 * @param minDurationMs Minimum silence duration in milliseconds (default 100 ms).
			 * @return std::vector< std::pair< size_t, size_t > > Vector of (start, end) sample pairs.
			 */
			[[nodiscard]]
			std::vector< std::pair< size_t, size_t > > detectSilence (float thresholdDb = -60.0F, float minDurationMs = 100.0F) const noexcept;

			/* ============================================
			 * Quality transformations
			 * ============================================ */

			/**
			 * @brief Normalizes the wave to a target level.
			 * @param targetDb Target peak level in dB (default 0 dB = full scale).
			 * @return bool
			 */
			bool normalize (float targetDb = 0.0F) noexcept;

			/**
			 * @brief Simulates bit depth reduction (dithering optional).
			 * @note This is a simulation; the internal format remains float.
			 * @param bits Target bit depth (8, 16, 24, 32).
			 * @param dither Apply dithering to reduce quantization noise.
			 * @return bool
			 */
			bool convertBitDepth (int bits, bool dither = true) noexcept;

		private:

			Wave< float > m_wave{};
	};
}
