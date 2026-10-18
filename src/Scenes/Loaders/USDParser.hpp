/*
 * src/Scenes/Loaders/USDParser.hpp
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
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "VertexFactory/Shape.hpp"
#include "SceneData.hpp"

/* Forward declarations (lightusd stays out of the public headers). */
namespace lightusd
{
	class Stage;
	class Layer;
	class AssetResolutionResolver;
	struct USDLoadOptions;

	namespace tydra
	{
		class RenderScene;
	}
}

/**
 * @brief The analysis half of the USD loader: a composed stage becomes plain data — its material factors and texture
 * asset identifiers, its PointInstancers, its lights and their nodes, then one baked Shape per placed mesh — with no
 * engine resource, service or device.
 * @note Split from USDLoader on 2026-10-18 (owner decision: analysis / materialization) so the composition and the whole
 * translation can be fuzzed (src/Fuzzing/fuzz_usd.cpp). The loader reads the root layer (a file, or a USDZ archive through
 * its resolver), composes it here, then MATERIALIZES the result: texture resolution and decoding, deduplication, levels of
 * detail, materials, meshes. The meshes are handed out ONE AT A TIME (forEachMesh()): the shapes of a large stage are
 * never all resident together.
 */
namespace EmEn::Scenes::Loaders::USD
{
	/** @brief The memory budget of a root layer's parse for any file: 256 MiB (lightusd's own default is 512 MiB). */
	constexpr size_t MinimumMemoryLimit{256ULL * 1024ULL * 1024ULL};

	/** @brief The memory budget of a root layer's parse per byte of the file, above MinimumMemoryLimit: 16 (the FBX
	 * loader's rule). A USDC declares the size of its arrays and lightusd allocated them as declared (fuzz_usd: 15 KB
	 * asking for 400 MB). Owner decision, 2026-10-18. */
	constexpr size_t MemoryLimitPerFileByte{16};


	/**
	 * @brief Where a light sits and where it aims, in RAW USD units and USD axes.
	 *
	 * @note ⚠️ THIS EXISTS BECAUSE TYDRA NEVER FILLS `RenderLight::position` OR
	 * `RenderLight::direction`. `render-light-converter.cc` writes seventeen fields and contains not
	 * one occurrence of `transform`, `position` or `direction`, so those members always hold the
	 * struct's defaults — (0,0,0) and (0,-1,0). Reading them yields a stage whose every light is
	 * stacked at the world origin, correctly valued and illuminating nothing.
	 *
	 * @note NOT baked into engine space: the bake belongs to buildLights(), next to the one the
	 * geometry goes through, so the two conventions stay written in the same place.
	 */
	struct LightPlacement
	{
		/** @brief World position, in stage units. */
		Base::Math::Vector< 3, float > position;

		/** @brief World emission direction — a UsdLux light emits along its LOCAL -Z. */
		Base::Math::Vector< 3, float > direction;
	};

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

	/** @brief The stage metrics, read from the composed layer before it is consumed. */
	struct StageMetrics final
	{
		double startTimeCode{0.0};
		double endTimeCode{0.0};
		float metersPerUnit{1.0F};
		bool zUp{true};
	};

	/** @brief One UsdPreviewSurface material: its factors and the asset identifiers of its textures (empty: none). */
	struct Material final
	{
		std::string resourceName;
		std::string albedoTexture;
		std::string roughnessTexture;
		std::string metalnessTexture;
		std::string normalTexture;
		Base::PixelFactory::Color< float > albedoValue;
		float roughnessValue{0.5F};
		float metalnessValue{0.0F};
		float opacityValue{1.0F};
		float opacityThreshold{0.0F};
		bool isTranslucent{false};
		bool isCutout{false};
	};

	/** @brief One placed mesh, baked into engine space. */
	struct Mesh final
	{
		std::string resourceName;
		std::string primName;
		std::string primPath;
		std::shared_ptr< Base::VertexFactory::Shape< float > > shape;
		/** @brief The RenderMaterial index of the whole-mesh binding. */
		std::optional< size_t > material;
		bool doubleSided{false};
		/** @brief A PointInstancer prototype: drawn by its instances, never given a node. */
		bool instancedOnly{false};
	};

	/** @brief Where a render mesh is drawn: its index in the render scene, its world matrix (row-major, applied to row
	 * vectors: the translation in row 3) and its prim path. */
	struct Placement final
	{
		size_t meshIndex{0};
		std::array< std::array< double, 4 >, 4 > worldMatrix{};
		std::string primPath;
	};

	/** @brief One USD stage, analysed (the meshes are built on demand by forEachMesh()). */
	struct Asset final
	{
		Asset () noexcept;

		~Asset ();

		Asset (const Asset &) = delete;

		Asset (Asset &&) noexcept;

		Asset & operator= (const Asset &) = delete;

		Asset & operator= (Asset &&) noexcept;

		/** @brief Tydra's render scene: the meshes' source, read by forEachMesh(). */
		std::unique_ptr< lightusd::tydra::RenderScene > renderScene;
		std::string resourcePrefix;
		StageMetrics metrics;
		std::vector< Material > materials;
		std::vector< Instancer > instancers;
		std::vector< std::string > prototypePaths;
		/** @brief The mesh placements, in hierarchy order. */
		std::vector< Placement > drawList;
		/** @brief The lights (environment first, then punctual) and the punctual lights' nodes, their indices local to
		 * these lists. An environment light's textureAssetPath is the RAW authored path (the loader resolves it). */
		SceneData lighting;
		size_t punctualLightCount{0};
	};

	/**
	 * @brief Fills the lightusd load options of a root layer: its memory budget, max(MinimumMemoryLimit,
	 * MemoryLimitPerFileByte x the file size), over which lightusd refuses the layer.
	 * @param options The lightusd options.
	 * @param sourceBytes The size of the file holding the root layer (0 when unknown: the minimum applies).
	 */
	void setLoadOptions (lightusd::USDLoadOptions & options, uint64_t sourceBytes) noexcept;

	/**
	 * @brief Composes a root layer — its sublayers, then (when asked) every arc — into a stage.
	 * @param resolver The resolver of the layer's sources (a stage directory tree, or a USDZ archive).
	 * @param rootLayer The root layer, read by the caller (consumed).
	 * @param resolveReferences Whether the references, payloads, inherits and variants are composed
	 * (LoaderOptions::resolveReferences).
	 * @param sourceName The name the logs use.
	 * @param stage The stage receiving the composition.
	 * @param metrics The stage metrics, read before the layer is consumed.
	 * @return bool
	 */
	[[nodiscard]]
	bool composeStage (lightusd::AssetResolutionResolver & resolver, lightusd::Layer & rootLayer, bool resolveReferences, const std::string & sourceName, lightusd::Stage & stage, StageMetrics & metrics) noexcept;

	/**
	 * @brief Analyses a composed stage: inventory report, environment lights, PointInstancers, Tydra's render scene,
	 * materials, mesh placements, punctual lights.
	 * @param stage The composed stage.
	 * @param resolver The resolver Tydra resolves the texture asset paths with.
	 * @param searchPaths The resolver's search paths.
	 * @param usdFilename The root layer's name for Tydra.
	 * @param metrics The stage metrics.
	 * @param resourcePrefix The prefix of every resource name ("usd:<stem>").
	 * @param sourceName The name the logs use.
	 * @return std::optional< Asset > Nothing when Tydra cannot convert the stage.
	 */
	[[nodiscard]]
	std::optional< Asset > analyse (const lightusd::Stage & stage, const lightusd::AssetResolutionResolver & resolver, const std::vector< std::string > & searchPaths, const std::string & usdFilename, const StageMetrics & metrics, const std::string & resourcePrefix, const std::string & sourceName) noexcept;

	/**
	 * @brief Builds the shape of every placed mesh, in placement order, and hands each one to the consumer before the
	 * next is built.
	 * @param asset The analysed stage.
	 * @param consumer Receives each mesh.
	 * @return size_t The number of meshes handed out.
	 */
	size_t forEachMesh (const Asset & asset, const std::function< void (Mesh) > & consumer) noexcept;
}
