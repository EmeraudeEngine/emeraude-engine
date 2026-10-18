/*
 * src/Scenes/Loaders/GLTFParser.hpp
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
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

/* Local inclusions for usages. */
#include "Animation/AnimationClip.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Skin.hpp"
#include "Math/Matrix.hpp"
#include "VertexFactory/Shape.hpp"
#include "LoaderOptions.hpp"
#include "SceneData.hpp"

/* Forward declarations (fastgltf stays out of the engine headers). */
namespace fastgltf
{
	class Asset;
	class GltfDataBuffer;
	struct Material;
}

/**
 * @brief The analysis half of the glTF loader: the parse, every validation, the buffer decoding (EXT/KHR meshopt,
 * KHR_draco_mesh_compression), one Shape per mesh, the skins, the clips, the lights, the cameras and the node tree —
 * with no engine resource, service or device.
 * @note Split from GLTFLoader on 2026-10-18 (owner decision: analysis / materialization; the MATERIALS stay in the
 * materialization, which reads them from the validated fastgltf asset this analysis hands over). The decoded buffer views
 * stay alive in the Asset until the loader has read its images (bufferView()), then releaseBuffers(). Fuzzed by
 * src/Fuzzing/fuzz_gltf.cpp.
 */
namespace EmEn::Scenes::Loaders::GLTF
{
	/** @brief The decoded EXT/KHR meshopt buffer views, defined in GLTFParser.cpp. */
	class MeshoptBufferCache;

	/** @brief One glTF mesh. A null shape means the default mesh (no triangle, a failed read or an invalid shape). */
	struct Mesh final
	{
		std::string geometryName;
		std::string meshName;
		std::shared_ptr< Base::VertexFactory::Shape< float > > shape;
		/** @brief Every primitive declares TANGENT: the authored tangents are kept (none computed). */
		bool allPrimitivesHaveTangents{false};
		bool hasVertexColors{false};
		/** @brief A primitive samples TEXCOORD_1 with a map: the secondary set is read. */
		bool samplesSecondarySet{false};
	};

	/** @brief One glTF skin: its skeleton and skin, in parent-first joint order. */
	struct Skin final
	{
		std::string resourceName;
		Base::Animation::Skeleton< float > skeleton;
		Base::Animation::Skin< float > skin;
	};

	/** @brief One animation clip (skeletal or node). */
	struct Clip final
	{
		std::string resourceName;
		Base::Animation::AnimationClip< float > clip;
	};

	/** @brief One glTF asset, parsed, validated and analysed. */
	struct Asset final
	{
		Asset () noexcept;

		~Asset ();

		Asset (const Asset &) = delete;

		Asset (Asset &&) noexcept;

		Asset & operator= (const Asset &) = delete;

		Asset & operator= (Asset &&) noexcept;

		/** @brief The validated fastgltf asset (the materialization reads its images and materials). */
		std::unique_ptr< fastgltf::Asset > gltf;
		/** @brief The decoded meshopt buffer views, alive until releaseBuffers(). */
		std::unique_ptr< MeshoptBufferCache > buffers;
		/** @brief One per glTF mesh. */
		std::vector< Mesh > meshes;
		/** @brief Indexed like the loader's skeletons: a skin without joint is skipped (the glTF skin index of
		 * meshToSkinIndex then points past it, as before the split). */
		std::vector< Skin > skins;
		std::unordered_map< size_t, size_t > meshToSkinIndex;
		std::unordered_map< size_t, Base::Math::Matrix< 4, float > > meshSkinRootTransform;
		std::unordered_map< size_t, size_t > meshMaxJointIndex;
		std::vector< Clip > clips;
		std::vector< Clip > nodeClips;
		/** @brief The lights, cameras, node tree, roots and skin joint nodes. */
		SceneData scene;
		/** @brief False when a mesh fell back to the default mesh (the loader then refuses the asset). */
		bool meshesComplete{true};
	};

	/**
	 * @brief Returns the texture coordinate set a texture slot samples (KHR_texture_transform's override included).
	 * @tparam texture_info_t The fastgltf texture info type (optional).
	 * @param textureInfoOpt The slot.
	 * @return uint32_t
	 */
	/* The texture coordinate set a texture info samples (0 without one). */
	template< typename texture_info_t >
	[[nodiscard]]
	uint32_t
	textureCoordinateSet (const texture_info_t & textureInfoOpt) noexcept
	{
		if ( !textureInfoOpt.has_value() )
		{
			return 0;
		}

		if ( textureInfoOpt->transform != nullptr && textureInfoOpt->transform->texCoordIndex.has_value() )
		{
			return static_cast< uint32_t >(*textureInfoOpt->transform->texCoordIndex);
		}

		return static_cast< uint32_t >(textureInfoOpt->texCoordIndex);
	}

	/**
	 * @brief Returns whether a material samples TEXCOORD_1 with a map.
	 * @param material The material.
	 * @return bool
	 */
	[[nodiscard]]
	bool materialSamplesSecondarySet (const fastgltf::Material & material) noexcept;

	/**
	 * @brief Parses and validates a glTF (refused: a parse error, a buffer or image uri outside of basePath, a missing or
	 * short buffer file, invalid indices, fastgltf::validate(), a node hierarchy that is not a strict forest,
	 * inconsistent extents).
	 * @param data The glTF or GLB bytes.
	 * @param basePath The asset's directory: every external buffer and image is confined to it (IO::confinedPath()).
	 * @param binary Whether the bytes are a GLB.
	 * @param sourceName The name the logs use.
	 * @return std::optional< Asset >
	 */
	[[nodiscard]]
	std::optional< Asset > parse (fastgltf::GltfDataBuffer & data, const std::filesystem::path & basePath, bool binary, const std::string & sourceName) noexcept;

	/**
	 * @brief Analyses a parsed asset: the mesh shapes, then (unless LoaderOptions::skipSkinning) the skins and the clips,
	 * then the lights, cameras and node tree.
	 * @param analysed The parsed asset.
	 * @param resourcePrefix The prefix of every resource name ("glTF:<path>/").
	 * @param options The loader options.
	 * @param sourceName The name the logs use.
	 * @return bool False when a skinned mesh references a joint past its skin (the asset is refused).
	 */
	[[nodiscard]]
	bool analyse (Asset & analysed, const std::string & resourcePrefix, const LoaderOptions & options, const std::string & sourceName) noexcept;

	/**
	 * @brief Returns the bytes of a buffer view, decoded when it is meshopt-compressed (an image's source).
	 * @param analysed The asset.
	 * @param bufferViewIndex The buffer view.
	 * @return std::span< const std::byte > Empty when out of range.
	 */
	[[nodiscard]]
	std::span< const std::byte > bufferView (Asset & analysed, size_t bufferViewIndex) noexcept;

	/**
	 * @brief Returns whether a meshopt-compressed buffer view failed to decode (the loader refuses the asset).
	 * @param analysed The asset.
	 * @return bool
	 */
	[[nodiscard]]
	bool buffersFailed (const Asset & analysed) noexcept;

	/**
	 * @brief Releases the decoded buffer views (reported in the log).
	 * @param analysed The asset.
	 */
	void releaseBuffers (Asset & analysed) noexcept;
}
