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

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for inheritance. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The block-compressed format of a texture, chosen by its kind (graphics § 39, D8).
	 * @note Every format stores 4 × 4 pixel blocks; the source is always an RGBA8 pixmap.
	 */
	enum class BlockFormat : uint8_t
	{
		/** @brief BC7: four channels, 16 bytes per block — colour, opacity and the packed Data texture. sRGB or linear. */
		BC7,
		/** @brief BC5: the R and G channels, 16 bytes per block — a tangent-space normal map (X, Y; the shader
		 * reconstructs Z). Linear only. */
		BC5,
		/** @brief BC4: the R channel, 8 bytes per block — an opaque grey-scale map, broadcast to R, G and B by the image
		 * view (componentMapping()). Linear only. */
		BC4
	};

	/**
	 * @brief Returns the name of a block format, for the logs.
	 * @param format The block format.
	 * @return const char *
	 */
	[[nodiscard]]
	EMEN_API
	const char * to_cstring (BlockFormat format) noexcept;

	/**
	 * @brief Represents a single mip level of block-compressed texture data.
	 */
	struct EMEN_API CompressedMipLevel
	{
		std::vector< uint8_t > data; ///< Compressed blocks (TextureCompressor::blockBytes() per 4x4 block).
		uint32_t width{0}; ///< Width of this mip level in pixels.
		uint32_t height{0}; ///< Height of this mip level in pixels.
	};

	/**
	 * @brief Block texture compression sub-service of the Renderer: BC7, BC5 and BC4 (graphics § 39, D8).
	 *
	 * Uses bc7enc_rdo for BC7 and its rgbcx encoder for BC4 / BC5 (Richard Geldreich, public domain or MIT,
	 * https://github.com/richgel999/bc7enc_rdo). Generates CPU-side mipmaps (an area-weighted box filter) and
	 * compresses each level independently.
	 * Blocks are compressed SEQUENTIALLY on the calling thread: parallelism comes from the
	 * resource manager loading several textures at once on different workers, never from within
	 * one texture. The thread pool this used to receive was threaded through and never used.
	 *
	 * BC7 and BC5 produce 16 bytes per 4x4 pixel block (4:1 on RGBA8), BC4 8 bytes (8:1). The compressed data is
	 * suitable for direct upload to the VkImage of vulkanFormat().
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

			/** @brief Block size in pixels (4x4), every format. */
			static constexpr uint32_t BlockSize = 4;

			/** @brief BC7 / BC5 compressed block size in bytes (128 bits). */
			static constexpr uint32_t BlockBytes = 16;

			/** @brief BC4 compressed block size in bytes (64 bits). */
			static constexpr uint32_t HalfBlockBytes = 8;

			/**
			 * @brief Constructs the texture compressor.
			 */
			TextureCompressor () noexcept;

			/**
			 * @brief Compresses an RGBA8 pixmap with its mip chain.
			 * @note BC5 renormalizes the (X, Y, Z) vector of every texel of every level before keeping X and Y: the
			 * shader reconstructs Z = sqrt(1 - X² - Y²), which needs a unit vector — a filtered mip is shorter, and
			 * reconstructing from it would flatten the normal with distance where the BC7 path (normalize(xyz)) keeps
			 * its direction. A null vector becomes (0, 0, 1). The chain keeps filtering the UNNORMALIZED level.
			 * @param source Source RGBA8 pixel data, or one channel for a grey-scale image (read as its RGBA expansion).
			 * Dimensions should be multiples of 4.
			 * @param maxMipLevels Maximum number of mip levels to generate (0 = full chain down to 1x1).
			 * @note Every level is the previous one reduced by NVTT's Kaiser-windowed sinc (Processor::downsample(),
			 * ReductionFilter::Kaiser), the colour of an sRGB texture in linear light (graphics § 39 § The mip filter).
			 * @param format The block format.
			 * @param sRGB Whether the texture is a colour (sRGB) one: its colour channels are filtered in linear light, the
			 * alpha always linear. Read for BC7 only (BC5 holds normals, BC4 a linear grey map).
			 * @return Vector of compressed mip levels, empty on failure.
			 * @note Non-multiple-of-4 dimensions are padded by repeating edge pixels.
			 */
			[[nodiscard]]
			std::vector< CompressedMipLevel > compress (const Base::PixelFactory::Pixmap< uint8_t > & source, uint32_t maxMipLevels, BlockFormat format, bool sRGB) const noexcept;

			/**
			 * @brief Compresses a single RGBA8 pixmap to BC7 (no mipchain).
			 * @param pixmap Source RGBA8 pixel data.
			 * @return Compressed mip level, empty data on failure.
			 */
			[[nodiscard]]
			CompressedMipLevel compressSingle (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

			/**
			 * @brief Chooses the block format of a texture (graphics § 39, D8 — owner decisions 2026-10-20).
			 * @note A normal map (an identity the texture's creator gives, TextureResource::Usage::normalMap) is BC5;
			 * any other LINEAR texture whose every pixel is grey and opaque (R = G = B, A = 255) is BC4 — lossless in
			 * meaning whatever channel a reader samples, since the view broadcasts R; everything else is BC7. An sRGB
			 * texture stays BC7: BC4 and BC5 have no sRGB format (an sRGB normal map is a creator error, logged).
			 * @param pixmap The RGBA8 pixels the texture uploads.
			 * @param sRGB Whether the texture is sRGB.
			 * @param normalMap Whether the texture is a normal map.
			 * @return BlockFormat
			 */
			[[nodiscard]]
			static BlockFormat selectFormat (const Base::PixelFactory::Pixmap< uint8_t > & pixmap, bool sRGB, bool normalMap) noexcept;

			/**
			 * @brief Returns whether every pixel of an RGBA8 pixmap is grey and opaque (R = G = B, A = 255) — a one-channel
			 * pixmap always is.
			 * @note Stops at the first pixel that is not: a colour texture costs a few pixels, a grey one a full scan.
			 * @param pixmap The pixels.
			 * @return bool False for an invalid pixmap or one of two or three channels.
			 */
			[[nodiscard]]
			static bool isOpaqueGrey (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Returns the Vulkan format of a block format.
			 * @param format The block format.
			 * @param sRGB Whether the texture is sRGB (honoured by BC7 only, see selectFormat()).
			 * @return VkFormat
			 */
			[[nodiscard]]
			static VkFormat vulkanFormat (BlockFormat format, bool sRGB) noexcept;

			/**
			 * @brief Returns the image view swizzle an image format needs so the shaders read it as RGBA.
			 * @note A one-channel format (BC4, R8 linear or sRGB) broadcasts its R to R, G and B with an opaque A: a grey
			 * map reads the same from any channel, as its RGBA8 source did. Every other format is the identity (BC5 reads B = 0 and
			 * A = 1, the shaders reconstructing Z). Decided by the IMAGE format, so a pre-encoded KTX2 BC4 gets it too.
			 * @param format The format of the image the view shows.
			 * @return VkComponentMapping
			 */
			[[nodiscard]]
			static VkComponentMapping componentMapping (VkFormat format) noexcept;

			/**
			 * @brief Returns the size in bytes of one 4 × 4 block.
			 * @param format The block format.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static
			constexpr
			uint32_t
			blockBytes (BlockFormat format) noexcept
			{
				return format == BlockFormat::BC4 ? HalfBlockBytes : BlockBytes;
			}


		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

	};
}
