/*
 * tools/texture-bake/TextureBake.hpp
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

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

/* Local inclusions. */
#include "PixelFactory/Types.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class TextureCompressor;
}

/* NOTE: EmeraudeTextureBake's own declarations, shared by its two modes (TextureBake.cpp, MaterialBake.cpp). */
namespace EmEn::TextureBake
{
	/** @brief One output channel's source: one channel of an image, or a constant. */
	struct ChannelArgument final
	{
		std::filesystem::path image;
		Base::PixelFactory::Channel channel{Base::PixelFactory::Channel::Red};
		uint8_t constant{255};
		bool fromImage{false};
	};

	/** @brief One texture to bake. */
	struct BakeArguments final
	{
		std::filesystem::path output;
		std::array< ChannelArgument, 4 > channels{};
		uint32_t zstdLevel{18};
		bool colour{false};
		bool normal{false};
		bool flipNormalY{false};
		bool kindGiven{false};
	};

	/**
	 * @brief Bakes one texture: every distinct image read once, the channels filled (PackRecipe::fillChannel(), the
	 * largest source size), the normal flipped if asked, the block format selected, the full mip chain compressed, the
	 * KTX2 container written (its directories created).
	 * @param arguments The texture.
	 * @param compressor An initialized compressor.
	 * @return bool False, with the reason on the error stream.
	 */
	[[nodiscard]]
	bool bakeTexture (const BakeArguments & arguments, const Graphics::TextureCompressor & compressor) noexcept;

	/**
	 * @brief Runs the material mode (graphics § 40, L3): bakes the GPUTextures entries of one material definition, or of
	 * every definition under a directory of the Materials store.
	 * @param arguments The command line, without the program name.
	 * @param compressor An initialized compressor.
	 * @return int The process exit code.
	 */
	[[nodiscard]]
	int runMaterialMode (std::span< const std::string_view > arguments, const Graphics::TextureCompressor & compressor) noexcept;
}
