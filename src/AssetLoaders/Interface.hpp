/*
 * src/AssetLoaders/Interface.hpp
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
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>

/* Forward declarations. */
namespace EmEn::AssetLoaders
{
	struct AssetData;
}

namespace EmEn::AssetLoaders
{
	/**
	 * @brief Options that affect resource loading (shared by all loaders).
	 * @note flattenHierarchy is NOT here — it only affects scene building,
	 * not resource loading, and belongs in Scenes::AssetDataConsumer.
	 */
	struct LoaderOptions
	{
		std::unordered_set< std::string > excludedNodeNames;
		bool skipSkinning{false};
	};

	/**
	 * @brief Common interface for composite asset format loaders.
	 * @note Implementations load resources into engine containers and produce
	 * a format-agnostic AssetData describing the node hierarchy.
	 * No dependency on Scenes/ types.
	 */
	class Interface
	{
		public:

			virtual ~Interface () = default;

			/**
			 * @brief Sets loader options.
			 * @param options The options to apply.
			 */
			void
			setOptions (LoaderOptions options) noexcept
			{
				m_options = std::move(options);
			}

			/**
			 * @brief Loads a composite asset from a file.
			 * @param filepath Path to the asset file.
			 * @param output The AssetData to populate with loaded resources and hierarchy.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool load (const std::filesystem::path & filepath, AssetData & output) noexcept = 0;

			/**
			 * @brief Checks if this loader supports the given file extension.
			 * @param extension The file extension (e.g., ".gltf", ".glb").
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool supportsExtension (std::string_view extension) const noexcept = 0;

		protected:

			Interface () noexcept = default;
			Interface (const Interface &) = default;
			Interface (Interface &&) = default;
			Interface & operator= (const Interface &) = default;
			Interface & operator= (Interface &&) = default;

			LoaderOptions m_options;
	};
}
