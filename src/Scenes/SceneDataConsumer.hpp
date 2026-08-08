/*
 * src/Scenes/SceneDataConsumer.hpp
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
#include <memory>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace SceneLoaders
	{
		struct SceneData;
		struct NodeDescriptor;
	}

	namespace Scenes
	{
		class Node;
		class Scene;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief Consumes an SceneData to build Scene node hierarchies or static entities.
	 * @note This is the Scene-level counterpart of SceneLoaders::Interface.
	 * It takes format-agnostic SceneData and creates engine Scene objects.
	 */
	class EMEN_API SceneDataConsumer final
	{
		public:

			static constexpr auto ClassId{"SceneDataConsumer"};

			/**
			 * @brief Enables or disables hierarchy flattening.
			 * @note When enabled, all mesh visuals are attached directly to the
			 * caller's node, skipping all intermediate structural nodes.
			 * @param flatten True to flatten, false to preserve hierarchy (default).
			 */
			void
			setFlattenHierarchy (bool flatten) noexcept
			{
				m_flattenHierarchy = flatten;
			}

			/**
			 * @brief Enables or disables the creation of the lights declared by the asset.
			 * @note Disabled by default, and deliberately so (owner decision, 2026-08-08): a
			 * demo that lights its own scene must not have an asset's lights appear behind its
			 * back — a photometric calibration is a whole, and adding uninvited emitters to it
			 * silently changes the exposure the scene was balanced for. Turn it on when the
			 * asset IS the lighting authority.
			 * @warning Ask the loader's capabilities() before relying on this: an empty light
			 * table means "the loader does not read lights" just as much as it means "this asset
			 * declares none".
			 * @param create True to instantiate the asset's lights, false to ignore them (default).
			 */
			void
			setCreateLights (bool create) noexcept
			{
				m_createLights = create;
			}

			/**
			 * @brief Builds Scene objects from an SceneData.
			 * @param sceneData The loaded asset data (resources + node descriptors).
			 * @param scene Reference to the scene.
			 * @param parentNode If nullptr, mesh nodes become StaticEntity (world coordinates).
			 *				   Otherwise, the hierarchy is built under the given node.
			 * @return bool
			 */
			[[nodiscard]]
			bool build (const SceneLoaders::SceneData & sceneData, Scene & scene, const std::shared_ptr< Node > & parentNode = nullptr) noexcept;

		private:

			void processNodeAsStatic (const SceneLoaders::SceneData & sceneData, size_t nodeIndex, Scene & scene, const Base::Math::CartesianFrame< float > & parentWorldFrame) noexcept;

			void processNodeAsNode (const SceneLoaders::SceneData & sceneData, size_t nodeIndex, const std::shared_ptr< Node > & engineParent) noexcept;

			/**
			 * @brief Attaches the light referenced by a node descriptor to an engine entity.
			 * @note No-op when light creation is disabled or the descriptor carries no light.
			 * @tparam entity_t The entity type owning the component (Node or StaticEntity).
			 * @param sceneData The loaded asset data.
			 * @param nodeDescriptor The descriptor possibly referencing a light.
			 * @param entity A reference to the entity that will own the light component.
			 */
			template< typename entity_t >
			void attachLight (const SceneLoaders::SceneData & sceneData, const SceneLoaders::NodeDescriptor & nodeDescriptor, entity_t & entity) const noexcept;

			bool m_flattenHierarchy{false};
			bool m_createLights{false};
	};
}
