/*
 * src/CachedDownloadItem.hpp
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
#include <string>

namespace Emeraude
{
	/**
	 * @brief The cached download item class.
	 */
	class CachedDownloadItem final
	{
		public:

			/**
			 * @brief Constructs a cached download item.
			 * @param cacheId An integer for the cache Id.
			 * @param originalFilename A reference to a string [std::move].
			 * @param filesize The size in bytes of the file.
			 */
			CachedDownloadItem (size_t cacheId, std::string originalFilename, size_t filesize) noexcept;

			/**
			 * @brief Returns the cache Id to retrieve it from the cache directory.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			cacheId () const noexcept
			{
				return m_cacheId;
			}

			/**
			 * @brief Returns the original filename.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			originalFilename () const noexcept
			{
				return m_originalFilename;
			}

			/**
			 * @brief Returns the file size in bytes.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			filesize () const noexcept
			{
				return m_filesize;
			}

		private:

			size_t m_cacheId;
			std::string m_originalFilename;
			size_t m_filesize;
	};
}
