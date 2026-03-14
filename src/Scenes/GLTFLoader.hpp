/*
 * src/Scenes/GLTFLoader.hpp
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
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Resources/Manager.hpp"

/* Forward declarations. */
namespace fastgltf
{
	class Asset;
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

namespace EmEn::Scenes
{
	class Node;
	class Scene;

	class GLTFLoader final
	{
		public:

			static constexpr auto ClassId{"GLTFLoader"};

			explicit
			GLTFLoader (Resources::Manager & resources) noexcept
				: m_resources{resources}
			{

			}

			/**
			 * @brief Sets a list of glTF node names to exclude from loading.
			 * @note Excluded nodes and their entire subtree are skipped during hierarchy building.
			 * @param names Set of glTF node names (as they appear in the file, not engine-prefixed).
			 */
			void setExcludedNodeNames (std::unordered_set< std::string > names) noexcept
			{
				m_excludedNodeNames = std::move(names);
			}

			/**
			 * @brief Loads a glTF/glb file into the scene.
			 * @param filepath Path to the .gltf or .glb file.
			 * @param scene Reference to the scene.
			 * @param useStaticEntities If true, mesh nodes become StaticEntity (flat, with frustum culling AABB).
			 *                          If false, all nodes are created in the Node tree under the scene root.
			 * @return bool
			 */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, Scene & scene, bool useStaticEntities = true) noexcept;

		private:

			[[nodiscard]]
			bool loadImages (const fastgltf::Asset & asset, const std::filesystem::path & basePath) noexcept;

			[[nodiscard]]
			bool loadTextures (const fastgltf::Asset & asset) noexcept;

			[[nodiscard]]
			bool loadMaterials (const fastgltf::Asset & asset) noexcept;

			[[nodiscard]]
			bool loadMeshes (const fastgltf::Asset & asset) noexcept;

			void buildNodeHierarchy (const fastgltf::Asset & asset, Scene & scene, const std::shared_ptr< Node > & parentNode) noexcept;

			void processNodeAsStatic (const fastgltf::Asset & asset, size_t nodeIndex, Scene & scene, const Libs::Math::CartesianFrame< float > & parentWorldFrame) noexcept;

			void processNodeAsNode (const fastgltf::Asset & asset, size_t nodeIndex, const std::shared_ptr< Node > & engineParent) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
			std::unordered_set< std::string > m_excludedNodeNames;
			std::vector< std::shared_ptr< Graphics::ImageResource > > m_images;
			std::vector< std::shared_ptr< Graphics::TextureResource::Texture2D > > m_textures;
			std::vector< std::shared_ptr< Graphics::Material::Interface > > m_materials;
			std::vector< std::shared_ptr< Graphics::Renderable::Abstract > > m_meshes;
			bool m_useStaticEntities{true};
	};
}
