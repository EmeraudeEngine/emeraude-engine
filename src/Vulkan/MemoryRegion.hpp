/*
 * src/Vulkan/MemoryRegion.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <ostream>
#include <string>

namespace Emeraude::Vulkan
{
	/**
	 * @brief The memory region helper class.
	 */
	class MemoryRegion final
	{
		public:

			/**
			 * @brief Constructs a memory region.
			 * @param source The pointer to the source data.
			 * @param bytes The size of data in bytes.
			 * @param offset The offset to the destination. Default 0.
			 */
			MemoryRegion (const void * source, size_t bytes, size_t offset = 0) noexcept;

			/**
			 * @brief Returns the source pointer.
			 * @return const void *
			 */
			[[nodiscard]]
			const void *
			source () const noexcept
			{
				return m_source;
			}

			/**
			 * @brief Returns the size of data in bytes.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			bytes () const noexcept
			{
				return m_bytes;
			}

			/**
			 * @brief Returns the optional offset where the data must be copied/moved.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			offset () const noexcept
			{
				return m_offset;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const MemoryRegion & obj);

			/**
			 * @brief Stringify the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const MemoryRegion & obj) noexcept;

		private:

			const void * m_source{nullptr};
			size_t m_offset{0};
			size_t m_bytes{0};
	};
}
