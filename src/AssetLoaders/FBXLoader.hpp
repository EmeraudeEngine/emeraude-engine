/*
 * src/AssetLoaders/FBXLoader.hpp
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
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Libs/Animation/AnimationChannel.hpp"
#include "Libs/Animation/Skin.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/VertexFactory/Shape.hpp"
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

namespace EmEn::AssetLoaders
{
	/**
	 * @brief Loads FBX composite assets into engine resource containers.
	 * @note Produces an AssetData with format-agnostic node descriptors.
	 * Uses ufbx (vendored) for parsing. No dependency on Scenes/ types.
	 */
	class FBXLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"AssetLoaders::FBXLoader"};

			/**
			 * @brief Constructs the loader with access to the resource manager.
			 * @param resources A reference to the engine resource manager.
			 */
			explicit
			FBXLoader (Resources::Manager & resources) noexcept
				: m_resources{resources}
			{

			}

			/** @copydoc EmEn::AssetLoaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, AssetData & output) noexcept override;

			/** @copydoc EmEn::AssetLoaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".fbx";
			}

			/** @copydoc EmEn::AssetLoaders::Interface::loadAnimationClipsOnly() */
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
			bool loadMeshes (const ufbx_scene & scene, AssetData & output) noexcept;

			void loadSkins (const ufbx_scene & scene, AssetData & output) noexcept;

			void loadAnimations (const ufbx_scene & scene, AssetData & output) noexcept;

			void buildNodeDescriptors (const ufbx_scene & scene, AssetData & output) noexcept;

			/**
			 * @brief Resamples one anim_stack into per-joint T/R/S channels.
			 * @note jointToNode[i] is the ufbx_node driving joint i, or nullptr to skip
			 * the joint. Sample rate is 30 Hz (Mixamo canonical).
			 * @param stack The FBX animation stack to evaluate.
			 * @param jointToNode Per-joint ufbx_node mapping. Entries can be null.
			 * @return std::vector< Libs::Animation::AnimationChannel< float > >
			 */
			[[nodiscard]]
			static std::vector< Libs::Animation::AnimationChannel< float > >
			sampleAnimStack (const ufbx_anim_stack & stack, const std::vector< const ufbx_node * > & jointToNode) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
			std::vector< std::shared_ptr< Graphics::ImageResource > > m_images;
			std::vector< std::shared_ptr< Graphics::TextureResource::Texture2D > > m_textures;
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materials;
			/* Meshes are indexed by ufbx mesh_element_id (stable across the scene). */
			std::unordered_map< uint32_t, std::shared_ptr< Graphics::Renderable::Abstract > > m_meshes;
			std::unordered_map< uint32_t, std::shared_ptr< Libs::VertexFactory::Shape< float > > > m_shapes;
			/* Skeletal animation data. */
			std::vector< std::shared_ptr< Animations::SkeletonResource > > m_skeletons;
			std::vector< Libs::Animation::Skin< float > > m_skins;
			std::unordered_map< uint32_t, size_t > m_meshToSkinIndex;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_animationClips;
			std::unordered_set< size_t > m_skinJointNodeIndices;
	};
}

