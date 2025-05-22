/*
 * src/Libs/PixelFactory/FileFormatInterface.hpp
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
#include <array>
#include <filesystem>

/* Local inclusions for usages. */
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief File format interface for reading and writing a pixmap.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
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
			 * @brief Destructs the pixmap format.
			 */
			virtual ~FileFormatInterface () = default;

			/**
			 * @brief Returns whether the image must inverse Y axis to read and write image.
			 * @note true means origin at bottom-left (OpenGL compliant), else top-left (Vulkan, DX compliant).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			invertYAxis () const noexcept
			{
				return m_flags[InvertYAxis];
			}

			/**
			 * @brief Reads the pixmap from a file.
			 * @param filepath A reference to a filesystem path.
			 * @param pixmap A reference to the pixmap.
			 * @return bool
			 */
			virtual bool readFile (const std::filesystem::path & filepath, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept = 0;

			/**
			 * @brief Writes the pixmap to a file.
			 * @param filepath A reference to a filesystem path. Should be accessible.
			 * @param pixmap A read-only reference to the pixmap.
			 * @return bool
			 */
			virtual bool writeFile (const std::filesystem::path & filepath, const Pixmap< pixel_data_t, dimension_t > & pixmap) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a pixmap format.
			 */
			FileFormatInterface () noexcept = default;

		private:

			/* Flag names. */
			static constexpr auto InvertYAxis{0UL};

			std::array< bool, 8 > m_flags{
				false/*InvertYAxis*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
