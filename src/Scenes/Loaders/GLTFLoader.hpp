/*
 * src/Scenes/Loaders/GLTFLoader.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
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
#include "VertexFactory/Shape.hpp"
#include "Resources/Manager.hpp"

/* Forward declarations. */
namespace fastgltf
{
	class Asset;
}

namespace EmEn::Animations
{
	class SkeletonResource;
	class AnimationClipResource;
}

namespace EmEn::Graphics
{
	class CompressedImageResource;
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
	/* Decoder for EXT_meshopt_compression buffer views. Defined in GLTFLoader.cpp : its interface
	 * speaks fastgltf types, which must not leak into a public engine header. */
	class MeshoptBufferCache;

	/**
	 * @brief Loads glTF/glb composite assets into engine resource containers.
	 * @note Produces an SceneData with format-agnostic node descriptors.
	 * No dependency on Scenes/ types (Scene, Node, StaticEntity).
	 */
	class EMEN_API GLTFLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"GLTFLoader"};

			/**
			 * @brief Constructs the loader with access to the resource manager.
			 * @param resources A reference to the engine resource manager.
			 */
			explicit GLTFLoader (Resources::Manager & resources) noexcept;

			/**
			 * @brief Destructs the loader.
			 * @note Both the constructor and the destructor are defined out of line : m_bufferCache
			 * points at an incomplete type here, and an inlined constructor would already need the
			 * deleter to be complete.
			 */
			~GLTFLoader () override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".gltf" || extension == ".glb";
			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::capabilities() */
			[[nodiscard]]
			uint32_t
			capabilities () const noexcept override
			{
				return Geometry | Skinning | Animations | Lights | Cameras;
			}

		private:

			[[nodiscard]]
			bool loadImages (const fastgltf::Asset & asset, const std::filesystem::path & basePath) noexcept;

			[[nodiscard]]
			bool loadMaterials (const fastgltf::Asset & asset) noexcept;

			[[nodiscard]]
			bool loadMeshes (const fastgltf::Asset & asset, SceneData & output) noexcept;

			void loadSkins (const fastgltf::Asset & asset, SceneData & output) noexcept;

			void loadAnimations (const fastgltf::Asset & asset, SceneData & output) noexcept;

			/**
			 * @brief Turns a finished channel set into a clip resource and appends it to a list.
			 * @param clipName The clip's own name, which the animators index on.
			 * @param keySpace The resource key space telling the two halves of a split animation
			 * apart ("/animation/" for the skeletal half, "/node-animation/" for the node half).
			 * @param channels The channels, consumed.
			 * @param output A reference to the list to append to.
			 * @return void
			 */
			void registerClip (const std::string & clipName, const std::string & keySpace, std::vector< Base::Animation::AnimationChannel< float > > channels, std::vector< std::shared_ptr< Animations::AnimationClipResource > > & output) const noexcept;

			/**
			 * @brief Collects the punctual lights declared by the asset, in photometric units.
			 * @note Must run BEFORE buildNodeDescriptors(), which indexes into output.lights.
			 * @param asset A reference to the parsed glTF asset.
			 * @param output A reference to the scene data to populate.
			 */
			static void loadLights (const fastgltf::Asset & asset, SceneData & output) noexcept;

			/**
			 * @brief Collects the authored camera viewpoints declared by the asset.
			 * @note Must run BEFORE buildNodeDescriptors(), which indexes into output.cameras.
			 * @param asset A reference to the parsed glTF asset.
			 * @param output A reference to the scene data to populate.
			 */
			static void loadCameras (const fastgltf::Asset & asset, SceneData & output) noexcept;

			void buildNodeDescriptors (const fastgltf::Asset & asset, SceneData & output) const noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
			/* EXT_meshopt_compression working set. Built at the start of a load, released as soon
			 * as the geometry is out : on a compressed scene it is the load's memory high-water mark. */
			std::unique_ptr< MeshoptBufferCache > m_bufferCache;
			/* Images, indexed by glTF image index. An image lands in exactly one of the two tables :
			 * m_compressedImages for a KTX2 payload (KHR_texture_basisu) kept block-compressed all the
			 * way to the GPU, m_images for anything the engine has to decode to pixels. */
			std::vector< std::shared_ptr< Graphics::ImageResource > > m_images;
			std::vector< std::shared_ptr< Graphics::CompressedImageResource > > m_compressedImages;
			/* ⚠️ TWO slots per glTF texture, indexed by colour space (0 = linear data,
			 * 1 = sRGB). The sRGB flag is baked into the resource when it is created and comes
			 * from the USAGE, not from the asset: one image may serve as an sRGB albedo for one
			 * material and as a linear roughness map for another. A cache keyed on the texture
			 * index alone let the first usage impose its colour space on every other one. */
			std::vector< std::array< std::shared_ptr< Graphics::TextureResource::Texture2D >, 2 > > m_textures;
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materials;
			/* ⚠️ The VERTEX-COLOUR VARIANT of each material, non-null only for the materials a
			 * COLOR_0 primitive actually uses. `UseVertexColors` changes the material's SHADER
			 * CONTRACT — the vertex shader declares a vertex input attribute for it — so a material
			 * that modulates by vertex colour is a DIFFERENT resource, keyed `…-vc`. glTF makes
			 * this unavoidable: COLOR_0 is per-primitive while the material is shared, and on the
			 * reference Sponza 17 of 22 colour-using materials are ALSO used by colourless
			 * primitives. Enabling the flag on the shared resource would make the shader read an
			 * attribute those primitives' geometry does not provide. */
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materialsVertexColor;
			std::vector< std::shared_ptr< Graphics::Renderable::Abstract > > m_meshes;
			std::vector< std::shared_ptr< Base::VertexFactory::Shape< float > > > m_shapes;
			/* Skeletal animation data — indexed by glTF skin index. */
			std::vector< std::shared_ptr< Animations::SkeletonResource > > m_skeletons;
			std::vector< Base::Animation::Skin< float > > m_skins;
			std::unordered_map< size_t, size_t > m_meshToSkinIndex;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_animationClips;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_nodeAnimationClips;
			std::unordered_set< size_t > m_skinJointNodeIndices;
	};
}
