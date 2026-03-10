/*
 * src/Libs/WaveFactory/FileFormatJSON.hpp
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
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "SFXScript.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Class for reading procedural audio definitions from JSON data.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @extends EmEn::Libs::WaveFactory::FileFormatInterface The base IO class.
	 * @note Uses SFXScript to parse JSON and generate audio.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatJSON final : public FileFormatInterface< precision_t >
	{
		public:

			FileFormatJSON () noexcept = default;

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Wave< precision_t > & wave, const ReadOptions & options) noexcept override
			{
				if ( !stream.isOpen() )
				{
					std::cerr << "[WaveFactory::FileFormatJSON] readStream(), stream is not open !\n";

					return false;
				}

				/* Read the entire stream content into a string. */
				const auto dataSize = stream.size();

				std::string jsonString(dataSize, '\0');

				if ( !stream.read(jsonString.data(), dataSize) )
				{
					std::cerr << "[WaveFactory::FileFormatJSON] readStream(), failed to read stream data !\n";

					return false;
				}

				SFXScript< precision_t > script{wave, options.synthesisFrequency};

				if ( !script.generateFromString(jsonString) )
				{
					std::cerr << "[WaveFactory::FileFormatJSON] readStream(), failed to generate audio from JSON data !\n";

					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & /*stream*/, const Wave< precision_t > & /*wave*/, const WriteOptions & /*options*/) const noexcept override
			{
				/* NOTE: Writing a wave back to JSON would require reverse-engineering the synthesis,
				 * which is not practical. This format is read-only. */
				std::cerr << "[WaveFactory::FileFormatJSON] writeStream() is not supported ! JSON format is read-only.\n";

				return false;
			}
	};
}