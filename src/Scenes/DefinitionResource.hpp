/*
 * src/Scenes/DefinitionResource.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

	/**
	 * @brief The scene definition class.
	 * @extends EmEn::Resources::ResourceTrait This is a resource
	 */
	class DefinitionResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< DefinitionResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DefinitionResource"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/* JSON key. */
			static constexpr auto BackgroundKey{"Background"};
			static constexpr auto SceneAreaKey{"SceneArea"};
			static constexpr auto ExtraDataKey{"ExtraData"};
			static constexpr auto SurfaceGravityKey{"SurfaceGravity"};
			static constexpr auto AtmosphericDensityKey{"AtmosphericDensity"};
			static constexpr auto PlanetRadiusKey{"PlanetRadius"};
			static constexpr auto WaterDensityKey{"WaterDensity"};
			static constexpr auto NodesKey{"Nodes"};
			static constexpr auto ComponentsKey{"Components"};

			/**
			 * @brief Constructs a definition resource.
			 * @param name The name of the resource.
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			DefinitionResource (const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{name, resourceFlags}
			{

			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
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
				// TODO with json node ...
				return 0;
			}

			/** @brief Gives the name of the scene. */
			[[nodiscard]]
			std::string getSceneName () const noexcept;

			/** @brief Build the scene from JSON definition. */
			bool buildScene (Scene & scene) noexcept;

			/** @brief Gets the extra data from the scene definition */
			[[nodiscard]]
			Json::Value getExtraData () const noexcept;

		private:

			/**
			 * @brief readProperties
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readProperties (Scene & scene) noexcept;

			/**
			 * @brief readBackground
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readBackground (Scene & scene) noexcept;

			/**
			 * @brief readSceneArea
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			bool readSceneArea (Scene & scene) noexcept;

			Json::Value m_root;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using SceneDefinitions = Container< Scenes::DefinitionResource >;
}
