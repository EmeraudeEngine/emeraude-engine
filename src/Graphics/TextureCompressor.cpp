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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TextureCompressor.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>

/* 3rd party inclusions. */
#include "bc7enc.h"

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Libs/ThreadPool.hpp"
#include "Vulkan/Image.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs::PixelFactory;

	bool TextureCompressor::s_initialized = false;

	void
	TextureCompressor::initialize () noexcept
	{
		if ( s_initialized )
		{
			return;
		}

		bc7enc_compress_block_init();

		s_initialized = true;

		Tracer::success(ClassId, "BC7 encoder initialized.");
	}

	std::vector< CompressedMipLevel >
	TextureCompressor::compress (
		const Pixmap< uint8_t > & pixmap,
		uint32_t maxMipLevels,
		Libs::ThreadPool & threadPool
	) noexcept
	{
		if ( !s_initialized )
		{
			Tracer::error(ClassId, "BC7 encoder not initialized ! Call TextureCompressor::initialize() first.");

			return {};
		}

		if ( !pixmap.isValid() || pixmap.colorCount() != 4 )
		{
			Tracer::error(ClassId, "Invalid pixmap: must be valid RGBA8 data.");

			return {};
		}

		/* Compute the number of mip levels. */
		const auto fullMipCount = Vulkan::Image::getMIPLevels(pixmap.width(), pixmap.height());
		const auto mipCount = (maxMipLevels == 0) ? fullMipCount : std::min(maxMipLevels, fullMipCount);

		std::vector< CompressedMipLevel > result;
		result.reserve(mipCount);

		/* Compress base level. */
		result.emplace_back(compressLevel(pixmap, threadPool));

		if ( result.back().data.empty() )
		{
			Tracer::error(ClassId, "Failed to compress base mip level !");

			return {};
		}

		/* Generate and compress subsequent mip levels. */
		Pixmap< uint8_t > currentMip = pixmap;

		for ( uint32_t level = 1; level < mipCount; ++level )
		{
			currentMip = generateMip(currentMip);

			if ( !currentMip.isValid() )
			{
				TraceError{ClassId} << "Failed to generate mip level " << level << " !";

				break;
			}

			result.emplace_back(compressLevel(currentMip, threadPool));

			if ( result.back().data.empty() )
			{
				TraceError{ClassId} << "Failed to compress mip level " << level << " !";

				break;
			}
		}

		TraceSuccess{ClassId} <<
			"Compressed " << pixmap.width() << "x" << pixmap.height() <<
			" texture to BC7 (" << result.size() << " mip levels).";

		return result;
	}

	CompressedMipLevel
	TextureCompressor::compressSingle (
		const Pixmap< uint8_t > & pixmap,
		Libs::ThreadPool & threadPool
	) noexcept
	{
		if ( !s_initialized )
		{
			Tracer::error(ClassId, "BC7 encoder not initialized !");

			return {};
		}

		if ( !pixmap.isValid() || pixmap.colorCount() != 4 )
		{
			Tracer::error(ClassId, "Invalid pixmap: must be valid RGBA8 data.");

			return {};
		}

		return compressLevel(pixmap, threadPool);
	}

	Pixmap< uint8_t >
	TextureCompressor::generateMip (const Pixmap< uint8_t > & source) noexcept
	{
		const auto newWidth = std::max(source.width() / 2, static_cast< decltype(source.width()) >(1));
		const auto newHeight = std::max(source.height() / 2, static_cast< decltype(source.height()) >(1));

		return Processor< uint8_t >::resize(source, newWidth, newHeight, FilteringMode::Linear);
	}

	CompressedMipLevel
	TextureCompressor::compressLevel (
		const Pixmap< uint8_t > & pixmap,
		Libs::ThreadPool & threadPool
	) noexcept
	{
		const auto width = static_cast< uint32_t >(pixmap.width());
		const auto height = static_cast< uint32_t >(pixmap.height());
		const auto blocksX = (width + BlockSize - 1) / BlockSize;
		const auto blocksY = (height + BlockSize - 1) / BlockSize;
		const auto totalBlocks = blocksX * blocksY;
		const auto * sourcePixels = pixmap.data().data();
		const auto stride = width * 4; /* RGBA8 = 4 bytes per pixel. */

		CompressedMipLevel result;
		result.width = width;
		result.height = height;
		result.data.resize(totalBlocks * BlockBytes);

		/* Setup BC7 compression parameters. */
		bc7enc_compress_block_params params;
		bc7enc_compress_block_params_init(&params);

		/* Use uber level 1 (good balance between speed and quality).
		 * Range: 0 (fastest) to 4 (best quality). */
		bc7enc_compress_block_params_init_linear_weights(&params);
		params.m_max_partitions = BC7ENC_MAX_PARTITIONS;
		params.m_uber_level = 1;

		/* Compress all blocks in parallel. */
		threadPool.parallelFor(static_cast< size_t >(0), static_cast< size_t >(totalBlocks), [&] (size_t blockIndex) {
			const auto blockX = static_cast< uint32_t >(blockIndex % blocksX);
			const auto blockY = static_cast< uint32_t >(blockIndex / blocksX);
			const auto pixelX = blockX * BlockSize;
			const auto pixelY = blockY * BlockSize;

			/* Extract 4x4 block of RGBA pixels, handling edge padding. */
			uint8_t blockPixels[BlockSize * BlockSize * 4];

			for ( uint32_t row = 0; row < BlockSize; ++row )
			{
				for ( uint32_t col = 0; col < BlockSize; ++col )
				{
					/* Clamp to edge for textures not multiple of 4. */
					const auto srcX = std::min(pixelX + col, width - 1);
					const auto srcY = std::min(pixelY + row, height - 1);
					const auto srcOffset = (srcY * stride) + (srcX * 4);
					const auto dstOffset = (row * BlockSize + col) * 4;

					std::memcpy(&blockPixels[dstOffset], &sourcePixels[srcOffset], 4);
				}
			}

			/* Compress the block. */
			auto * outputBlock = &result.data[blockIndex * BlockBytes];

			bc7enc_compress_block(outputBlock, blockPixels, &params);
		});

		return result;
	}
}