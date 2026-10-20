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
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureResource/StoreTexture.hpp"

/* Forward declarations. */
namespace fastgltf
{
	class Asset;
}

namespace EmEn::Scenes::Loaders::GLTF
{
	struct Asset;
}

namespace EmEn::Scenes::Loaders
{
	struct SceneData;
}

namespace EmEn::Resources
{
	class Manager;
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
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			GLTFLoader (const GLTFLoader & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			GLTFLoader (GLTFLoader && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return GLTFLoader &
			 */
			GLTFLoader & operator= (const GLTFLoader & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return GLTFLoader &
			 */
			GLTFLoader & operator= (GLTFLoader && copy) noexcept = delete;

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
			bool loadImages (GLTF::Asset & analysed, const std::filesystem::path & filepath, const std::filesystem::path & basePath) noexcept;

			[[nodiscard]]
			bool loadMaterials (const fastgltf::Asset & asset) noexcept;

			/**
			 * @brief Materializes the analysed meshes: geometries, the per-primitive materials (the vertex-colour and
			 * primary-set-only variants included), meshes and descriptors.
			 * @param analysed The analysed asset (GLTF::analyse()).
			 * @param output A reference to the scene data to populate.
			 * @return bool False when a mesh fell back to the default one.
			 */
			[[nodiscard]]
			bool buildMeshes (const GLTF::Asset & analysed, SceneData & output) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
			/* Images, indexed by glTF image index. An image lands in exactly one of the two tables :
			 * m_compressedImages for a KTX2 payload (KHR_texture_basisu) kept block-compressed all the
			 * way to the GPU, m_images for anything the engine has to decode to pixels. */
			std::vector< std::shared_ptr< Graphics::ImageResource > > m_images;
			std::vector< std::shared_ptr< Graphics::CompressedImageResource > > m_compressedImages;
			/* ⚠️ THREE slots per glTF texture, indexed by usage (TextureResource::loaderUsageSlot(): linear data, sRGB,
			 * normal map). The colour space and the normal map kind are baked into the resource when it is created and
			 * come from the USAGE, not from the asset: one image may serve as an sRGB albedo for one material and as a
			 * linear roughness map for another. A cache keyed on the texture index alone let the first usage impose its
			 * identity on every other one. */
			std::vector< std::array< std::shared_ptr< Graphics::TextureResource::Texture2D >, Graphics::TextureResource::LoaderUsageCount > > m_textures;
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
			/* Whether each glTF material samples the secondary texture coordinates (TEXCOORD_1) with a map, and its
			 * `…-uv0` variant: every map back on TEXCOORD_0, for a primitive that lacks TEXCOORD_1 (an invalid asset —
			 * glTF requires the set a material samples). Without it that primitive's shader would read an attribute its
			 * mesh may not carry (the presence wall: docs/todo/vertex-attribute-presence-belongs-to-geometry.md). */
			std::vector< uint8_t > m_materialSamplesSecondarySet;
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materialsPrimarySetOnly;
			std::vector< std::shared_ptr< Graphics::Renderable::Abstract > > m_meshes;
			/* Skeletal animation data — indexed by glTF skin index. */
			std::vector< std::shared_ptr< Animations::SkeletonResource > > m_skeletons;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_animationClips;
			std::vector< std::shared_ptr< Animations::AnimationClipResource > > m_nodeAnimationClips;
	};
}
