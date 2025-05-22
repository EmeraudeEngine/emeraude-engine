/*
 * src/Vulkan/MemoryRegion.hpp
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
#include <cstddef>
#include <sstream>
#include <string>

namespace EmEn::Vulkan
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
			MemoryRegion (const void * source, size_t bytes, size_t offset = 0) noexcept
				: m_source{source},
				m_offset{offset},
				m_bytes{bytes}
			{

			}

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

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const MemoryRegion & obj);

			const void * m_source{nullptr};
			size_t m_offset{0};
			size_t m_bytes{0};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const MemoryRegion & obj)
	{
		return out << "Region of " << obj.m_bytes << " bytes from @" << obj.m_source << " to destination offset : " << obj.m_offset;
	}

	/**
	 * @brief Stringify the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const MemoryRegion & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
