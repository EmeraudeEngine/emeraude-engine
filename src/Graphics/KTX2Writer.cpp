/*
 * src/Graphics/KTX2Writer.cpp
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

#include "KTX2Writer.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include <ktx.h>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/TextureCompressor.hpp"
#include "IO/IO.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	namespace
	{
		/* The base-class view of a KTX2 texture, for the libktx calls that take a ktxTexture *. ktxTexture2 begins with
		 * the ktxTexture members (libktx's C "inheritance"); this is what libktx's own ktxTexture() macro does, with a
		 * C-style cast (the same helper as KTX2Decoder's). */
		ktxTexture *
		asBaseTexture (ktxTexture2 * texture) noexcept
		{
			return reinterpret_cast< ktxTexture * >(texture);
		}

		/** @brief Destroys a libktx texture (the RAII owner of the container being built). */
		struct TextureDeleter final
		{
			void
			operator() (ktxTexture2 * texture) const noexcept
			{
				ktxTexture_Destroy(asBaseTexture(texture));
			}
		};

		/** @brief Frees the memory libktx allocated for a written container (malloc'ed by libktx). */
		struct BufferDeleter final
		{
			void
			operator() (ktx_uint8_t * bytes) const noexcept
			{
				/* NOTE: libktx's ktxTexture_WriteToMemory() hands out a malloc'ed buffer the caller frees. */
				std::free(bytes);
			}
		};

		/**
		 * @brief Returns the bytes of one 4 × 4 block of a format, 0 for a format the writer refuses.
		 * @param format The Vulkan format.
		 * @return uint32_t
		 */
		[[nodiscard]]
		uint32_t
		blockBytesOf (VkFormat format) noexcept
		{
			switch ( format )
			{
				case VK_FORMAT_BC7_UNORM_BLOCK :
				case VK_FORMAT_BC7_SRGB_BLOCK :
					return TextureCompressor::blockBytes(BlockFormat::BC7);

				case VK_FORMAT_BC5_UNORM_BLOCK :
					return TextureCompressor::blockBytes(BlockFormat::BC5);

				case VK_FORMAT_BC4_UNORM_BLOCK :
					return TextureCompressor::blockBytes(BlockFormat::BC4);

				default :
					return 0;
			}
		}

		/**
		 * @brief Checks a mip chain against its format: each level's size and dimensions.
		 * @param levels The levels, level 0 first.
		 * @param blockBytes The bytes of one block.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isConsistentChain (std::span< const CompressedMipLevel > levels, uint32_t blockBytes) noexcept
		{
			const auto baseWidth = levels.front().width;
			const auto baseHeight = levels.front().height;

			if ( baseWidth == 0 || baseHeight == 0 )
			{
				return false;
			}

			uint32_t index = 0;

			return std::ranges::all_of(levels, [&index, baseWidth, baseHeight, blockBytes] (const CompressedMipLevel & level) {
				const auto width = std::max(1U, baseWidth >> index);
				const auto height = std::max(1U, baseHeight >> index);

				++index;

				const auto blocks = static_cast< uint64_t >((width + 3) / 4) * ((height + 3) / 4);

				return level.width == width && level.height == height && static_cast< uint64_t >(level.data.size()) == blocks * blockBytes;
			});
		}
	}

	bool
	KTX2Writer::write (const std::filesystem::path & filepath, VkFormat format, std::span< const CompressedMipLevel > levels, uint32_t zstdLevel) noexcept
	{
		const auto blockBytes = blockBytesOf(format);

		if ( blockBytes == 0 )
		{
			TraceError{ClassId} << "The format " << static_cast< int >(format) << " is not a block format the writer handles (BC7, BC5, BC4) !";

			return false;
		}

		if ( levels.empty() || levels.size() > 32 || !isConsistentChain(levels, blockBytes) )
		{
			TraceError{ClassId} << "The mip chain for '" << filepath << "' is empty, longer than 32 levels, or inconsistent with its format !";

			return false;
		}

		if ( zstdLevel > 22 )
		{
			TraceError{ClassId} << "The zstd level " << zstdLevel << " is out of range (0 to 22) !";

			return false;
		}

		ktxTextureCreateInfo createInfo{};
		createInfo.vkFormat = static_cast< ktx_uint32_t >(format);
		createInfo.baseWidth = levels.front().width;
		createInfo.baseHeight = levels.front().height;
		createInfo.baseDepth = 1;
		createInfo.numDimensions = 2;
		createInfo.numLevels = static_cast< ktx_uint32_t >(levels.size());
		createInfo.numLayers = 1;
		createInfo.numFaces = 1;
		createInfo.isArray = KTX_FALSE;
		createInfo.generateMipmaps = KTX_FALSE;

		ktxTexture2 * rawTexture = nullptr;

		if ( const auto error = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &rawTexture); error != KTX_SUCCESS || rawTexture == nullptr )
		{
			TraceError{ClassId} << "Unable to create the KTX2 container for '" << filepath << "' : " << ktxErrorString(error);

			return false;
		}

		const std::unique_ptr< ktxTexture2, TextureDeleter > texture{rawTexture};

		ktx_uint32_t levelIndex = 0;

		for ( const auto & level : levels )
		{
			if ( const auto error = ktxTexture_SetImageFromMemory(asBaseTexture(texture.get()), levelIndex, 0, 0, level.data.data(), level.data.size()); error != KTX_SUCCESS )
			{
				TraceError{ClassId} << "Unable to store the level " << levelIndex << " of '" << filepath << "' : " << ktxErrorString(error);

				return false;
			}

			++levelIndex;
		}

		if ( zstdLevel > 0 )
		{
			if ( const auto error = ktxTexture2_DeflateZstd(texture.get(), zstdLevel); error != KTX_SUCCESS )
			{
				TraceError{ClassId} << "Unable to supercompress '" << filepath << "' with zstd : " << ktxErrorString(error);

				return false;
			}
		}

		ktx_uint8_t * rawBytes = nullptr;
		ktx_size_t byteCount = 0;

		if ( const auto error = ktxTexture_WriteToMemory(asBaseTexture(texture.get()), &rawBytes, &byteCount); error != KTX_SUCCESS || rawBytes == nullptr )
		{
			TraceError{ClassId} << "Unable to serialize the KTX2 container for '" << filepath << "' : " << ktxErrorString(error);

			return false;
		}

		const std::unique_ptr< ktx_uint8_t, BufferDeleter > bytes{rawBytes};

		if ( !Base::IO::filePutContents(filepath, std::span< const ktx_uint8_t >{bytes.get(), byteCount}, false, true) )
		{
			TraceError{ClassId} << "Unable to write the KTX2 file '" << filepath << "' !";

			return false;
		}

		return true;
	}
}
