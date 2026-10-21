/*
 * src/Graphics/TextureCompressor.cpp
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

#include "TextureCompressor.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <utility>

/* Third-party inclusions. */
#include <bc7enc_rdo/bc7enc.h>
#include <bc7enc_rdo/rgbcx.h>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/MipChain.hpp"
#include "Tracer.hpp"
#include "Vulkan/Image.hpp"

namespace EmEn::Graphics
{
	using namespace Base::PixelFactory;

	namespace
	{
		/**
		 * @brief Compresses one 4 × 4 block of RGBA8 pixels.
		 * @param format The block format.
		 * @param blockPixels The 16 RGBA8 pixels, row-major.
		 * @param params The BC7 parameters (unused by BC4 / BC5).
		 * @param outputBlock Where to write the block (TextureCompressor::blockBytes(format) bytes).
		 */
		void
		compressBlock (BlockFormat format, const uint8_t * blockPixels, const bc7enc_compress_block_params & params, uint8_t * outputBlock) noexcept
		{
			switch ( format )
			{
				case BlockFormat::BC7 :
					bc7enc_compress_block(outputBlock, blockPixels, &params);
					return;

				/* NOTE: The HQ encoders: the load time is no constraint (owner, 2026-10-20), and the cache keeps the result. */
				case BlockFormat::BC5 :
					rgbcx::encode_bc5_hq(outputBlock, blockPixels, 0, 1, 4);
					return;

				case BlockFormat::BC4 :
					static_cast< void >(rgbcx::encode_bc4_hq(outputBlock, blockPixels, 4));
					return;
			}
		}

		/**
		 * @brief Compresses one mip level to blocks of a format.
		 * @note Blocks are compressed sequentially on the calling thread. Parallelism comes from
		 * the resource manager loading several textures at once on different workers, which is why
		 * this takes no thread pool: the one it used to receive was never used.
		 * @param pixmap A reference to the source RGBA8 pixel data for this mip level.
		 * @param format The block format.
		 * @return CompressedMipLevel
		 */
		[[nodiscard]]
		CompressedMipLevel
		compressLevel (const Pixmap< uint8_t > & pixmap, BlockFormat format) noexcept
		{
			constexpr auto BlockSize = TextureCompressor::BlockSize;
			const auto bytesPerBlock = TextureCompressor::blockBytes(format);

			const auto startTime = std::chrono::steady_clock::now();

			const auto width = static_cast< uint32_t >(pixmap.width());
			const auto height = static_cast< uint32_t >(pixmap.height());
			const auto blocksX = (width + BlockSize - 1) / BlockSize;
			const auto blocksY = (height + BlockSize - 1) / BlockSize;
			const auto totalBlocks = blocksX * blocksY;
			const auto & sourcePixels = pixmap.data();
			/* RGBA8, or one channel for a grey-scale image (graphics § 39, D8 step 2). */
			const auto channels = static_cast< size_t >(pixmap.colorCount());
			const auto stride = static_cast< size_t >(width) * channels;

			CompressedMipLevel result;
			result.width = width;
			result.height = height;
			result.data.resize(static_cast< size_t >(totalBlocks) * bytesPerBlock);

			/* Setup BC7 compression parameters. */
			bc7enc_compress_block_params params{};
			bc7enc_compress_block_params_init(&params);

			/* Fast quality preset: uber_level 0 (no extra refinement passes),
			 * 16 partitions (vs 64 max). Good quality with ~5x faster compression.
			 * The disk cache will make this a one-time cost anyway. */
			bc7enc_compress_block_params_init_linear_weights(&params);
			params.m_max_partitions = 16;
			params.m_uber_level = 0;

			for ( size_t blockIndex = 0; blockIndex < totalBlocks; ++blockIndex )
			{
				const auto blockX = static_cast< uint32_t >(blockIndex % blocksX);
				const auto blockY = static_cast< uint32_t >(blockIndex / blocksX);
				const auto pixelX = blockX * BlockSize;
				const auto pixelY = blockY * BlockSize;

				/* Extract 4x4 block of RGBA pixels, handling edge padding. */
				std::array< uint8_t, static_cast< size_t >(BlockSize) * BlockSize * 4 > blockPixels{};

				for ( uint32_t row = 0; row < BlockSize; ++row )
				{
					for ( uint32_t col = 0; col < BlockSize; ++col )
					{
						/* Clamp to edge for textures not multiple of 4. */
						const auto srcX = std::min(pixelX + col, width - 1);
						const auto srcY = std::min(pixelY + row, height - 1);
						/* NOTE: In size_t: srcY * stride wrapped in 32 bits from a 32k x 32k source on (an out-of-bounds read). */
						const auto srcOffset = (static_cast< size_t >(srcY) * stride) + (static_cast< size_t >(srcX) * channels);
						const auto destination = std::span{blockPixels}.subspan(((static_cast< size_t >(row) * BlockSize) + col) * 4, 4);
						const auto source = std::span{sourcePixels}.subspan(srcOffset, channels);

						if ( channels == 4 )
						{
							std::ranges::copy(source, destination.begin());
						}
						else
						{
							/* A one-channel (grey-scale) source: the block reads it as its RGBA expansion (R = G = B, A = 1). */
							std::ranges::fill(destination.first(3), source.front());
							destination.back() = 255;
						}
					}
				}

				compressBlock(format, blockPixels.data(), params, &result.data[blockIndex * bytesPerBlock]);
			}

			const auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - startTime).count();

			TraceDebug{TextureCompressor::ClassId} << "Compressed " << width << "x" << height << " mip level to " << to_cstring(format) << " (" << totalBlocks << " blocks) in " << elapsed << " ms.";

			return result;
		}
	}

	const char *
	to_cstring (BlockFormat format) noexcept
	{
		switch ( format )
		{
			case BlockFormat::BC7 :
				return "BC7";

			case BlockFormat::BC5 :
				return "BC5";

			case BlockFormat::BC4 :
				return "BC4";
		}

		return "Unknown";
	}

	TextureCompressor::TextureCompressor () noexcept
		: ServiceInterface{ClassId}
	{

	}

	bool
	TextureCompressor::onInitialize () noexcept
	{
		/* NOTE: Both set up global tables once, before any worker compresses (rgbcx::init() is not thread-safe, its
		 * encoders are). */
		bc7enc_compress_block_init();
		rgbcx::init();

		Tracer::success(ClassId, "BC7 and BC4 / BC5 encoders initialized.");

		return true;
	}

	bool
	TextureCompressor::onTerminate () noexcept
	{
		return true;
	}

	std::vector< CompressedMipLevel >
	TextureCompressor::compress (const Pixmap< uint8_t > & source, uint32_t maxMipLevels, BlockFormat format, bool sRGB) const noexcept
	{
		if ( !source.isValid() || (source.colorCount() != 4 && source.colorCount() != 1) )
		{
			Tracer::error(ClassId, "Invalid pixmap: must be valid RGBA8 or one-channel (grey-scale) data.");

			return {};
		}

		std::vector< CompressedMipLevel > result;
		result.reserve(maxMipLevels == 0 ? Vulkan::Image::getMIPLevels(source.width(), source.height()) : maxMipLevels);

		/* NOTE: The format tells the kind (selectFormat()): BC5 holds a normal map, BC4 an opaque grey LINEAR map, BC7
		 * anything else — the only one that can be colour, and the only one with an alpha to handle. */
		const MipChain::Treatment treatment{
			.sRGB = sRGB && format == BlockFormat::BC7,
			.normalMap = format == BlockFormat::BC5,
			.alpha = format == BlockFormat::BC7
		};

		/* NOTE: A level that fails to compress ends the chain: the levels before it are kept. */
		static_cast< void >(MipChain::build(source, maxMipLevels, treatment, [&result, format] (uint32_t level, const Pixmap< uint8_t > & pixmap) {
			auto compressed = compressLevel(pixmap, format);

			if ( compressed.data.empty() )
			{
				TraceError{ClassId} << "Failed to compress mip level " << level << " !";

				return false;
			}

			result.emplace_back(std::move(compressed));

			return true;
		}));

		if ( result.empty() )
		{
			Tracer::error(ClassId, "Failed to compress base mip level !");

			return {};
		}

		/* Calculate total compressed size for logging. */
		size_t totalCompressedBytes = 0;

		for ( const auto & mip : result )
		{
			totalCompressedBytes += mip.data.size();
		}

		TraceSuccess{ClassId} <<
			"Compressed " << source.width() << "x" << source.height() <<
			" texture to " << to_cstring(format) << " (" << result.size() << " mip levels, " <<
			(totalCompressedBytes / 1024) << " KB).";

		return result;
	}

	CompressedMipLevel
	TextureCompressor::compressSingle (const Pixmap< uint8_t > & pixmap) const noexcept
	{
		if ( !pixmap.isValid() || (pixmap.colorCount() != 4 && pixmap.colorCount() != 1) )
		{
			Tracer::error(ClassId, "Invalid pixmap: must be valid RGBA8 or one-channel (grey-scale) data.");

			return {};
		}

		return compressLevel(pixmap, BlockFormat::BC7);
	}

	bool
	TextureCompressor::isOpaqueGrey (const Pixmap< uint8_t > & pixmap) noexcept
	{
		if ( !pixmap.isValid() )
		{
			return false;
		}

		/* A one-channel image is grey and opaque by construction. */
		if ( pixmap.colorCount() == 1 )
		{
			return true;
		}

		if ( pixmap.colorCount() != 4 )
		{
			return false;
		}

		const auto & bytes = pixmap.data();

		for ( size_t offset = 0; offset + 3 < bytes.size(); offset += 4 )
		{
			if ( bytes[offset] != bytes[offset + 1] || bytes[offset] != bytes[offset + 2] || bytes[offset + 3] != 255 )
			{
				return false;
			}
		}

		return true;
	}

	BlockFormat
	TextureCompressor::selectFormat (const Pixmap< uint8_t > & pixmap, bool sRGB, bool normalMap) noexcept
	{
		if ( normalMap )
		{
			if ( !sRGB )
			{
				return BlockFormat::BC5;
			}

			Tracer::error(ClassId, "A normal map is declared sRGB: BC5 has no sRGB format, it stays BC7 (the texture's creator must declare it linear) !");

			return BlockFormat::BC7;
		}

		if ( !sRGB && isOpaqueGrey(pixmap) )
		{
			return BlockFormat::BC4;
		}

		return BlockFormat::BC7;
	}

	VkFormat
	TextureCompressor::vulkanFormat (BlockFormat format, bool sRGB) noexcept
	{
		switch ( format )
		{
			case BlockFormat::BC7 :
				return sRGB ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;

			case BlockFormat::BC5 :
				return VK_FORMAT_BC5_UNORM_BLOCK;

			case BlockFormat::BC4 :
				return VK_FORMAT_BC4_UNORM_BLOCK;
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkComponentMapping
	TextureCompressor::componentMapping (VkFormat format) noexcept
	{
		if ( format == VK_FORMAT_BC4_UNORM_BLOCK || format == VK_FORMAT_R8_UNORM || format == VK_FORMAT_R8_SRGB )
		{
			return {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE};
		}

		return {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
	}
}
