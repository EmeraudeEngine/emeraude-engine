/*
 * src/Fuzzing/fuzz_usd.cpp
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
 * libFuzzer target for the USD loader's analysis (Ave Robustus II P-W, gap 7, 2026-10-18): the root layer read from
 * the bytes (USDA or USDC, or a USDZ archive held in memory), its composition (sublayers and every arc), Tydra's render
 * scene, the materials, the PointInstancers, the lights and every baked mesh shape, with no engine resource
 * (USD::composeStage() / USD::analyse() / USD::forEachMesh(), split from USDLoader for this). Built by build-fuzzers.sh.
 *
 * NOTE: Hermetic: a USDZ resolves against its own table only, and a plain layer's resolver refuses every asset — an
 * input never reads the file system.
 */

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "lightusd.hh"
#include "asset-resolution.hh"

/* Local inclusions. */
#include "Scenes/Loaders/USDParser.hpp"
#include "Scenes/Loaders/USDZArchive.hpp"

namespace
{
	/* A resolver handler that resolves nothing: the plain layer's arcs and sublayers stay unresolved. */
	int
	refuse (const char * /*assetName*/, const std::vector< std::string > & /*searchPaths*/, std::string * /*resolvedName*/, std::string * /*error*/, void * /*userdata*/)
	{
		return -1;
	}
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
	using namespace EmEn::Scenes::Loaders;

	lightusd::AssetResolutionResolver resolver;
	resolver.set_allow_parent_relative_paths(true);

	std::vector< std::string > searchPaths;
	std::string usdFilename{"fuzz.usd"};
	lightusd::Layer layer;
	std::string warning;
	std::string error;

	USDZArchive archive;

	/* NOTE: The loader's budget for a root layer, on the input's size. */
	lightusd::USDLoadOptions loadOptions;
	USD::setLoadOptions(loadOptions, size);

	if ( size >= 4 && std::memcmp(data, "PK\x03\x04", 4) == 0 )
	{
		if ( !archive.openMemory(std::vector< uint8_t >{data, data + size}, "fuzz.usdz") )
		{
			return 0;
		}

		archive.equip(resolver);
		searchPaths.emplace_back(archive.baseDirectory());
		usdFilename = archive.rootAssetName();

		if ( !lightusd::LoadLayerFromAsset(resolver, archive.rootAssetName(), &layer, &warning, &error, loadOptions) )
		{
			return 0;
		}
	}
	else
	{
		lightusd::AssetResolutionHandler handler;
		handler.resolve_fun = &refuse;
		resolver.register_asset_resolution_handler("*", handler);

		if ( !lightusd::LoadLayerFromMemory(data, size, usdFilename, &layer, &warning, &error, loadOptions) )
		{
			return 0;
		}
	}

	lightusd::Stage stage;
	USD::StageMetrics metrics;

	if ( !USD::composeStage(resolver, layer, true, "fuzz", stage, metrics) )
	{
		return 0;
	}

	const auto asset = USD::analyse(stage, resolver, searchPaths, usdFilename, metrics, "usd:fuzz", "fuzz");

	if ( !asset.has_value() )
	{
		return 0;
	}

	/* NOTE: What the materialization runs on every shape, on a resource loading thread. */
	static_cast< void >(USD::forEachMesh(*asset, [] (USD::Mesh mesh) {
		mesh.shape->computeTriangleTangent();
		mesh.shape->computeVertexTangent();
	}));

	return 0;
}
