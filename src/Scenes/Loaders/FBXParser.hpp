/*
 * src/Scenes/Loaders/FBXParser.hpp
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
#include <string>
#include <unordered_set>
#include <vector>

/* Local inclusions for usages. */
#include "Animation/AnimationClip.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Skin.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "VertexFactory/Shape.hpp"
#include "LoaderOptions.hpp"
#include "SceneData.hpp"

/* Forward declarations (ufbx stays out of the public headers). */
struct ufbx_scene;
struct ufbx_load_opts;

/**
 * @brief The analysis half of the FBX loader: a ufbx scene becomes a plain description of the asset — its images,
 * materials, meshes (one Shape each), skins, animation clips and node tree — with no engine resource, service or device.
 * @note Split from FBXLoader on 2026-10-18 (owner decision: analysis / materialization) so the whole conversion and every
 * validation it carries can be fuzzed (src/Fuzzing/fuzz_fbx.cpp); FBXLoader materializes the result. The loader still
 * opens the file with ufbx_load_file() (external files included); the fuzzer parses bytes with ufbx_load_memory().
 */
namespace EmEn::Scenes::Loaders::FBX
{
	/** @brief The memory ceiling of each ufbx allocator (temporary and scene) for any file: 256 MiB. */
	constexpr size_t MinimumMemoryLimit{256ULL * 1024ULL * 1024ULL};

	/** @brief The memory ceiling of each ufbx allocator per byte of the file, above MinimumMemoryLimit: 16. Measured on
	 * real assets (2026-10-18): the temporary allocator peaks at 1x the file size, the scene at 6.5x (Knight_USD_002, 7.7
	 * MB -> 50 MB; the 1.7 GB IvyGrowth needs 1.36 GB). A binary FBX declares the size of its arrays and ufbx allocated
	 * them as declared: 50 KB asked for 14 GB (fuzz_fbx). Owner decision, 2026-10-18. */
	constexpr size_t MemoryLimitPerFileByte{16};

	/** @brief The keyframes one animation stack may produce (frames x animated joints x 3 channels): 4 M (~200 MB). The
	 * 30 Hz sampling sized itself on the duration the file declares: up to 10^12 samples asked (fuzz_fbx). Real data
	 * (2026-10-18): 12.5 s at most, 69 joints — the Paladin's longest clip is 261 frames x 69 joints x 3 = 54 k; 4 M is
	 * 10.7 min for 69 joints. A stack beyond, or of a non-finite duration, is skipped with a warning (owner decision). */
	constexpr uint64_t MaxClipKeyFrames{4ULL * 1024ULL * 1024ULL};

	/** @brief Where an image comes from. */
	enum class ImageKind : uint8_t
	{
		/** @brief Not a file texture (layered, procedural, shader): the default image. */
		None,
		/** @brief A file texture without any path: the default image, reported as a failure. */
		Unresolved,
		/** @brief The encoded bytes are inside the FBX. */
		Embedded,
		/** @brief An external file. */
		File
	};

	/** @brief One FBX texture's image, not decoded. */
	struct Image final
	{
		std::string resourceName;
		std::string textureName;
		std::shared_ptr< std::vector< std::byte > > bytes;
		std::filesystem::path path;
		size_t contentSize{0};
		Base::PixelFactory::Pixmap< uint8_t >::Format format{Base::PixelFactory::Pixmap< uint8_t >::Format::None};
		ImageKind kind{ImageKind::None};
	};

	/** @brief A material's use of a texture: its index and the colour space the usage needs. */
	struct TextureSlot final
	{
		size_t texture{0};
		bool sRGB{false};
	};

	/** @brief One FBX material, its maps referenced by texture index. */
	struct Material final
	{
		std::string resourceName;
		std::optional< TextureSlot > albedoTexture;
		std::optional< TextureSlot > roughnessTexture;
		std::optional< TextureSlot > metalnessTexture;
		std::optional< TextureSlot > normalTexture;
		std::optional< TextureSlot > ambientOcclusionTexture;
		std::optional< TextureSlot > emissiveTexture;
		std::optional< TextureSlot > opacityTexture;
		Base::PixelFactory::Color< float > albedoColor;
		Base::PixelFactory::Color< float > emissiveColor;
		Base::Math::Vector< 3, float > specularColor{1.0F, 1.0F, 1.0F};
		float roughnessFactor{0.5F};
		float metalnessFactor{0.0F};
		float emissiveStrength{0.0F};
		float opacityValue{1.0F};
		float specularFactor{1.0F};
		bool hasEmissiveColor{false};
		bool specularIsDielectricF0{false};
	};

	/** @brief One material part of a mesh: the FBX material (by its typed index) and its double-sided feature. */
	struct MeshPart final
	{
		std::optional< size_t > material;
		bool doubleSided{false};
	};

	/** @brief One FBX mesh. A null shape means the default mesh (no triangle, an invalid shape). */
	struct Mesh final
	{
		std::string geometryName;
		std::string meshName;
		std::shared_ptr< Base::VertexFactory::Shape< float > > shape;
		std::vector< MeshPart > parts;
		/** @brief The skin deformer (typed index) whose influences and weights the shape carries. */
		std::optional< size_t > skin;
	};

	/** @brief One skin deformer: its skeleton and skin, built in cluster order. */
	struct Skin final
	{
		std::string resourceName;
		Base::Animation::Skeleton< float > skeleton;
		Base::Animation::Skin< float > skin;
	};

	/** @brief One animation clip, sampled against the reference skin's joints. */
	struct Clip final
	{
		std::string resourceName;
		Base::Animation::AnimationClip< float > clip;
	};

	/** @brief One FBX asset, analysed. */
	struct Asset final
	{
		std::vector< Image > images;
		std::vector< Material > materials;
		std::vector< Mesh > meshes;
		/** @brief Indexed by the skin deformer's typed index: nothing for a deformer without cluster. */
		std::vector< std::optional< Skin > > skins;
		std::vector< Clip > clips;
		std::vector< NodeDescriptor > nodes;
		std::vector< size_t > rootNodeIndices;
		std::unordered_set< size_t > skinJointNodeIndices;
	};

	/**
	 * @brief Caps both ufbx allocators at max(MinimumMemoryLimit, MemoryLimitPerFileByte x the file size): beyond, ufbx
	 * fails cleanly (UFBX_ERROR_MEMORY_LIMIT) and the load is refused.
	 * @param options The ufbx options.
	 * @param sourceBytes The size of the FBX file (0 when unknown: the minimum applies).
	 */
	void setMemoryLimits (ufbx_load_opts & options, uint64_t sourceBytes) noexcept;

	/**
	 * @brief Fills the ufbx load options that bring a scene into the engine conventions (Y up, metres, axis conversion
	 * and geometry transforms baked into the vertices), with the memory limits of setMemoryLimits().
	 * @param options The ufbx options, zero-initialized by the caller.
	 * @param wholeScene True for a whole asset (external files loaded, missing normals generated), false for the
	 * animation clips only.
	 * @param sourceBytes The size of the FBX file.
	 */
	void setLoadOptions (ufbx_load_opts & options, bool wholeScene, uint64_t sourceBytes) noexcept;

	/**
	 * @brief Analyses a loaded FBX scene.
	 * @param scene The ufbx scene (loaded with setLoadOptions()).
	 * @param resourcePrefix The prefix of every resource name ("FBX:<stem>/").
	 * @param basePath The directory external textures are relative to.
	 * @param options The loader options (scale, skinning, excluded nodes).
	 * @return Asset
	 */
	[[nodiscard]]
	Asset analyse (const ufbx_scene & scene, const std::string & resourcePrefix, const std::filesystem::path & basePath, const LoaderOptions & options) noexcept;

	/**
	 * @brief Analyses the animation clips of an animation-only FBX against a target skeleton (the joints matched by name).
	 * @param scene The ufbx scene.
	 * @param skeleton The target skeleton.
	 * @param fileStem The clip names' base (the file stem).
	 * @param options The loader options (scale, root motion).
	 * @return std::vector< Clip > The clips (resource names "FBX:<stem>/Animation/<clip>"), empty when no joint name matches.
	 */
	[[nodiscard]]
	std::vector< Clip > analyseClipsOnly (const ufbx_scene & scene, const Base::Animation::Skeleton< float > & skeleton, const std::string & fileStem, const LoaderOptions & options) noexcept;
}
