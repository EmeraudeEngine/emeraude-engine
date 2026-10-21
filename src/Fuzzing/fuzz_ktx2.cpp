/*
 * src/Fuzzing/fuzz_ktx2.cpp
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

/*
 * libFuzzer target for the KTX2 key/value reader (graphics § 40, L4 and L6): KTX2Decoder::readKeyValue() over a
 * container in memory — a glTF's embedded or sidecar image, read for the bake's fingerprint and the alpha coverage the
 * bake recorded. The container's header and key/value block are untrusted: every offset and length is checked against
 * the bytes. Built by build-fuzzers.sh.
 */

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <span>

/* Local inclusions. */
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/KTX2Writer.hpp"

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
	using namespace EmEn::Graphics;

	const auto bytes = std::as_bytes(std::span< const uint8_t >{data, size});

	static_cast< void >(KTX2Decoder::readKeyValue(bytes, KTX2Writer::AlphaCoverageKey));
	static_cast< void >(KTX2Decoder::readKeyValue(bytes, KTX2Writer::BakeFingerprintKey));

	return 0;
}
