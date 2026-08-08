/*
 * src/SceneLoaders/USDLoader.hpp
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
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Forward declarations. */
namespace tinyusdz
{
	class Stage;
	class Prim;

	namespace tydra
	{
		struct RenderScene;
	}
}

namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::Graphics::Material
{
	class Interface;
}

namespace EmEn::SceneLoaders
{
	/**
	 * @brief OpenUSD scene loader, backed by tinyusdz (USDA, USDC crate, USDZ).
	 * @note Composition is resolved at load time and NOTHING of USD survives it: the stage is
	 * translated into native engine scene logic and dropped. Where the scene layer cannot
	 * express a USD concept, the capability is added to Scenes — never a USD construct kept
	 * alive, never a workaround here. See docs/scene-loaders-usd.md.
	 * @extends EmEn::SceneLoaders::Interface
	 */
	class EMEN_API USDLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"SceneLoaders::USDLoader"};

			/**
			 * @brief Constructs an OpenUSD loader.
			 * @param resources A reference to the resource manager.
			 */
			explicit USDLoader (Resources::Manager & resources) noexcept;

			/** @copydoc EmEn::SceneLoaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::SceneLoaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".usd" || extension == ".usda" || extension == ".usdc" || extension == ".usdz";
			}

			/**
			 * @copydoc EmEn::SceneLoaders::Interface::capabilities()
			 * @note Deliberately None: this loader currently produces a STAGE INVENTORY and no
			 * scene data at all. The mask states what is delivered, so it grows only as each
			 * translation milestone lands — geometry, then materials, then instancing, then
			 * lights and cameras.
			 */
			[[nodiscard]]
			uint32_t
			capabilities () const noexcept override
			{
				return None;
			}

		private:

			/**
			 * @brief Counters gathered while walking a composed stage.
			 */
			struct Inventory
			{
				std::map< std::string, size_t > primTypeCounts;
				size_t primCount{0};
				size_t maxDepth{0};
				size_t pointInstancerCount{0};
				size_t materialCount{0};
				size_t meshCount{0};
			};

			/**
			 * @brief Translates the render scene's materials into engine PBR materials.
			 * @note UsdPreviewSurface IS a metallic-roughness model, so the mapping is term for
			 * term. Texture paths are resolved HERE, relative to the stage: tinyusdz refuses any
			 * asset path containing "..", which every one of this asset's textures uses, so its
			 * own image loading never runs — only the paths survive, and the engine reads them.
			 * @param renderScene A reference to the Tydra render scene.
			 * @param stageDirectory The directory the stage was read from.
			 * @return std::vector< std::shared_ptr< Graphics::Material::Interface > > Indexed by RenderMaterial index.
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< Graphics::Material::Interface > > buildMaterials (const tinyusdz::tydra::RenderScene & renderScene, const std::filesystem::path & stageDirectory) noexcept;

			/**
			 * @brief Translates the render scene's meshes into engine resources and descriptors.
			 * @note Axis and unit conversion is BAKED here, into positions, normals and tangents
			 * alike — the engine then sees nothing but its own convention.
			 * @param renderScene A reference to the Tydra render scene.
			 * @param metersPerUnit The stage's linear unit.
			 * @param output A reference to the scene data to populate.
			 * @return size_t The number of meshes actually built.
			 */
			[[nodiscard]]
			size_t buildMeshes (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output) noexcept;

			/**
			 * @brief Walks a prim subtree and collects environment (dome) lights.
			 * @note Tydra's RenderLight does not carry the dome's image path, so the prim is read
			 * directly. Dropping it silently loses the asset's own sky.
			 * @param prim A reference to the prim to visit.
			 * @param stageDirectory The directory the stage was read from.
			 * @param output A reference to the scene data to populate.
			 */
			static void collectEnvironmentLights (const tinyusdz::Prim & prim, const std::filesystem::path & stageDirectory, SceneData & output) noexcept;

			/**
			 * @brief Walks a prim subtree and accumulates the inventory.
			 * @param prim A reference to the prim to visit.
			 * @param depth The current depth in the hierarchy.
			 * @param inventory A reference to the inventory being filled.
			 */
			static void collectInventory (const tinyusdz::Prim & prim, size_t depth, Inventory & inventory) noexcept;

			/**
			 * @brief Reports what the composed stage actually contains.
			 * @note This is the loader's FIRST functional output on purpose. Without it, "the
			 * scene is empty" cannot be told from "the asset uses a construct we ignore", and
			 * every diagnosis starts blind. Byte-scanning a USDC crate cannot answer it either:
			 * crate files compress their token table.
			 * @param filepath The file the stage came from.
			 * @param stage A reference to the composed stage.
			 */
			static void reportInventory (const std::filesystem::path & filepath, const tinyusdz::Stage & stage) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;
	};
}
