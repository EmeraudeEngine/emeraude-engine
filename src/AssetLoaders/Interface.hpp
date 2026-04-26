/*
 * src/AssetLoaders/Interface.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

/* Forward declarations. */
namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::AssetLoaders
{
	struct AssetData;
	struct MeshDescriptor;
}

namespace EmEn::AssetLoaders
{
	/**
	 * @brief Options that affect resource loading (shared by all loaders).
	 * @note flattenHierarchy is NOT here — it only affects scene building,
	 * not resource loading, and belongs in Scenes::AssetDataConsumer.
	 */
	/**
	 * @brief Material container selected for the resources produced by a loader.
	 * @note Standard maps the FBX/glTF PBR factors (albedo, roughness, metalness)
	 * onto a Phong/Blinn surface via the cross-material setters of StandardResource.
	 */
	enum class MaterialMode : uint8_t
	{
		PBR,
		Standard
	};

	struct LoaderOptions
	{
		std::unordered_set< std::string > excludedNodeNames;
		/**
		 * @brief Optional per-mesh hook invoked right after a mesh's renderable, geometry
		 * and materials have been registered. Lets the caller patch the descriptor in place
		 * (e.g. enable IBL reflection on PBR materials, override geometry, swap a renderable).
		 * Called once per loaded mesh, in load order, before nodes are wired.
		 */
		std::function< void (MeshDescriptor &) > onMeshLoaded;
		MaterialMode materialMode{MaterialMode::PBR};
		bool skipSkinning{false};
		/**
		 * @brief Strips the translation track of every root joint from animation
		 * clips produced by `loadAnimationClipsOnly`. Rotation and scale tracks
		 * are kept intact, and non-root joints are not touched.
		 *
		 * Mixamo (and many other DCC) per-action FBX clips bake forward
		 * locomotion into the root bone — the model physically translates
		 * meters during the clip. When the actor's displacement is also driven
		 * by gameplay code (physics force, navmesh, etc.), the two motions
		 * stack and the model snaps backward at every loop boundary. Enabling
		 * this flag turns Mixamo locomotion clips into "in-place" clips at
		 * load time without re-exporting from the DCC.
		 *
		 * @note Has no effect on `load()` (full-pipeline import) — only on
		 * `loadAnimationClipsOnly()`. Tracked TODO: a future "root-motion
		 * mode" will instead extract the root delta and feed it to the actor
		 * as actual displacement (foot-planting, no sliding) — see
		 * `dependencies/emeraude-engine/docs/` and the engine TODO list.
		 */
		bool stripRootMotion{false};
		/**
		 * @brief Uniform scale applied at load time, coherently across the
		 * full skinned-mesh pipeline: vertex positions, joint local
		 * translations, inverse bind matrix translation columns, and
		 * animation translation keyframes (both in `load()` embedded clips
		 * and `loadAnimationClipsOnly()` external clips). Rotations and
		 * scales of joint TRS, plus the per-vertex influence weights, are
		 * never touched.
		 *
		 * The scale must be passed identically to BOTH the rig load and
		 * every subsequent `loadAnimationClipsOnly()` call against that rig
		 * — otherwise animation keyframes would describe translations in a
		 * different unit than the scaled bind pose, and joints would snap
		 * to wrong positions at every keyframe (visual: the rig would
		 * collapse on the first animated frame).
		 *
		 * Default `1.0F` is a no-op. Set `< 1.0` to shrink, `> 1.0` to
		 * enlarge. Also propagates to the bounding box of the produced
		 * renderables, so collision shapes derived from the bbox reflect
		 * the scaled size automatically.
		 */
		float uniformScale{1.0F};
	};

	/**
	 * @brief Common interface for composite asset format loaders.
	 * @note Implementations load resources into engine containers and produce
	 * a format-agnostic AssetData describing the node hierarchy.
	 * No dependency on Scenes/ types.
	 */
	class Interface
	{
		public:

			virtual ~Interface () = default;

			/**
			 * @brief Sets loader options.
			 * @param options The options to apply.
			 */
			void
			setOptions (LoaderOptions options) noexcept
			{
				m_options = std::move(options);
			}

			/**
			 * @brief Loads a composite asset from a file.
			 * @param filepath Path to the asset file.
			 * @param output The AssetData to populate with loaded resources and hierarchy.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool load (const std::filesystem::path & filepath, AssetData & output) noexcept = 0;

			/**
			 * @brief Checks if this loader supports the given file extension.
			 * @param extension The file extension (e.g., ".gltf", ".glb").
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool supportsExtension (std::string_view extension) const noexcept = 0;

			/**
			 * @brief Loads animation clips from an asset file and binds them to an existing skeleton.
			 * @note Used to attach motion data exported as standalone files (Mixamo split animations,
			 * Maya/Blender per-action exports). Bones are resolved by name against @a targetSkeleton —
			 * channels referencing missing joints are silently dropped.
			 * @note The default implementation returns false. Loaders must override to opt in.
			 * @param filepath Path to the animation-only asset file.
			 * @param targetSkeleton The skeleton whose joint names drive bone resolution.
			 * @param output Vector to which the produced clip resources are appended.
			 * @return bool True if at least one clip was produced.
			 */
			[[nodiscard]]
			virtual
			bool
			loadAnimationClipsOnly (
				const std::filesystem::path & /*filepath*/,
				const Animations::SkeletonResource & /*targetSkeleton*/,
				std::vector< std::shared_ptr< Animations::AnimationClipResource > > & /*output*/
			) noexcept
			{
				return false;
			}

		protected:

			Interface () noexcept = default;
			Interface (const Interface &) = default;
			Interface (Interface &&) = default;
			Interface & operator= (const Interface &) = default;
			Interface & operator= (Interface &&) = default;

			LoaderOptions m_options;
	};
}
