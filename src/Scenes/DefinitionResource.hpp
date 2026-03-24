/*
 * src/Scenes/DefinitionResource.hpp
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

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions. */
#include "Resources/Container.hpp"

namespace EmEn::Scenes
{
	class Scene;
	class Node;

	/**
	 * @brief The scene definition class. Builds a complete scene from a JSON description.
	 * @details The JSON format supports:
	 * - Name, Boundary: scene identification and size
	 * - Properties: physics (gravity, atmosphere, etc.)
	 * - Background: SkyBox, DynamicSky, Color
	 * - Ground: Basic (flat), Terrain (heightmap)
	 * - Lighting: Static directional lighting
	 * - Nodes: hierarchical tree with components (Camera, Microphone, Lights, Visual, etc.)
	 * - StaticEntities: positioned objects with components (Visual, Lights, SoundEmitter, etc.)
	 * Each element uses a "Type" field for extensibility.
	 * @extends EmEn::Resources::ResourceTrait This is a resource
	 */
	class DefinitionResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< DefinitionResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DefinitionResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/* Top-level JSON keys. */
			static constexpr auto BoundaryKey{"Boundary"};
			static constexpr auto BackgroundKey{"Background"};
			static constexpr auto GroundKey{"Ground"};
			static constexpr auto LightingKey{"Lighting"};
			static constexpr auto NodesKey{"Nodes"};
			static constexpr auto StaticEntitiesKey{"StaticEntities"};
			static constexpr auto ExtraDataKey{"ExtraData"};

			/* Shared JSON keys. */
			static constexpr auto TypeKey{"Type"};
			static constexpr auto ResourceKey{"Resource"};
			static constexpr auto PositionKey{"Position"};
			static constexpr auto LookAtKey{"LookAt"};
			static constexpr auto ComponentsKey{"Components"};
			static constexpr auto ScaleKey{"Scale"};
			static constexpr auto ColorKey{"Color"};
			static constexpr auto IntensityKey{"Intensity"};
			static constexpr auto PrimaryKey{"Primary"};
			static constexpr auto MeshKey{"Mesh"};
			static constexpr auto MaterialKey{"Material"};

			/* Property keys. */
			static constexpr auto SurfaceGravityKey{"SurfaceGravity"};
			static constexpr auto AtmosphericDensityKey{"AtmosphericDensity"};
			static constexpr auto PlanetRadiusKey{"PlanetRadius"};
			static constexpr auto WaterDensityKey{"WaterDensity"};

			/* Ground keys. */
			static constexpr auto GridDivisionKey{"GridDivision"};
			static constexpr auto UVMultiplierKey{"UVMultiplier"};
			static constexpr auto ShiftHeightKey{"ShiftHeight"};

			/* Noise keys. */
			static constexpr auto NoiseKey{"Noise"};
			static constexpr auto SizeKey{"Size"};
			static constexpr auto FactorKey{"Factor"};
			static constexpr auto RoughnessKey{"Roughness"};
			static constexpr auto SeedKey{"Seed"};

			/* Lighting keys. */
			static constexpr auto AmbientKey{"Ambient"};
			static constexpr auto LightKey{"Light"};
			static constexpr auto DirectionKey{"Direction"};

			/**
			 * @brief Constructs a definition resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			DefinitionResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, name, resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return 0;
			}

			/**
			 * @brief Returns the scene name from the JSON definition.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getSceneName () const noexcept;

			/**
			 * @brief Returns the scene boundary from the JSON definition.
			 * @param defaultBoundary Fallback value if not specified.
			 * @return float
			 */
			[[nodiscard]]
			float getBoundary (float defaultBoundary = 1000.0F) const noexcept;

			/**
			 * @brief Builds the scene from the JSON definition.
			 * @param scene A reference to the scene to populate.
			 * @return bool
			 */
			bool buildScene (Scene & scene) noexcept;

			/**
			 * @brief Returns the extra data from the scene definition.
			 * @return Json::Value
			 */
			[[nodiscard]]
			Json::Value getExtraData () const noexcept;

		private:

			/**
			 * @brief Reads and applies physics properties.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readProperties (Scene & scene) noexcept;

			/**
			 * @brief Reads and applies the background (SkyBox, DynamicSky, Color).
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readBackground (Scene & scene) noexcept;

			/**
			 * @brief Reads and applies the ground (Basic, Terrain).
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readGround (Scene & scene) noexcept;

			/**
			 * @brief Reads and applies the lighting setup.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readLighting (Scene & scene) noexcept;

			/**
			 * @brief Reads and creates nodes recursively.
			 * @param scene A reference to the scene.
			 * @param parentNode The parent node to attach children to.
			 * @param nodesArray The JSON array of node definitions.
			 * @return bool
			 */
			bool readNodes (Scene & scene, const std::shared_ptr< Node > & parentNode, const Json::Value & nodesArray) noexcept;

			/**
			 * @brief Reads and creates static entities.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readStaticEntities (Scene & scene) noexcept;

			Json::Value m_root;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using SceneDefinitions = Container< Scenes::DefinitionResource >;
}
