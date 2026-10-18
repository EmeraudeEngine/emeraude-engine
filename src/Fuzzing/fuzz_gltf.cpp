/*
 * src/Fuzzing/fuzz_gltf.cpp
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
 * libFuzzer target for the glTF loader's analysis (Ave Robustus II P-W, gap 7, 2026-10-18): fastgltf's parse of a glTF
 * or GLB, the URI confinement, every validation, the EXT/KHR meshopt and Draco decoding, the shapes built from the
 * accessors, the skins, the animation sampling, the lights, the cameras and the node tree, then the image buffer views,
 * with no engine resource (GLTF::parse() / GLTF::analyse(), split from GLTFLoader for this). Built by build-fuzzers.sh.
 *
 * NOTE: Hermetic: every external buffer and image is confined to an EMPTY directory, so an input never reads a file.
 */

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <variant>

/* Third-party inclusions. */
#include "fastgltf/core.hpp"

/* Local inclusions. */
#include "Scenes/Loaders/GLTFParser.hpp"

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t size)
{
	using namespace EmEn::Scenes::Loaders;

	auto buffer = fastgltf::GltfDataBuffer::FromBytes(reinterpret_cast< const std::byte * >(data), size);

	if ( buffer.error() != fastgltf::Error::None )
	{
		return 0;
	}

	/* NOTE: A directory that holds nothing (and is never created): the confinement refuses every escaping uri, and a
	 * confined one finds no file. */
	static const std::filesystem::path EmptyDirectory{"/nonexistent/emeraude-fuzz-gltf"};

	const bool binary = size >= 4 && std::memcmp(data, "glTF", 4) == 0;

	auto analysed = GLTF::parse(buffer.get(), EmptyDirectory, binary, "fuzz.gltf");

	if ( !analysed.has_value() )
	{
		return 0;
	}

	if ( GLTF::analyse(*analysed, "glTF:fuzz/", LoaderOptions{}, "fuzz.gltf") )
	{
		/* NOTE: What the materialization runs: the tangents of every shape that carries none, and the image buffer
		 * views (decoded when meshopt-compressed). */
		for ( const auto & mesh : analysed->meshes )
		{
			if ( mesh.shape != nullptr && !mesh.allPrimitivesHaveTangents )
			{
				mesh.shape->computeTriangleTangent();
				mesh.shape->computeVertexTangent();
			}
		}

		for ( const auto & image : analysed->gltf->images )
		{
			if ( const auto * view = std::get_if< fastgltf::sources::BufferView >(&image.data); view != nullptr )
			{
				static_cast< void >(GLTF::bufferView(*analysed, view->bufferViewIndex));
			}
		}

		static_cast< void >(GLTF::buffersFailed(*analysed));
	}

	GLTF::releaseBuffers(*analysed);

	return 0;
}
