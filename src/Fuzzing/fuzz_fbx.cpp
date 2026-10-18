/*
 * src/Fuzzing/fuzz_fbx.cpp
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
 * libFuzzer target for the FBX loader's analysis (Ave Robustus II P-W, gap 7, 2026-10-18): ufbx's parse of the bytes
 * with the loader's options, then the whole conversion — images, materials, mesh triangulation and skin weights,
 * skeletons, the 30 Hz animation sampling, the node tree — and the animation-only path of a split-animation file, with
 * no engine resource (FBX::analyse() / FBX::analyseClipsOnly(), split from FBXLoader for this). Built by build-fuzzers.sh.
 */

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>

/* Third-party inclusions. */
#include <ufbx/ufbx.h>

/* Local inclusions. */
#include "Scenes/Loaders/FBXParser.hpp"

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
	using namespace EmEn::Scenes::Loaders;

	/* NOTE: The loader's options, without the external files: the fuzzer reads bytes only. */
	ufbx_load_opts loadOptions{};
	FBX::setLoadOptions(loadOptions, true, size);
	loadOptions.load_external_files = false;

	ufbx_error error{};
	const std::unique_ptr< ufbx_scene, decltype(&ufbx_free_scene) > scene{ufbx_load_memory(data, size, &loadOptions, &error), ufbx_free_scene};

	if ( scene == nullptr )
	{
		return 0;
	}

	LoaderOptions options{};
	options.stripRootMotion = true;

	auto asset = FBX::analyse(*scene, "FBX:fuzz/", {}, options);

	/* NOTE: What the materialization runs on every shape, on a resource loading thread. */
	for ( const auto & mesh : asset.meshes )
	{
		if ( mesh.shape != nullptr )
		{
			mesh.shape->computeTriangleTangent();
			mesh.shape->computeVertexTangent();
		}
	}

	/* NOTE: The animation-only path against the first skeleton, on the same scene: a split-animation file is parsed
	 * with the same conversions (no external file, no generated normal — neither changes the curves). */
	for ( const auto & skin : asset.skins )
	{
		if ( skin.has_value() && !skin->skeleton.empty() )
		{
			static_cast< void >(FBX::analyseClipsOnly(*scene, skin->skeleton, "fuzz", options));

			break;
		}
	}

	return 0;
}
