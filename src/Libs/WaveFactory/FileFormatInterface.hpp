/*
 * src/Libs/WaveFactory/FileFormatInterface.hpp
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
#include <filesystem>
#include <type_traits>

/* Local inclusions. */
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief File format interface for reading and writing a wave.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileFormatInterface (const FileFormatInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FileFormatInterface (FileFormatInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileFormatInterface &
			 */
			FileFormatInterface & operator= (const FileFormatInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FileFormatInterface &
			 */
			FileFormatInterface & operator= (FileFormatInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the wave format.
			 */
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Reads the wave from a file.
			 * @param filepath A reference to a filesystem path.
			 * @param wave A reference to the wave.
			 * @return bool
			 */
			virtual bool readFile (const std::filesystem::path & filepath, Wave< precision_t > & wave) noexcept = 0;

			/**
			 * @brief Writes the wave to a file.
			 * @param filepath A reference to a filesystem path. Should be accessible.
			 * @param wave A read-only reference to the wave.
			 * @return bool
			 */
			virtual bool writeFile (const std::filesystem::path & filepath, const Wave< precision_t > & wave) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a wave format.
			 */
			FileFormatInterface () noexcept = default;
	};
}