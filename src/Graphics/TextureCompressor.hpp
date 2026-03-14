/*
 * src/Graphics/TextureCompressor.hpp
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
#include <vector>

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"

/* Forward declarations. */
namespace EmEn::Libs
{
	class ThreadPool;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Represents a single mip level of BC7-compressed texture data.
	 */
	struct CompressedMipLevel
	{
		std::vector< uint8_t > data; ///< BC7 compressed blocks (16 bytes per 4x4 block).
		uint32_t width{0}; ///< Width of this mip level in pixels.
		uint32_t height{0}; ///< Height of this mip level in pixels.
	};

	/**
	 * @brief Utility class for compressing RGBA pixel data to BC7 format.
	 *
	 * Uses bc7enc_rdo for block compression. Generates CPU-side mipmaps
	 * (linear downscale) and compresses each level independently.
	 * Compression is parallelized across blocks using the engine ThreadPool.
	 *
	 * BC7 produces 16 bytes per 4x4 pixel block, giving a 4:1 compression
	 * ratio on RGBA8 textures. The compressed data is suitable for direct
	 * upload to VkImage with VK_FORMAT_BC7_UNORM_BLOCK or VK_FORMAT_BC7_SRGB_BLOCK.
	 *
	 * @note This class is stateless. Call initialize() once at engine startup.
	 */
	class TextureCompressor final
	{
		public:

			static constexpr auto ClassId{"TextureCompressor"};

			/** @brief BC7 block size in pixels (4x4). */
			static constexpr uint32_t BlockSize = 4;

			/** @brief BC7 compressed block size in bytes (128 bits). */
			static constexpr uint32_t BlockBytes = 16;

			/**
			 * @brief One-time initialization of the BC7 encoder.
			 * @note Must be called before any compression. Safe to call multiple times.
			 */
			static void initialize () noexcept;

			/**
			 * @brief Compresses an RGBA8 pixmap to BC7 with full mipchain.
			 * @param pixmap Source RGBA8 pixel data. Dimensions should be multiples of 4.
			 * @param maxMipLevels Maximum number of mip levels to generate (0 = full chain down to 1x1).
			 * @param threadPool Thread pool for parallel block compression.
			 * @return Vector of compressed mip levels, empty on failure.
			 * @note Non-multiple-of-4 dimensions are padded by repeating edge pixels.
			 */
			[[nodiscard]]
			static std::vector< CompressedMipLevel > compress (
				const Libs::PixelFactory::Pixmap< uint8_t > & pixmap,
				uint32_t maxMipLevels,
				Libs::ThreadPool & threadPool
			) noexcept;

			/**
			 * @brief Compresses a single RGBA8 pixmap to BC7 (no mipchain).
			 * @param pixmap Source RGBA8 pixel data.
			 * @param threadPool Thread pool for parallel block compression.
			 * @return Compressed mip level, empty data on failure.
			 */
			[[nodiscard]]
			static CompressedMipLevel compressSingle (
				const Libs::PixelFactory::Pixmap< uint8_t > & pixmap,
				Libs::ThreadPool & threadPool
			) noexcept;

			/**
			 * @brief Returns the compressed size in bytes for a given resolution.
			 * @param width Texture width in pixels.
			 * @param height Texture height in pixels.
			 * @return Size in bytes of the BC7 compressed data.
			 */
			[[nodiscard]]
			static uint32_t
			compressedSize (uint32_t width, uint32_t height) noexcept
			{
				const uint32_t blocksX = (width + BlockSize - 1) / BlockSize;
				const uint32_t blocksY = (height + BlockSize - 1) / BlockSize;

				return blocksX * blocksY * BlockBytes;
			}

		private:

			TextureCompressor () = delete;

			/**
			 * @brief Generates a half-resolution mip level using box filter.
			 * @param source Source pixmap.
			 * @return Downscaled pixmap (half width, half height, minimum 1x1).
			 */
			[[nodiscard]]
			static Libs::PixelFactory::Pixmap< uint8_t > generateMip (
				const Libs::PixelFactory::Pixmap< uint8_t > & source
			) noexcept;

			/**
			 * @brief Compresses a single mip level to BC7 blocks.
			 * @param pixmap Source RGBA8 pixel data for this mip level.
			 * @param threadPool Thread pool for parallel block compression.
			 * @return Compressed data for this mip level.
			 */
			[[nodiscard]]
			static CompressedMipLevel compressLevel (
				const Libs::PixelFactory::Pixmap< uint8_t > & pixmap,
				Libs::ThreadPool & threadPool
			) noexcept;

			static bool s_initialized;
	};
}