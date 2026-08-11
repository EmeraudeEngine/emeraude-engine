/*
 * src/Graphics/KTX2Decoder.cpp
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

#include "KTX2Decoder.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstring>

/* Third-party inclusions. */
#include <ktx.h>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base::PixelFactory;

	/* The 12-byte KTX2 file identifier, as mandated by the KTX 2.0 specification. */
	static constexpr std::array< uint8_t, 12 > KTX2Identifier{0xABU, 0x4BU, 0x54U, 0x58U, 0x20U, 0x32U, 0x30U, 0xBBU, 0x0DU, 0x0AU, 0x1AU, 0x0AU};

	/* Linear <-> sRGB pairs for every block format the engine can meet. The transcoder
	 * always targets BC7; the remaining entries only matter for a KTX2 that already
	 * carries a real vkFormat and is passed through without transcoding. */
	struct FormatPair
	{
		VkFormat linear;
		VkFormat nonLinear;
	};

	static constexpr std::array< FormatPair, 9 > BlockFormatPairs{{
		{VK_FORMAT_BC1_RGB_UNORM_BLOCK, VK_FORMAT_BC1_RGB_SRGB_BLOCK},
		{VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
		{VK_FORMAT_BC2_UNORM_BLOCK, VK_FORMAT_BC2_SRGB_BLOCK},
		{VK_FORMAT_BC3_UNORM_BLOCK, VK_FORMAT_BC3_SRGB_BLOCK},
		{VK_FORMAT_BC7_UNORM_BLOCK, VK_FORMAT_BC7_SRGB_BLOCK},
		{VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
		{VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
		{VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
		{VK_FORMAT_ASTC_4x4_UNORM_BLOCK, VK_FORMAT_ASTC_4x4_SRGB_BLOCK}
	}};

	/* Normalizes a format to its linear variant. libktx derives the vkFormat of the
	 * transcoded data from the container's transfer function, so an sRGB-tagged asset comes
	 * back as *_SRGB_BLOCK. The bits are the same either way: the colour space belongs to
	 * the texture's usage, decided by the consumer, so we hand back the linear variant and
	 * let it choose. */
	static
	VkFormat
	toLinearFormat (VkFormat format) noexcept
	{
		const auto pairIt = std::ranges::find_if(BlockFormatPairs, [format] (const auto & pair) {
			return pair.nonLinear == format;
		});

		return pairIt != BlockFormatPairs.cend() ? pairIt->linear : format;
	}

	VkFormat
	KTX2Decoder::sRGBFormat (VkFormat format) noexcept
	{
		const auto pairIt = std::ranges::find_if(BlockFormatPairs, [format] (const auto & pair) {
			return pair.linear == format;
		});

		return pairIt != BlockFormatPairs.cend() ? pairIt->nonLinear : format;
	}

	bool
	KTX2Decoder::isKTX2 (std::span< const std::byte > bytes) noexcept
	{
		if ( bytes.size() < KTX2Identifier.size() )
		{
			return false;
		}

		return std::memcmp(bytes.data(), KTX2Identifier.data(), KTX2Identifier.size()) == 0;
	}

	/* Opens a KTX2 blob and transcodes it, when needed, to the requested target.
	 * Returns nullptr on failure, an owning ktxTexture2 * otherwise. */
	static
	ktxTexture2 *
	openAndTranscode (std::span< const std::byte > bytes, ktx_transcode_fmt_e target, const std::string & label) noexcept
	{
		ktxTexture2 * texture = nullptr;

		const auto createError = ktxTexture2_CreateFromMemory(
			reinterpret_cast< const ktx_uint8_t * >(bytes.data()),
			static_cast< ktx_size_t >(bytes.size()),
			KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
			&texture
		);

		if ( createError != KTX_SUCCESS || texture == nullptr )
		{
			TraceError{KTX2Decoder::ClassId} << "Unable to open the KTX2 container '" << label << "' : " << ktxErrorString(createError);

			return nullptr;
		}

		if ( ktxTexture2_NeedsTranscoding(texture) )
		{
			const auto transcodeError = ktxTexture2_TranscodeBasis(texture, target, 0);

			if ( transcodeError != KTX_SUCCESS )
			{
				TraceError{KTX2Decoder::ClassId} << "Unable to transcode the KTX2 container '" << label << "' : " << ktxErrorString(transcodeError);

				ktxTexture_Destroy(ktxTexture(texture));

				return nullptr;
			}
		}

		return texture;
	}

	/* Returns the first mip level whose dimensions fit within the clamp, and the count of
	 * levels kept from there. A clamp bigger than the texture keeps everything. */
	static
	uint32_t
	firstFittingLevel (const ktxTexture2 * texture, uint32_t maxDimension) noexcept
	{
		if ( maxDimension == 0 )
		{
			return 0;
		}

		for ( uint32_t level = 0; level < texture->numLevels; ++level )
		{
			const auto width = std::max(1U, texture->baseWidth >> level);
			const auto height = std::max(1U, texture->baseHeight >> level);

			if ( width <= maxDimension && height <= maxDimension )
			{
				return level;
			}
		}

		/* Every level is oversized (a clamp below 1 pixel is nonsense) : keep the smallest. */
		return texture->numLevels - 1;
	}

	KTX2Decoder::Result
	KTX2Decoder::decodeCompressed (std::span< const std::byte > bytes, const Options & options, const std::string & label) noexcept
	{
		auto * texture = openAndTranscode(bytes, KTX_TTF_BC7_RGBA, label);

		if ( texture == nullptr )
		{
			return {};
		}

		Result result;

		if ( texture->isCompressed == KTX_FALSE )
		{
			TraceError{ClassId} << "The KTX2 container '" << label << "' holds uncompressed data, it cannot feed the block-compressed path !";

			ktxTexture_Destroy(ktxTexture(texture));

			return {};
		}

		result.format = toLinearFormat(static_cast< VkFormat >(texture->vkFormat));

		if ( result.format == VK_FORMAT_UNDEFINED )
		{
			TraceError{ClassId} << "The KTX2 container '" << label << "' yielded an undefined Vulkan format !";

			ktxTexture_Destroy(ktxTexture(texture));

			return {};
		}

		const auto baseLevel = firstFittingLevel(texture, options.maxDimension);

		result.mips.reserve(texture->numLevels - baseLevel);

		for ( uint32_t level = baseLevel; level < texture->numLevels; ++level )
		{
			ktx_size_t offset = 0;

			const auto offsetError = ktxTexture_GetImageOffset(ktxTexture(texture), level, 0, 0, &offset);

			if ( offsetError != KTX_SUCCESS )
			{
				TraceError{ClassId} << "Unable to locate the level " << level << " of the KTX2 container '" << label << "' : " << ktxErrorString(offsetError);

				ktxTexture_Destroy(ktxTexture(texture));

				return {};
			}

			const auto size = ktxTexture_GetImageSize(ktxTexture(texture), level);

			if ( size == 0 || offset + size > texture->dataSize )
			{
				TraceError{ClassId} << "The level " << level << " of the KTX2 container '" << label << "' overruns the payload !";

				ktxTexture_Destroy(ktxTexture(texture));

				return {};
			}

			const auto * levelData = texture->pData + offset;

			result.mips.emplace_back(CompressedMipLevel{
				.data = {levelData, levelData + size},
				.width = std::max(1U, texture->baseWidth >> level),
				.height = std::max(1U, texture->baseHeight >> level)
			});
		}

		if ( baseLevel > 0 )
		{
			TraceInfo{ClassId} <<
				"The KTX2 container '" << label << "' was clamped from " << texture->baseWidth << 'x' << texture->baseHeight <<
				" down to " << result.mips.front().width << 'x' << result.mips.front().height << " (" << baseLevel << " top level(s) dropped).";
		}

		ktxTexture_Destroy(ktxTexture(texture));

		return result;
	}

	bool
	KTX2Decoder::decodeToPixmap (std::span< const std::byte > bytes, const Options & options, const std::string & label, Pixmap< uint8_t > & output) noexcept
	{
		auto * texture = openAndTranscode(bytes, KTX_TTF_RGBA32, label);

		if ( texture == nullptr )
		{
			return false;
		}

		const auto baseLevel = firstFittingLevel(texture, options.maxDimension);

		ktx_size_t offset = 0;

		const auto offsetError = ktxTexture_GetImageOffset(ktxTexture(texture), baseLevel, 0, 0, &offset);

		if ( offsetError != KTX_SUCCESS )
		{
			TraceError{ClassId} << "Unable to locate the level " << baseLevel << " of the KTX2 container '" << label << "' : " << ktxErrorString(offsetError);

			ktxTexture_Destroy(ktxTexture(texture));

			return false;
		}

		const auto width = std::max(1U, texture->baseWidth >> baseLevel);
		const auto height = std::max(1U, texture->baseHeight >> baseLevel);
		const auto size = static_cast< ktx_size_t >(width) * height * 4;

		if ( offset + size > texture->dataSize )
		{
			TraceError{ClassId} << "The level " << baseLevel << " of the KTX2 container '" << label << "' overruns the payload !";

			ktxTexture_Destroy(ktxTexture(texture));

			return false;
		}

		const auto success = output.initialize(width, height, ChannelMode::RGBA, {texture->pData + offset, size});

		ktxTexture_Destroy(ktxTexture(texture));

		if ( !success )
		{
			TraceError{ClassId} << "Unable to build a pixmap from the KTX2 container '" << label << "' !";

			return false;
		}

		return true;
	}
}
