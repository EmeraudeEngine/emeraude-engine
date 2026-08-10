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
#include "Math/CartesianFrame.hpp"

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

namespace EmEn::Graphics::TextureResource
{
	class Abstract;
}

namespace EmEn::Scenes::Loaders
{
	/**
	 * @brief A memory-mapped USDZ archive and its asset table, defined in USDLoader.cpp.
	 * @note Held through a shared pointer on purpose: the image resource factories run on the
	 * THREAD POOL and outlive load(), so the mapping must be owned by whoever still needs it.
	 * See USDLoader.cpp for the resolution rules — they carry the traps.
	 */
	class USDZArchive;

	/**
	 * @brief OpenUSD scene loader, backed by tinyusdz (USDA, USDC crate, USDZ).
	 * @note Composition is resolved at load time and NOTHING of USD survives it: the stage is
	 * translated into native engine scene logic and dropped. Where the scene layer cannot
	 * express a USD concept, the capability is added to Scenes — never a USD construct kept
	 * alive, never a workaround here. See docs/scene-loaders-usd.md.
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

			[[nodiscard]]
			std::vector< std::shared_ptr< Graphics::Material::Interface > > buildMaterials (const tinyusdz::tydra::RenderScene & renderScene, const std::filesystem::path & stageDirectory) noexcept;

			/**
			 * @brief Translates the render scene's meshes into engine resources and descriptors.
			 * @note Axis and unit conversion is BAKED here, into positions, normals and tangents
			 * alike — the engine then sees nothing but its own convention.
			 * @param renderScene A reference to the Tydra render scene.
			 * @param metersPerUnit The stage's linear unit.
			 * @param prototypePaths The prim paths whose meshes are instanced rather than drawn.
			 * @param materials The translated materials, indexed by RenderMaterial index.
			 * @param output A reference to the scene data to populate.
			 * @param builtMeshesByPath Receives, for every mesh built, its USD prim path.
			 * @return size_t The number of meshes actually built.
			 */
			[[nodiscard]]
			size_t buildMeshes (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, const std::vector< std::string > & prototypePaths, const std::vector< std::shared_ptr< Graphics::Material::Interface > > & materials, SceneData & output, std::map< std::string, size_t > & builtMeshesByPath) noexcept;

			/**
			 * @brief Describes one PointInstancer read straight from the stage.
			 * @note Tydra knows NOTHING about PointInstancer — zero occurrence in its whole
			 * source — so this is read from the prims, not from the render scene. It is also why
			 * the prototypes must be kept off the ordinary draw path by hand: Tydra happily
			 * converts them into meshes like any other, and they would then be drawn once more,
			 * alone, wherever the asset happens to store them.
			 */
			struct Instancer
			{
				std::string path;
				std::vector< std::string > prototypePaths;
				std::vector< Base::Math::CartesianFrame< float > > instances;
				std::vector< int32_t > prototypeIndices;
			};

			/**
			 * @brief Walks a prim subtree and reads every PointInstancer it holds.
			 * @note Positions, orientations and scales are converted into the engine's own space
			 * HERE, by the same bake the vertices go through, so a set of instances and the mesh
			 * it instances always agree.
			 * @param prim A reference to the prim to visit.
			 * @param primPath The absolute path of that prim.
			 * @param metersPerUnit The stage's linear unit.
			 * @param instancers The list being filled.
			 */
			static void collectInstancers (const tinyusdz::Prim & prim, const std::string & primPath, float metersPerUnit, std::vector< Instancer > & instancers) noexcept;

			/**
			 * @brief Turns the collected instancers into instance sets referencing built meshes.
			 * @param instancers The instancers read from the stage.
			 * @param builtMeshesByPath Every mesh built, by USD prim path.
			 * @param output A reference to the scene data to populate.
			 * @return size_t The total number of instances declared.
			 */
			static size_t buildInstanceSets (const std::vector< Instancer > & instancers, const std::map< std::string, size_t > & builtMeshesByPath, SceneData & output) noexcept;

			/**
			 * @brief Translates the render scene's punctual lights into engine light descriptors.
			 *
			 * @note ⚠️ THE PHOTOMETRIC ANCHOR LIVES HERE, and it is a CALIBRATION, not a formula.
			 * USD's `inputs:intensity` is DIMENSIONLESS while the engine works in candela, so a
			 * conversion factor has to be chosen. The one in force (owner decision, 2026-08-10)
			 * reads `intensity` as a LUMINANCE in cd/m², multiplies by the emitter's AREA — which
			 * is what `normalize = false` means — and normalizes by 4π:
			 *
			 *     candela = intensity * 2^exposure * area / (4 * pi)
			 *
			 * Measured on the World Lobby's 25 ceiling DiskLights (intensity 60000, radius 0.5 m,
			 * 4 m above the floor): 3751 cd, so 234 lux at the floor — the real range of a building
			 * lobby (200-500 lux). The two rejected readings gave 2945 and 3750 lux, outdoor levels.
			 *
			 * @warning ⚠️ An emitter's AREA is part of the conversion, so a light's radius is NOT
			 * decoration: two fixtures of the same intensity and different size do not light alike.
			 *
			 * @note Dome lights are deliberately skipped — they carry an image and no position, and
			 * are collected by collectEnvironmentLights() as `LightType::Environment`.
			 *
			 * @param renderScene A reference to the Tydra render scene.
			 * @param metersPerUnit The stage's linear unit.
			 * @param output A reference to the scene data to populate.
			 * @return size_t The number of lights translated.
			 */
			[[nodiscard]]
			static size_t buildLights (const tinyusdz::tydra::RenderScene & renderScene, float metersPerUnit, SceneData & output) noexcept;

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

			/**
			 * @brief Prints the prim tree, path and type, bounded.
			 * @note A type histogram says WHAT a stage contains; it cannot say where, nor under
			 * what. When a prim is expected and missing, the tree is the only thing that tells
			 * "it was dropped" from "it is there under another type".
			 * @param prim A reference to the prim to visit.
			 * @param depth The current depth.
			 * @param remaining Lines left in the budget, decremented as they are printed.
			 */
			static void reportPrimTree (const tinyusdz::Prim & prim, size_t depth, size_t & remaining) noexcept;

			Resources::Manager & m_resources;
			std::string m_resourcePrefix;

			/* Non-null only while a USDZ is being loaded, and for as long as an image factory still
			 * holds a copy of it. */
			std::shared_ptr< USDZArchive > m_archive;
	};
}
