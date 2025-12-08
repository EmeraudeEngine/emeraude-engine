/*
 * src/Libs/WaveFactory/FileFormatJSON.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <iostream>
#include <type_traits>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "SFXScript.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Class for reading procedural audio definitions from JSON files.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @extends EmEn::Libs::WaveFactory::FileFormatInterface The base IO class.
	 * @note Uses SFXScript to parse JSON and generate audio.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatJSON final : public FileFormatInterface< precision_t >
	{
		public:

			/**
			 * @brief Constructs a JSON format IO with default sample rate.
			 * @param frequency The sample rate to use for generation. Default 48kHz.
			 */
			explicit
			FileFormatJSON (Frequency frequency = Frequency::PCM48000Hz) noexcept
				: m_frequency{frequency}
			{

			}

			/**
			 * @brief Sets the sample rate for audio generation.
			 * @param frequency The sample rate.
			 * @return void
			 */
			void
			setFrequency (Frequency frequency) noexcept
			{
				m_frequency = frequency;
			}

			/**
			 * @brief Returns the current sample rate.
			 * @return Frequency
			 */
			[[nodiscard]]
			Frequency
			frequency () const noexcept
			{
				return m_frequency;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Wave< precision_t > & wave) noexcept override
			{
				SFXScript< precision_t > script{wave, m_frequency};

				if ( !script.generateFromFile(filepath) )
				{
					std::cerr << "[WaveFactory::FileFormatJSON] readFile(), failed to generate audio from '" << filepath << "' !\n";

					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & /*filepath*/, const Wave< precision_t > & /*wave*/) const noexcept override
			{
				/* NOTE: Writing a wave back to JSON would require reverse-engineering the synthesis,
				 * which is not practical. This format is read-only. */
				std::cerr << "[WaveFactory::FileFormatJSON] writeFile() is not supported ! JSON format is read-only.\n";

				return false;
			}

		private:

			Frequency m_frequency{Frequency::PCM48000Hz};
	};
}