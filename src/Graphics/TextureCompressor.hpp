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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <vector>

/* Local inclusions for inheritance. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"

namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Represents a single mip level of BC7-compressed texture data.
	 */
	struct EMEN_API CompressedMipLevel
	{
		std::vector< uint8_t > data; ///< BC7 compressed blocks (16 bytes per 4x4 block).
		uint32_t width{0}; ///< Width of this mip level in pixels.
		uint32_t height{0}; ///< Height of this mip level in pixels.
	};

	/**
	 * @brief BC7 texture compression sub-service of the Renderer.
	 *
	 * Uses bc7enc_rdo for block compression. Generates CPU-side mipmaps
	 * (linear downscale) and compresses each level independently.
	 * Blocks are compressed SEQUENTIALLY on the calling thread: parallelism comes from the
	 * resource manager loading several textures at once on different workers, never from within
	 * one texture. The thread pool this used to receive was threaded through and never used.
	 *
	 * BC7 produces 16 bytes per 4x4 pixel block, giving a 4:1 compression
	 * ratio on RGBA8 textures. The compressed data is suitable for direct
	 * upload to VkImage with VK_FORMAT_BC7_UNORM_BLOCK or VK_FORMAT_BC7_SRGB_BLOCK.
	 *
	 * @note The encoder's one-time setup happens in onInitialize(), so a caller can
	 * never reach a compression method before the encoder is ready. This used to be a
	 * static initialize() the caller had to remember to invoke; forgetting it only
	 * produced a runtime error log.
	 * @extends EmEn::ServiceInterface This is a sub-service of the graphics renderer.
	 */
	class EMEN_API TextureCompressor final : public ServiceInterface
	{
		public:

			static constexpr auto ClassId{"TextureCompressorService"};

			/** @brief BC7 block size in pixels (4x4). */
			static constexpr uint32_t BlockSize = 4;

			/** @brief BC7 compressed block size in bytes (128 bits). */
			static constexpr uint32_t BlockBytes = 16;

			/**
			 * @brief Constructs the texture compressor.
			 * @param primaryServices A reference to the primary services.
			 */
			explicit TextureCompressor (PrimaryServices & primaryServices) noexcept;

			/**
			 * @brief Compresses an RGBA8 pixmap to BC7 with full mipchain.
			 * @param pixmap Source RGBA8 pixel data. Dimensions should be multiples of 4.
			 * @param maxMipLevels Maximum number of mip levels to generate (0 = full chain down to 1x1).
			 * @return Vector of compressed mip levels, empty on failure.
			 * @note Non-multiple-of-4 dimensions are padded by repeating edge pixels.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > compress (const Base::PixelFactory::Pixmap< uint8_t > & pixmap, uint32_t maxMipLevels) const noexcept;

			/**
			 * @brief Compresses a single RGBA8 pixmap to BC7 (no mipchain).
			 * @param pixmap Source RGBA8 pixel data.
			 * @return Compressed mip level, empty data on failure.
			 */
			[[nodiscard]]
			CompressedMipLevel compressSingle (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

			/**
			 * @brief Returns the compressed size in bytes for a given resolution.
			 * @note Pure arithmetic on its arguments, hence static: it holds no state.
			 * @param width Texture width in pixels.
			 * @param height Texture height in pixels.
			 * @return Size in bytes of the BC7 compressed data.
			 */
			[[nodiscard]]
			static
			uint32_t
			compressedSize (uint32_t width, uint32_t height) noexcept
			{
				const uint32_t blocksX = (width + BlockSize - 1) / BlockSize;
				const uint32_t blocksY = (height + BlockSize - 1) / BlockSize;

				return blocksX * blocksY * BlockBytes;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			PrimaryServices & m_primaryServices;
	};
}
