/*
 * src/Scenes/AssetDataConsumer.hpp
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
#include <memory>

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"

/* Forward declarations. */
namespace EmEn::AssetLoaders
{
	struct AssetData;
}

namespace EmEn::Scenes
{
	class Node;
	class Scene;

	/**
	 * @brief Consumes an AssetData to build Scene node hierarchies or static entities.
	 * @note This is the Scene-level counterpart of AssetLoaders::Interface.
	 * It takes format-agnostic AssetData and creates engine Scene objects.
	 */
	class AssetDataConsumer final
	{
		public:

			static constexpr auto ClassId{"AssetDataConsumer"};

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
			 * @brief Builds Scene objects from an AssetData.
			 * @param assetData The loaded asset data (resources + node descriptors).
			 * @param scene Reference to the scene.
			 * @param parentNode If nullptr, mesh nodes become StaticEntity (world coordinates).
			 *                   Otherwise, the hierarchy is built under the given node.
			 * @return bool
			 */
			[[nodiscard]]
			bool build (const AssetLoaders::AssetData & assetData, Scene & scene, const std::shared_ptr< Node > & parentNode = nullptr) noexcept;

		private:

			void processNodeAsStatic (const AssetLoaders::AssetData & assetData, size_t nodeIndex, Scene & scene, const Libs::Math::CartesianFrame< float > & parentWorldFrame) noexcept;

			void processNodeAsNode (const AssetLoaders::AssetData & assetData, size_t nodeIndex, const std::shared_ptr< Node > & engineParent) noexcept;

			bool m_flattenHierarchy{false};
	};
}
