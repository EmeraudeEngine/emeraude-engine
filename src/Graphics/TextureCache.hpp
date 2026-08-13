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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/* Local inclusions for inheritance. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureCompressor.hpp"

namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Disk cache for BC7-compressed textures, sub-service of the Renderer.
	 *
	 * Stores and retrieves pre-compressed BC7 mip chains on disk to avoid re-compressing
	 * textures at every engine launch. Cache files live in a "texture-cache" sub-directory
	 * of the user cache directory.
	 *
	 * The service owns the whole BC7 path through getOrCompress(): look the entry up, compress
	 * it on a miss through the TextureCompressor sub-service, store the result. A caller only
	 * asks for compressed mips and never sees the cache policy.
	 *
	 * Governed by "Core/Graphics/Texture/EnableTextureCache", default true. Disabled, the service
	 * stays up with an empty directory: every lookup misses and every texture is re-encoded, which
	 * is what you want to measure the encoding cost.
	 *
	 * @note Invalidation is keyed on a FNV-1a hash of the DECODED PIXELS plus the dimensions and
	 * the channel count. Hashing the content is what makes it correct: the previous key mixed the
	 * resource name with a "file size" that was the decoded byte count and a "modification time"
	 * that was width * 1000000 + height, so it reduced to name + dimensions. Repainting a texture
	 * without resizing it served the stale BC7 blob forever.
	 * @warning Changing the key scheme does not INVALIDATE existing entries, it ORPHANS them: their
	 * filenames simply stop being produced, so they sit on disk unreachable. Whoever touches
	 * cacheKey() again must clear the directory, which --clear-renderer-cache does.
	 * @extends EmEn::ServiceInterface This is a sub-service of the graphics renderer.
	 *
	 * File format (.bc7cache):
	 *   [4 bytes] Magic ("BC7C")
	 *   [4 bytes] Version
	 *   [4 bytes] Mip level count
	 *   For each mip level:
	 *	 [4 bytes] Width
	 *	 [4 bytes] Height
	 *	 [4 bytes] Compressed data size
	 *	 [N bytes] Compressed data
	 */
	class EMEN_API TextureCache final : public ServiceInterface
	{
		public:

			static constexpr auto ClassId{"TextureCacheService"};

			/**
			 * @brief Constructs the texture cache.
			 * @param primaryServices A reference to the primary services.
			 * @param compressor A reference to the texture compressor sub-service, used on a miss.
			 */
			TextureCache (PrimaryServices & primaryServices, const TextureCompressor & compressor) noexcept;

			/**
			 * @brief Returns the BC7 mip chain for a pixmap, compressing it only on a cache miss.
			 * @param resourceName The texture resource name, used for logging ONLY: the key is
			 * derived from the pixel content, never from the name.
			 * @param pixmap A reference to the decoded RGBA8 source pixels.
			 * @param maxMipLevels Maximum number of mip levels (0 = full chain).
			 * @return Vector of compressed mip levels, empty on failure.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > getOrCompress (const std::string & resourceName, const Base::PixelFactory::Pixmap< uint8_t > & pixmap, uint32_t maxMipLevels) const noexcept;

			/**
			 * @brief Erases every cache entry from disk.
			 * @note Called on startup when --clear-renderer-cache is present.
			 * @return void
			 */
			void clearCache () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Computes the content-addressed cache key for a pixmap.
			 * @param pixmap A reference to the decoded RGBA8 source pixels.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t cacheKey (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Returns the cache file path for a key.
			 * @param key The content-addressed cache key.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path cacheFilePath (size_t key) const noexcept;

			/**
			 * @brief Tries to load a compressed mip chain from disk.
			 * @param key The content-addressed cache key.
			 * @return Vector of compressed mip levels, empty on a miss.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > tryLoad (size_t key) const noexcept;

			/**
			 * @brief Stores a compressed mip chain on disk.
			 * @param key The content-addressed cache key.
			 * @param mipLevels A reference to the compressed mip chain.
			 * @return bool
			 */
			[[nodiscard]]
			bool store (size_t key, const std::vector< CompressedMipLevel > & mipLevels) const noexcept;

			static constexpr uint32_t Magic{0x43374342}; /* "BC7C" */
			static constexpr uint32_t Version{1};
			static constexpr auto CacheDirectoryName{"texture-cache"};
			static constexpr auto CacheFileExtension{".bc7cache"};

			PrimaryServices & m_primaryServices;
			const TextureCompressor & m_compressor;
			std::filesystem::path m_cacheDirectory;
	};
}
