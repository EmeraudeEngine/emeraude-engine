/*
 * src/Libs/WaveFactory/Wave.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

/* Local inclusions. */
#include "Libs/Utility.hpp"
#include "Types.hpp"

namespace EmEn::Libs::WaveFactory
{
	/* Forward declarations. */
	template< typename precision_t > requires (std::is_arithmetic_v< precision_t >) class Synthesizer;
	class Processor;

	/**
	 * @brief The Wave class.
	 * @tparam precision_t The type of number for wave precision. Default int16_t.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class Wave final
	{
		/* NOTE: Let's be friend with the Synthesizer and Processor classes. */
		friend class Synthesizer< precision_t >;
		friend class Processor;

		/* NOTE: The conversion function is way easier if we let it be a friend too. */
		template< typename input_data_t, typename output_data_t >
		friend Wave< output_data_t > dataConversion (const Wave< input_data_t > & input) noexcept;

		public:

			/**
			 * @brief Constructs a default wave.
			 */
			Wave () noexcept = default;

			/**
			 * @brief Constructs a wave.
			 * @param samplesCount The length of the sound in samples.
			 * @param channels The number of channels.
			 * @param frequency The frequency of the sound (samples per seconds).
			 */
			Wave (size_t samplesCount, Channels channels, Frequency frequency) noexcept
				: Wave{}
			{
				this->initialize(samplesCount, channels, frequency);
			}

			/**
			 * @brief Initializes an empty wave.
			 * @param samplesCount The length of the sound in samples. It will be multiplied by the channel count.
			 * @param channels The number of channels.
			 * @param frequency The frequency of the sound (samples per seconds).
			 * @return bool
			 */
			bool
			initialize (size_t samplesCount, Channels channels, Frequency frequency) noexcept
			{
				/* Gets the real sample count. */
				auto bufferSize = samplesCount * static_cast< size_t >(channels);

				if ( bufferSize == 0UL )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", trying to allocate a zero-length audio buffer !" "\n";

					return false;
				}

				m_channels = channels;
				m_frequency = frequency;

				/* Checks the diff with a precedent buffer */
				if ( m_data.size() != bufferSize )
				{
					m_data.resize(bufferSize);
				}

				if constexpr ( WaveFactoryDebugEnabled )
				{
					/* Shows memory usage */
					auto memoryAllocated = m_data.size() * sizeof(precision_t);

					std::cout << "[DEBUG] " << __PRETTY_FUNCTION__ << ", " << ( static_cast< float >(memoryAllocated) / 1048576 ) << " Mib" "\n";
				}

				return true;
			}

			/**
			 * @brief Initializes a wave from a sample vector.
			 * @param data A reference to a vector.
			 * @param channels The number of channels.
			 * @param frequency The frequency of the sound (samples per seconds).
			 * @return bool
			 */
			bool
			initialize (const std::vector< precision_t > & data, Channels channels, Frequency frequency) noexcept
			{
				if ( data.empty() )
				{
					return false;
				}

				m_data = data;
				m_channels = channels;
				m_frequency = frequency;

				return true;
			}

			/**
			 * @brief Clears the wave data.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_data.clear();
			}

		/**
			 * @brief Returns whether there is data loaded.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return !m_data.empty();
			}

			/**
			 * @brief Returns the total data in the buffer.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			elementCount () const noexcept
			{
				return m_data.size();
			}

			/**
			 * @brief Returns the total samples of the buffer.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			sampleCount () const noexcept
			{
				if ( m_data.empty() )
				{
					return 0;
				}

				return m_data.size() / static_cast< size_t >(m_channels);
			}

			/**
			 * @brief Returns a token for the channel count.
			 * @note Cast-able as a number.
			 * @return Channels
			 */
			[[nodiscard]]
			Channels
			channels () const noexcept
			{
				return m_channels;
			}

			/**
			 * @brief Returns a token for the frequency used.
			 * @note Cast-able as a number.
			 * @return Frequency
			 */
			[[nodiscard]]
			Frequency
			frequency () const noexcept
			{
				return m_frequency;
			}

			/**
			 * @brief Returns the size of the wave in bytes (samplesCount * channelsCount * bytesCount (2 for 16bits sound)).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bytes () const noexcept
			{
				return static_cast< output_t >(m_data.size() * sizeof(precision_t));
			}

			/**
			 * @brief Returns read-only access to raw data of the wave.
			 * @return const std::vector< precision_t > &
			 */
			[[nodiscard]]
			const std::vector< precision_t > &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a write access to raw data of the wave.
			 * @return std::vector< precision_t > &
			 */
			[[nodiscard]]
			std::vector< precision_t > &
			data () noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a raw pointer to the data at specified offset.
			 * @param offset The offset in the buffer. Default 0.
			 * @return type_t *
			 */
			[[nodiscard]]
			precision_t *
			samplePointer (size_t offset = 0UL) noexcept
			{
				if ( offset > 0UL )
				{
					if ( offset >= m_data.size() )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", buffer overflow !" "\n";

						return nullptr;
					}

					return &m_data[offset];
				}

				return m_data.data();
			}

			/**
			 * @brief Returns a raw constant pointer to the data at specified offset.
			 * @param offset The offset in the buffer. Default 0.
			 * @return const type_t *
			 */
			[[nodiscard]]
			const precision_t *
			samplePointer (size_t offset) const noexcept
			{
				if ( offset > 0UL )
				{
					if ( offset >= m_data.size() )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", buffer overflow !" "\n";

						return nullptr;
					}

					return &m_data[offset];
				}

				return m_data.data();
			}

			/**
			 * @brief Returns the number of chunks needed to split down the wave by a specific size.
			 * @param chunkSize The chunk size in bytes.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			chunkCount (size_t chunkSize) const noexcept
			{
				return Utility::ceilDivision(m_data.size(), chunkSize);
			}

			/**
			 * @brief Returns a chunk of the wave.
			 * @param chunkIndex The number of the chunk.
			 * @param chunkSize The chunk size in bytes.
			 * @return Chunk
			 */
			[[nodiscard]]
			Chunk
			chunk (size_t chunkIndex, size_t chunkSize) const noexcept
			{
				const auto count = this->chunkCount(chunkSize);

				if ( chunkIndex >= count )
				{
					chunkIndex = count - 1;
				}

				Chunk output;
				output.offset = chunkIndex * chunkSize;
				output.bytes = chunkSize;

				/* Last chunk size check. */
				if ( chunkIndex == count - 1 )
				{
					output.bytes -= ((count * chunkSize) - m_data.size());
				}

				if constexpr ( WaveFactoryDebugEnabled )
				{
					std::cout << "Chunk #" << chunkIndex << " (size: " << chunkSize << ") -> offset " << output.offset << " of " << m_data.size() << "." "\n";
				}

				/* Note: sizeof = 2 */
				output.bytes *= sizeof(precision_t);

				return output;
			}

			/**
			 * @brief Returns the duration in seconds.
			 * @return float Returns 0.0 if frequency is invalid.
			 */
			[[nodiscard]]
			float
			seconds () const noexcept
			{
				const auto freq = static_cast< float >(m_frequency);

				return (freq > 0.0F) ? (static_cast< float >(this->sampleCount()) / freq) : 0.0F;
			}

			/**
			 * @brief Returns the duration in milliseconds.
			 * @return float Returns 0.0 if frequency is invalid.
			 */
			[[nodiscard]]
			float
			milliseconds () const noexcept
			{
				const auto freq = static_cast< float >(m_frequency);

				return (freq > 0.0F) ? ((static_cast< float >(this->sampleCount()) * 1000.0F) / freq) : 0.0F;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Wave & obj)
			{
				return out <<
					"Wave (wave_t) data :\n" <<
					"Samples count : " << obj.sampleCount() << "\n"
					"Channels count : " << static_cast< int >(obj.m_channels) <<"\n"
					"Frequency : " << static_cast< int >(obj.m_frequency) << "\n"
					"Wave data count : " << obj.elementCount() << "\n"
					"Wave data size : " << obj.bytes() << "\n"
					"Wave data : " << ( obj.isValid() ? "Loaded" : "Not loaded" ) << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Wave & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::vector< precision_t > m_data{};
			Channels m_channels = Channels::Invalid;
			Frequency m_frequency = Frequency::Invalid;
	};

	/**
	 * @brief Converts a wave from one data type to another.
	 * @tparam input_data_t The data type of the source wave.
	 * @tparam output_data_t The data type of the target wave.
	 * @param input A reference to input wave.
	 * @return Wave< output_data_t >
	 */
	template< typename input_data_t, typename output_data_t >
	[[nodiscard]]
	Wave< output_data_t >
	dataConversion (const Wave< input_data_t > & input) noexcept
	requires (std::is_arithmetic_v< input_data_t >, std::is_arithmetic_v< output_data_t >)
	{
		Wave< output_data_t > output{input.sampleCount(), input.channels(), input.frequency()};

		const auto & inputData = input.data();
		auto & outputData = output.data();

		for ( size_t index = 0; index < inputData.size(); index++ )
		{
			if constexpr ( std::is_floating_point_v< input_data_t > )
			{
				/* float -> float */
				if constexpr ( std::is_floating_point_v< output_data_t > )
				{
					outputData[index] = static_cast< output_data_t >(inputData[index]);
				}
				/* float -> integer */
				else
				{
					outputData[index] = static_cast< output_data_t >(std::round(inputData[index] * std::numeric_limits< output_data_t >::max()));
				}
			}
			else
			{
				/* integer -> float */
				if constexpr ( std::is_floating_point_v< output_data_t > )
				{
					outputData[index] = static_cast< output_data_t >(inputData[index]) / static_cast< output_data_t >(std::numeric_limits< input_data_t >::max());
				}
				/* integer -> integer */
				else
				{
					const auto ratio = static_cast< float >(std::numeric_limits< output_data_t >::max()) / static_cast< float >(std::numeric_limits< input_data_t >::max());

					outputData[index] = static_cast< output_data_t >(std::round(ratio * inputData[index]));
				}
			}
		}

		return output;
	}
}
