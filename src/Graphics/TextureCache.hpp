/*
 * src/Graphics/TextureCache.hpp
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
#include <filesystem>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "Graphics/TextureCompressor.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief Disk cache for BC7-compressed textures.
	 *
	 * Stores and retrieves pre-compressed BC7 mip chains on disk to avoid
	 * re-compressing textures at every engine launch. Cache files are stored
	 * in the user cache directory under a "TextureCache" subdirectory.
	 *
	 * Invalidation is based on a SHA256 hash of the resource name combined
	 * with the source file size and modification time. If the source file
	 * changes, the cache entry is automatically invalidated.
	 *
	 * File format (.bc7cache):
	 *   [4 bytes] Magic ("BC7C")
	 *   [4 bytes] Version (1)
	 *   [4 bytes] Mip level count
	 *   For each mip level:
	 *     [4 bytes] Width
	 *     [4 bytes] Height
	 *     [4 bytes] Compressed data size
	 *     [N bytes] Compressed data
	 */
	class TextureCache final
	{
		public:

			static constexpr auto ClassId{"TextureCache"};

			/**
			 * @brief Initializes the texture cache with the base cache directory.
			 * @param baseCacheDirectory The application cache directory (from FileSystem).
			 */
			static void initialize (const std::filesystem::path & baseCacheDirectory) noexcept;

			/**
			 * @brief Tries to load compressed mip data from the disk cache.
			 * @param resourceName The texture resource name (used for cache key).
			 * @param sourceFileSize Size of the original source file in bytes.
			 * @param sourceModTime Last modification time of the source file.
			 * @return Vector of compressed mip levels, empty if cache miss.
			 */
			[[nodiscard]]
			static std::vector< CompressedMipLevel > tryLoad (
				const std::string & resourceName,
				uint64_t sourceFileSize,
				uint64_t sourceModTime
			) noexcept;

			/**
			 * @brief Stores compressed mip data to the disk cache.
			 * @param resourceName The texture resource name (used for cache key).
			 * @param sourceFileSize Size of the original source file in bytes.
			 * @param sourceModTime Last modification time of the source file.
			 * @param mipLevels The compressed mip chain to store.
			 * @return True if stored successfully.
			 */
			[[nodiscard]]
			static bool store (
				const std::string & resourceName,
				uint64_t sourceFileSize,
				uint64_t sourceModTime,
				const std::vector< CompressedMipLevel > & mipLevels
			) noexcept;

		private:

			TextureCache () = delete;

			static constexpr uint32_t Magic = 0x43374342; /* "BC7C" */
			static constexpr uint32_t Version = 1;

			/**
			 * @brief Computes the cache file path for a given resource.
			 * @param resourceName The texture resource name.
			 * @param sourceFileSize Size of the original source file.
			 * @param sourceModTime Last modification time of the source file.
			 * @return Full path to the cache file.
			 */
			[[nodiscard]]
			static std::filesystem::path cacheFilePath (
				const std::string & resourceName,
				uint64_t sourceFileSize,
				uint64_t sourceModTime
			) noexcept;

			static std::filesystem::path s_cacheDirectory;
			static bool s_initialized;
	};
}
