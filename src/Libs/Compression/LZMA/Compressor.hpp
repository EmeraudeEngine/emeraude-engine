/*
 * src/Libs/Compression/LZMA/Compressor.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <sstream>
#include <string>

/* Third-party inclusions. */
#include "lzma.h"

namespace EmEn::Libs::Compression::LZMA
{
	/**
	 * @brief The LZMA compressor with multithread capabilities.
	 */
	class Compressor final
	{
		public:

			/**
			 * @brief Constructs a LZMA compressor.
			 * @param maxThreads The max number of threads desired. 0 means single thread. Default single thread.
			 * @param preset The LZMA preset (0-9). Default LZMA default (6).
			 */
			explicit Compressor (uint32_t maxThreads = 0, uint32_t preset = LZMA_PRESET_DEFAULT) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Compressor (const Compressor & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Compressor (Compressor && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Compressor &
			 */
			Compressor & operator= (const Compressor & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Compressor &
			 */
			Compressor & operator= (Compressor && copy) noexcept = delete;

			/**
			 * @brief Destructs the LZMA compressor.
			 */
			~Compressor ();

			/**
			 * @brief Performs a compression using an STL stream.
			 * @param input A writable reference to a input stream.
			 * @param output A writable reference to a output stream.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			compressStream (std::stringstream & input, std::stringstream & output) noexcept
			{
				return this->compress(input, output);
			}

			/**
			 * @brief Performs a compression using an STL string.
			 * @param input A reference to string.
			 * @param output A writable reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool compressString (const std::string & input, std::string & output) noexcept;

		private:

			/**
			 * @brief Initializes the LZMA compressor in single thread mode.
			 * @param preset The LZMA preset.
			 * @return bool
			 */
			[[nodiscard]]
			bool init (uint32_t preset) noexcept;

			/**
			 * @brief Initializes the LZMA compressor in multithreads mode.
			 * @param maxThreads The max number of threads.
			 * @param preset The LZMA preset.
			 * @return bool
			 */
			[[nodiscard]]
			bool initMT (uint32_t maxThreads, uint32_t preset) noexcept;

			/**
			 * @brief Internal compression method following the initialization.
			 * @param input A writable reference to a stream.
			 * @param output A writable reference to a stream.
			 * @return bool
			 */
			[[nodiscard]]
			bool compress (std::stringstream & input, std::stringstream & output) noexcept;

			lzma_stream m_stream LZMA_STREAM_INIT;
			bool m_ready{false};
	};
}
