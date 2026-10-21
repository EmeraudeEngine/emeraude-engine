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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/* Local inclusions for inheritance. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureCompressor.hpp"
#include "PixelFactory/Pixmap.hpp"

namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Disk cache for block-compressed textures (BC7, BC5, BC4), sub-service of the Renderer.
	 *
	 * Stores and retrieves pre-compressed mip chains on disk to avoid re-compressing
	 * textures at every engine launch. Cache files live in a "texture-cache" sub-directory
	 * of the user cache directory.
	 *
	 * The service owns the whole compressed path through getOrCompress(): look the entry up, compress
	 * it on a miss through the TextureCompressor sub-service, store the result. A caller only
	 * asks for compressed mips and never sees the cache policy.
	 *
	 * Governed by "Core/Graphics/Texture/EnableTextureCache", default true. Disabled, the service
	 * stays up with an empty directory: every lookup misses and every texture is re-encoded, which
	 * is what you want to measure the encoding cost.
	 *
	 * @note Invalidation is keyed on a FNV-1a hash of the DECODED PIXELS plus the dimensions,
	 * the channel count, the mip levels asked and the block format (since 2026-10-20). Hashing the content is what makes it correct: the previous key mixed the
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
	 *   [4 bytes] Block format (BlockFormat, since version 4)
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
			 * @brief Returns the compressed mip chain for a pixmap, compressing it only on a cache miss.
			 * @param resourceName The texture resource name, used for logging ONLY: the key is
			 * derived from the pixel content, never from the name.
			 * @param pixmap A reference to the decoded RGBA8 source pixels.
			 * @param maxMipLevels Maximum number of mip levels (0 = full chain).
			 * @param format The block format (TextureCompressor::selectFormat()).
			 * @param sRGB Whether the texture is a colour (sRGB) one: its chain is filtered in linear light.
			 * @return Vector of compressed mip levels, empty on failure.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > getOrCompress (const std::string & resourceName, const Base::PixelFactory::Pixmap< uint8_t > & pixmap, uint32_t maxMipLevels, BlockFormat format, bool sRGB) const noexcept;

			/**
			 * @brief Erases every cache entry from disk.
			 * @note Called on startup when --clear-renderer-cache is present.
			 */
			void clearCache () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Computes the content-addressed cache key for a pixmap, the mip chain and the block format asked of it.
			 * @note The mip count is part of the key (2026-10-20): keyed on the pixels alone, an entry written with
			 * one mip level (a run with `Core/Graphics/Texture/MipMappingLevels` = 1) served every later run that asked
			 * for a full chain, and a full chain served a run that asked for one level.
			 * @note The block format is folded in for BC5 and BC4 only (D8, 2026-10-20): the same pixels may be a BC7
			 * texture and a BC5 normal map variant at once, and one shared entry would make them overwrite each other.
			 * A BC7 key is the version-3 key, so the version-4 entry REPLACES the old file instead of orphaning it.
			 * @note The colour space is folded in for a LINEAR BC7 texture only (version 6, 2026-10-21): an sRGB chain is
			 * filtered in linear light, a linear one as stored, so the same pixels make two blobs. The common case, an sRGB
			 * BC7 colour texture, keeps the version-3 key and replaces its old file.
			 * @param pixmap A reference to the decoded RGBA8 source pixels.
			 * @param maxMipLevels The mip levels asked of the compression.
			 * @param format The block format.
			 * @param sRGB Whether the texture is a colour (sRGB) one.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t cacheKey (const Base::PixelFactory::Pixmap< uint8_t > & pixmap, uint32_t maxMipLevels, BlockFormat format, bool sRGB) noexcept;

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
			 * @param format The block format the entry must hold (anything else is a miss).
			 * @return Vector of compressed mip levels, empty on a miss.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > tryLoad (size_t key, BlockFormat format) const noexcept;

			/**
			 * @brief Stores a compressed mip chain on disk.
			 * @param key The content-addressed cache key.
			 * @param format The block format of the chain.
			 * @param mipLevels A reference to the compressed mip chain.
			 * @return bool
			 */
			[[nodiscard]]
			bool store (size_t key, BlockFormat format, const std::vector< CompressedMipLevel > & mipLevels) const noexcept;

			static constexpr uint32_t Magic{0x43374342}; /* "BC7C" */
			/* ⚠️ BUMP THIS whenever the BLOCKS a given pixmap compresses to change, not only when the
			 * file layout does: the key hashes the SOURCE pixels, so a blob produced by an older
			 * compressor stays a valid hit forever otherwise. Version 2 = coverage-preserving mip
			 * chains for binary alpha masks (TextureCompressor::compress()); version 1 blobs carry
			 * box-filtered alpha and would keep far foliage thinning out. Version 3 = every mip level is an
			 * exact area-weighted box filter (Processor::downsample()); version 2 blobs were bilinear
			 * resamples whose tail lost the mean (a 23 % leaf mask read 0.001 at 1 × 4). Version 4 = the header
			 * carries the block format (BC7, BC5, BC4 — D8, 2026-10-20). Version 5 = the colour under the transparent
			 * pixels of a BC7 chain is dilated first (Processor::fillTransparentColour(), 2026-10-21). Version 6 = every
			 * level is NVTT's Kaiser of the previous one, an sRGB colour filtered in linear light (2026-10-21). */
			static constexpr uint32_t Version{6};
			static constexpr auto CacheDirectoryName{"texture-cache"};
			static constexpr auto CacheFileExtension{".bc7cache"};

			PrimaryServices & m_primaryServices;
			const TextureCompressor & m_compressor;
			std::filesystem::path m_cacheDirectory;
	};
}
