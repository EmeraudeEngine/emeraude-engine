/*
 * src/Fuzzing/fuzz_wad.cpp
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

/*
 * libFuzzer target for the WAD loader's analysis (Ave Robustus II P-W, gap 7, 2026-10-18): the whole parse of a WAD —
 * directory, map lumps, TEXTURE1/2 and PNAMES, patch composition, the BSP walk, the level shape — with no engine
 * resource (WAD::parse(), split from WADLoader::load() for this). Built by build-fuzzers.sh.
 */

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <span>

/* Local inclusions. */
#include "Scenes/Loaders/WADParser.hpp"

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
	using namespace EmEn::Scenes::Loaders;

	/* NOTE: The first map of the WAD, as the doom-loader demo reads it. */
	static_cast< void >(WAD::parse(std::span< const uint8_t >{data, size}, WAD::Selection{}, "fuzz.wad", 1.0F));

	return 0;
}
