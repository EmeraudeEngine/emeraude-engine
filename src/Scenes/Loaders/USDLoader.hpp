/*
 * src/Scenes/Loaders/USDLoader.hpp
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

/* Local inclusions for usages. */
#include "VertexFactory/Shape.hpp"

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Forward declarations. */
namespace EmEn::Scenes::Loaders::USD
{
	struct Asset;
	struct Instancer;
	struct Material;
}

namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::Graphics::Geometry
{
	class Interface;
}

namespace EmEn::Graphics::Material
{
	class Interface;
}

namespace EmEn::Graphics::TextureResource
{
	class Abstract;
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief A memory-mapped USDZ archive and its asset table, defined in USDZArchive.hpp.
	 * @note Held through a shared pointer on purpose: the image resource factories run on the
	 * THREAD POOL and outlive load(), so the mapping must be owned by whoever still needs it.
	 * See USDZArchive.cpp for the resolution rules — they carry the traps.
	 */
	class USDZArchive;

	/**
	 * @brief OpenUSD scene loader, backed by tinyusdz — LightUSD since v1.0.0-rc4 (USDA, USDC crate, USDZ).
	 * @note Composition is resolved at load time and NOTHING of USD survives it: the stage is
	 * translated into native engine scene logic and dropped. Where the scene layer cannot
	 * express a USD concept, the capability is added to Scenes — never a USD construct kept
	 * alive, never a workaround here. See docs/scene-loaders-usd.md.
	 * @note The translation itself is the USD:: analysis (USDParser.hpp, fuzzed): this class reads the root layer, has it
	 * composed and analysed, then materializes the result.
	 * @extends EmEn::Scenes::Loaders::Interface
	 */
	class EMEN_API USDLoader final : public Interface
	{
		public:

			static constexpr auto ClassId{"USDLoader"};

			/**
			 * @brief Constructs an OpenUSD loader.
			 * @param resources A reference to the resource manager.
			 */
			explicit USDLoader (Resources::Manager & resources) noexcept;

			/** @copydoc EmEn::Scenes::Loaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".usd" || extension == ".usda" || extension == ".usdc" || extension == ".usdz";
			}

			/**
			 * @copydoc EmEn::Scenes::Loaders::Interface::capabilities()
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
			 * @brief Retries a missing path by matching its filename WITHOUT case.
			 * @note Assets authored on Windows or macOS carry whatever spelling the DCC recorded, on
			 * a filesystem that does not care. On Linux the file is simply not found, and a missing
			 * base colour beside a resolved normal map renders as lit, detailed, pure WHITE geometry —
			 * a failure that looks like a material bug and never points at a filename.
			 * @param wanted The path that failed to resolve.
			 * @return std::filesystem::path Empty when nothing matches.
			 */
			[[nodiscard]]
			static std::filesystem::path findCaseInsensitive (const std::filesystem::path & wanted) noexcept;

			/**
			 * @brief Decodes one image out of the mapped archive into an engine texture.
			 * @note Reads the archive bytes IN PLACE — no extraction to disk, no intermediate copy.
			 * @param assetIdentifier The asset path Tydra reported for the image.
			 * @param sRGB Whether the texture holds colour rather than data.
			 * @return std::shared_ptr< Graphics::TextureResource::Abstract > Null when unusable.
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::TextureResource::Abstract > archiveTexture (const std::string & assetIdentifier, bool sRGB) noexcept;

			/**
			 * @brief Turns a texture asset identifier into an engine texture: out of the archive, or next to the stage
			 * (a case-insensitive retry included).
			 * @param assetIdentifier The asset path Tydra reported for the image (empty: none).
			 * @param sRGB Whether the texture holds colour rather than data.
			 * @param stageDirectory The directory the stage was read from.
			 * @return std::shared_ptr< Graphics::TextureResource::Abstract > Null when unusable.
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::TextureResource::Abstract > resolveTexture (const std::string & assetIdentifier, bool sRGB, const std::filesystem::path & stageDirectory) noexcept;

			/**
			 * @brief Translates the render scene's materials into engine PBR materials.
			 * @note UsdPreviewSurface IS a metallic-roughness model, so the mapping is term for
			 * term. Texture paths are resolved HERE, relative to the stage: tinyusdz refuses any
			 * asset path containing "..", which every one of this asset's textures uses, so its
			 * own image loading never runs — only the paths survive, and the engine reads them.
			 * @param descriptions The analysed materials (USD::analyse()).
			 * @param stageDirectory The directory the stage was read from.
			 * @return std::vector< std::shared_ptr< Graphics::Material::Interface > > Indexed by RenderMaterial index.
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< Graphics::Material::Interface > > buildMaterials (const std::vector< USD::Material > & descriptions, const std::filesystem::path & stageDirectory) noexcept;

			/**
			 * @brief Builds the coarser levels of a heavy mesh (LoaderOptions::generateLevelsOfDetail).
			 * @note Synchronous: called before the finest geometry's factory, which writes the same shape's tangents.
			 * @param finest The finest shape.
			 * @param resourceName The finest geometry's resource name; the levels append "-lod<n>".
			 * @return std::vector< std::shared_ptr< Graphics::Geometry::Interface > > The coarser levels, coarsest last;
			 * empty when nothing worth a level came out.
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< Graphics::Geometry::Interface > > buildLevelsOfDetail (const Base::VertexFactory::Shape< float, uint32_t > & finest, const std::string & resourceName) const noexcept;

			/**
			 * @brief Materializes the analysed meshes, one at a time as USD::forEachMesh() builds them: geometries
			 * (deduplicated by content, with their levels of detail), meshes, descriptors and nodes.
			 * @param asset The analysed stage.
			 * @param materials The translated materials, indexed by RenderMaterial index.
			 * @param output A reference to the scene data to populate.
			 * @param builtMeshesByPath Receives, for every mesh built, its USD prim path.
			 * @return size_t The number of meshes actually built.
			 */
			[[nodiscard]]
			size_t buildMeshes (const USD::Asset & asset, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output, std::map< std::string, size_t > & builtMeshesByPath) noexcept;

			/**
			 * @brief Turns the collected instancers into instance sets referencing built meshes.
			 * @param instancers The instancers read from the stage.
			 * @param builtMeshesByPath Every mesh built, by USD prim path.
			 * @param output A reference to the scene data to populate.
			 * @return size_t The total number of instances declared.
			 */
			static size_t buildInstanceSets (const std::vector< USD::Instancer > & instancers, const std::map< std::string, size_t > & builtMeshesByPath, SceneData & output) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;

			/* Non-null only while a USDZ is being loaded, and for as long as an image factory still
			 * holds a copy of it. */
			std::shared_ptr< USDZArchive > m_archive;
	};
}
