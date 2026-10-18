/*
 * src/Scenes/Loaders/WADParser.hpp
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
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "VertexFactory/Shape.hpp"

/**
 * @brief The analysis half of the WAD loader: the bytes of a WAD become a plain description of one map — composed
 * textures, animated ranges, the level shape, its surfaces — with no engine resource, no service, no device.
 * @note Split from WADLoader::load() on 2026-10-18 (owner decision: analysis / materialization) so the whole parse and
 * every validation it carries can be fuzzed (src/Fuzzing/fuzz_wad.cpp); WADLoader::load() materializes the result.
 */
namespace EmEn::Scenes::Loaders::WAD
{
	/** @brief Doom map units per meter: the player is 56 units tall (~1.75 m). */
	constexpr float MapUnitsPerMeter{32.0F};

	/** @brief The largest side of a composed texture: TEXTURE1/2 declare it in 16 bits, a hostile WAD 32 768 × 32 768
	 * (fuzz_wad, 2026-10-18). doom.wad's largest is 256 × 128; 4096 keeps the modern ports' tall walls and wide skies. */
	constexpr uint32_t MaxTextureSide{4096};

	/** @brief The pixels one map may compose, every texture, flat and animation frame together: 64 M (17 times the
	 * whole of doom.wad, ~400 MB of working memory). Beyond, a texture is skipped (owner decision, 2026-10-18). */
	constexpr uint64_t MaxComposedPixels{64ULL * 1024ULL * 1024ULL};

	/** @brief The duration of one frame of an animated flat or wall texture: 8 game tics at 35 Hz. */
	constexpr uint32_t AnimationFrameDurationMS{228};

	/** @brief Which map of the WAD to read: by name when one is given, else by its 1-based index. */
	struct Selection final
	{
		std::string mapName;
		uint32_t mapIndex{1};
	};

	/** @brief A static wall texture or flat, composed from its patches and the palette. */
	struct Texture final
	{
		std::vector< uint8_t > rgba;
		uint16_t width{0};
		uint16_t height{0};
	};

	/** @brief An animated range (NUKAGE1..NUKAGE3, …): every name of the range shares its frames. */
	struct AnimatedRange final
	{
		std::string suffix;
		std::vector< std::string > names;
		std::vector< Base::PixelFactory::Pixmap< uint8_t > > frames;
	};

	/** @brief One surface of the level: a texture name and whether it cuts out on its alpha. */
	struct Surface final
	{
		std::string textureName;
		bool masked{false};
	};

	/** @brief One map of a WAD, analysed. */
	struct Level final
	{
		std::string mapName;
		std::map< std::string, Texture > textures;
		std::vector< AnimatedRange > animatedRanges;
		/** @brief The level geometry, one sub-geometry group per surface, in the order of `surfaces`. */
		std::shared_ptr< Base::VertexFactory::Shape< float > > shape;
		std::vector< Surface > surfaces;
		Base::Math::Vector< 3, float > playerStartPosition;
		Base::Math::Vector< 3, float > playerStartDirection{0.0F, 0.0F, 1.0F};
	};

	/**
	 * @brief Analyses one map of a WAD.
	 * @param wad The bytes of the whole WAD.
	 * @param selection Which map to read.
	 * @param sourceName The name of the WAD in the messages.
	 * @param uniformScale The loader option's uniform scale (0 or less: 1).
	 * @return std::optional< Level > Nothing when the WAD is refused (the reason is traced).
	 */
	[[nodiscard]]
	std::optional< Level > parse (std::span< const uint8_t > wad, const Selection & selection, std::string_view sourceName, float uniformScale) noexcept;
}
