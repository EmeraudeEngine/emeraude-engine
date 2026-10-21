/*
 * src/Graphics/KTX2Writer.hpp
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
#include <filesystem>
#include <span>

/* Third-party inclusions. */
#include "volk.h"

/* Forward declarations. */
namespace EmEn::Graphics
{
	struct CompressedMipLevel;
}

namespace EmEn::Graphics
{
	/**
	 * @class KTX2Writer
	 * @brief Writes a block-compressed mip chain into a KTX2 container, ready for a direct upload (graphics § 39, the
	 * baked assets).
	 *
	 * The counterpart of @ref KTX2Decoder for the engine's own formats: BC7 (sRGB or linear), BC5 and BC4, the blocks
	 * that @ref TextureCompressor produces. The container carries the real `vkFormat`, so it needs no transcoding at
	 * load time — @ref KTX2Decoder passes it through and the image is uploaded as stored. Written through libktx
	 * (KhronosGroup KTX-Software, Apache 2.0, https://github.com/KhronosGroup/KTX-Software), the library the decoder
	 * already uses.
	 *
	 * @note A glTF that references such a container through `KHR_texture_basisu` is meant for Emeraude only: the
	 * extension requires Basis Universal payloads (owner decision 2026-10-21).
	 * @note Stateless utility class, all methods are static.
	 * @see EmEn::Graphics::KTX2Decoder
	 */
	class EMEN_API KTX2Writer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"KTX2Writer"};

			/**
			 * @brief Writes a mip chain to a KTX2 file.
			 * @pre The levels are level 0 (largest) first, each one the size of its dimensions in its format, the
			 * dimensions halving down the chain (`max(1, base >> level)`) — checked, a mismatch is refused.
			 * @param filepath The file to write (its parent directories are created).
			 * @param format The block format: BC7 (UNORM or SRGB), BC5_UNORM or BC4_UNORM. Anything else is refused.
			 * @param levels The compressed levels (1 to 32).
			 * @param zstdLevel The zstd supercompression level (1–22, lossless: the file is smaller, the blocks the
			 * same), 0 for none.
			 * @return bool False, with an error, on a refused input or a libktx / file failure (nothing is left half
			 * written: the container is built in memory first).
			 */
			[[nodiscard]]
			static bool write (const std::filesystem::path & filepath, VkFormat format, std::span< const CompressedMipLevel > levels, uint32_t zstdLevel) noexcept;
	};
}
