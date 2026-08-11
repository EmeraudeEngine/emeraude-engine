/*
 * src/Graphics/KTX2Decoder.hpp
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
#include <span>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "PixelFactory/Pixmap.hpp"
#include "TextureCompressor.hpp"

namespace EmEn::Graphics
{
	/**
	 * @class KTX2Decoder
	 * @brief Decodes KTX2 containers into GPU-ready block-compressed mip chains.
	 *
	 * KTX2 (KhronosGroup container, `.ktx2`) is the transport format behind the glTF
	 * `KHR_texture_basisu` extension. The payload is usually **Basis Universal** — either
	 * UASTC (high quality, zstd-supercompressed) or ETC1S (BasisLZ) — which is not a GPU
	 * format: it must be *transcoded* to whatever block format the device actually samples.
	 * That transcode is a block-to-block table conversion, not a re-encode, so it is one to
	 * two orders of magnitude cheaper than compressing an RGBA source with
	 * @ref TextureCompressor, and it never round-trips through uncompressed pixels.
	 *
	 * This is why a KTX2 asset is lighter *in memory*, not merely on disk: the CPU-side
	 * payload stays block-compressed from file to VkImage. A 4096×4096 texture costs
	 * ~22 MiB of BC7 with its full mip chain, against ~89 MiB for the RGBA8 level 0 alone
	 * that the PNG/JPEG path has to materialise before it can compress anything.
	 *
	 * The engine supports exactly one block format (BC7, see @ref TextureCompressor), so
	 * that is the transcode target. A KTX2 that already carries a real `vkFormat` (i.e. it
	 * needs no transcoding) is passed through with its own format untouched.
	 *
	 * @note Output mip levels are ordered level 0 (largest) first, which is the order
	 * @ref Vulkan::Image::createFromCompressed() expects. The KTX2 file itself stores them
	 * smallest-first; libktx hides that.
	 * @note Stateless utility class, all methods are static.
	 * @see EmEn::Graphics::CompressedImageResource
	 * @see EmEn::Graphics::TextureCompressor
	 * @version 0.8.36
	 */
	class EMEN_API KTX2Decoder final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"KTX2Decoder"};

			/**
			 * @brief Decoding options.
			 */
			struct Options
			{
				/**
				 * @brief Largest accepted mip dimension, in pixels. 0 disables clamping.
				 *
				 * Levels wider or taller than this are dropped and the first level that fits
				 * becomes level 0 of the result. Because a KTX2 ships its whole mip chain,
				 * clamping is free: nothing is resampled, the top levels are simply not kept.
				 * Halving the dimension divides the VRAM footprint by four.
				 */
				uint32_t maxDimension{0};
			};

			/**
			 * @brief A decoded, GPU-ready block-compressed image.
			 */
			struct Result
			{
				std::vector< CompressedMipLevel > mips; ///< Block-compressed levels, level 0 first.
				VkFormat format{VK_FORMAT_UNDEFINED}; ///< Linear (UNORM) format of the blocks.

				/**
				 * @brief Returns whether the decode produced usable data.
				 * @return bool
				 */
				[[nodiscard]]
				bool
				isValid () const noexcept
				{
					return !mips.empty() && format != VK_FORMAT_UNDEFINED;
				}
			};

			/**
			 * @brief Returns whether a byte blob starts with the KTX2 identifier.
			 * @param bytes The blob to probe. Shorter blobs simply return false.
			 * @return bool
			 * @note Cheap magic-number check, it does not validate the rest of the container.
			 */
			[[nodiscard]]
			static bool isKTX2 (std::span< const std::byte > bytes) noexcept;

			/**
			 * @brief Transcodes a KTX2 blob to a block-compressed mip chain.
			 * @param bytes The KTX2 container bytes.
			 * @param options Decoding options.
			 * @param label A name used for tracing only.
			 * @return A Result, invalid on failure.
			 * @note The returned format is always the *linear* variant. Picking the sRGB
			 * variant of the same block layout is the consumer's decision, taken from the
			 * texture's usage, not from the container — the transcoded bits are identical
			 * either way. See @ref sRGBFormat().
			 */
			[[nodiscard]]
			static Result decodeCompressed (std::span< const std::byte > bytes, const Options & options, const std::string & label) noexcept;

			/**
			 * @brief Transcodes level 0 of a KTX2 blob to an RGBA8 pixmap.
			 * @param bytes The KTX2 container bytes.
			 * @param options Decoding options.
			 * @param label A name used for tracing only.
			 * @param output A reference to the pixmap to write to.
			 * @return bool
			 * @note Fallback for devices without block-compression support. It defeats the
			 * whole memory argument of KTX2, so it is a correctness net, not a target path.
			 */
			[[nodiscard]]
			static bool decodeToPixmap (std::span< const std::byte > bytes, const Options & options, const std::string & label, Base::PixelFactory::Pixmap< uint8_t > & output) noexcept;

			/**
			 * @brief Maps a linear block format to its sRGB counterpart.
			 * @param format A linear block format, as returned by decodeCompressed().
			 * @return The sRGB variant, or the input unchanged when it has none.
			 */
			[[nodiscard]]
			static VkFormat sRGBFormat (VkFormat format) noexcept;

		private:

			KTX2Decoder () = delete;
	};
}
