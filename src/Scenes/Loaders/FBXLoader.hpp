/*
 * src/Scenes/Loaders/FBXLoader.hpp
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
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Animation/AnimationChannel.hpp"
#include "Animation/Skin.hpp"
#include "Math/CartesianFrame.hpp"
#include "VertexFactory/Shape.hpp"
#include "Resources/Manager.hpp"

/* Forward declarations. */
struct ufbx_scene;
struct ufbx_node;
struct ufbx_anim_stack;

namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Graphics
{
	class ImageResource;

	namespace TextureResource
	{
		class Texture2D;
	}

	namespace Material
	{
		class Interface;
	}

	namespace Renderable
	{
		class Abstract;
	}
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief Loads FBX composite assets into engine resource containers.
	 * @note Produces an SceneData with format-agnostic node descriptors.
	 * Uses ufbx (vendored) for parsing. No dependency on Scenes/ types.
	 */
	class EMEN_API FBXLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"FBXLoader"};

			/**
			 * @brief Constructs the loader with access to the resource manager.
			 * @param resources A reference to the engine resource manager.
			 */
			explicit
			FBXLoader (Resources::Manager & resources) noexcept
				: m_resources{resources}
			{

			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".fbx";
			}

			/**
			 * @copydoc EmEn::Scenes::Loaders::Interface::capabilities()
			 * @note FBX carries lights and cameras; this loader does not read them yet. The mask
			 * states what is DELIVERED, so they stay out until the code exists.
			 */
			[[nodiscard]]
			uint32_t
			capabilities () const noexcept override
			{
				return Geometry | Skinning | Animations;
			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::loadAnimationClipsOnly() */
			[[nodiscard]]
			bool loadAnimationClipsOnly (
				const std::filesystem::path & filepath,
				const Animations::SkeletonResource & targetSkeleton,
				std::vector< std::shared_ptr< Animations::AnimationClipResource > > & output
			) noexcept override;

		private:

			[[nodiscard]]
			bool loadImages (const ufbx_scene & scene, const std::filesystem::path & basePath) noexcept;

			[[nodiscard]]
			bool loadMaterials (const ufbx_scene & scene) noexcept;

			[[nodiscard]]
			bool loadMeshes (const ufbx_scene & scene, SceneData & output) noexcept;

			void loadSkins (const ufbx_scene & scene, SceneData & output) noexcept;

			void loadAnimations (const ufbx_scene & scene, SceneData & output) noexcept;

			void buildNodeDescriptors (const ufbx_scene & scene, SceneData & output) noexcept;

			/**
			 * @brief Resamples one anim_stack into per-joint T/R/S channels.
			 * @note jointToNode[i] is the ufbx_node driving joint i, or nullptr to skip
			 * the joint. Sample rate is 30 Hz (Mixamo canonical).
			 * @param stack The FBX animation stack to evaluate.
			 * @param jointToNode Per-joint ufbx_node mapping. Entries can be null.
			 * @param uniformScale The uniform scale applied to translation keyframes.
			 * @param axisFlip The axis negation the whole import is routed through: translation
			 * keyframes are mirrored, rotation keyframes CONJUGATED, scale keyframes untouched.
			 * ⚠️ It must be the very same flip the bind pose was built with, or the rig snaps to the
			 * unmirrored pose on the first animated frame.
			 * @return std::vector< Base::Animation::AnimationChannel< float > >
			 */
			[[nodiscard]]
			static std::vector< Base::Animation::AnimationChannel< float > > sampleAnimStack (const ufbx_anim_stack & stack, const std::vector< const ufbx_node * > & jointToNode, float uniformScale, const AxisFlip & axisFlip) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
			std::vector< std::shared_ptr< Graphics::ImageResource > > m_images;
			std::vector< std::shared_ptr< Graphics::TextureResource::Texture2D > > m_textures;
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materials;
			/* Meshes are indexed by ufbx mesh_element_id (stable across the scene). */
			std::unordered_map< uint32_t, std::shared_ptr< Graphics::Renderable::Abstract > > m_meshes;
			std::unordered_map< uint32_t, std::shared_ptr< Base::VertexFactory::Shape< float > > > m_shapes;
			/* Skeletal animation data. */
			std::vector< std::shared_ptr< Animations::SkeletonResource > > m_skeletons;
			std::vector< Base::Animation::Skin< float > > m_skins;
			std::unordered_map< uint32_t, size_t > m_meshToSkinIndex;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_animationClips;
			/* Bone nodes collected by loadSkins(). They are stored as ufbx POINTERS, never as
			 * element ids: SceneData::skinJointNodeIndices is consumed as an index into
			 * SceneData::nodes, which buildNodeDescriptors() COMPACTS (the root and every
			 * excluded subtree are skipped). The two numbering schemes do not coincide, and
			 * feeding an element id to the consumer makes it drop an unrelated node together
			 * with its whole subtree. buildNodeDescriptors() resolves these pointers to real
			 * node indices once the compaction is known. */
			std::unordered_set< const ufbx_node * > m_skinJointNodes;
			std::unordered_set< size_t > m_skinJointNodeIndices;
	};
}
