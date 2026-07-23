/*
 * src/AssetLoaders/WADLoader.hpp
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
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	class Manager;
}

namespace EmEn::AssetLoaders
{
	/**
	 * @brief Loads a classic Doom-engine WAD (IWAD/PWAD) and materializes ONE map as static
	 * textured geometry: walls from the linedefs/sidedefs, floors and ceilings from the
	 * BSP subsectors (SSECTORS/SEGS convex fans — no polygon triangulation needed), wall
	 * textures composited from patches (TEXTURE1/TEXTURE2 + PNAMES), flats decoded through
	 * the PLAYPAL palette. Sector light levels are baked as vertex colors on unlit (Basic)
	 * materials. Game mechanics, things and sprites are deliberately ignored — this is a
	 * level MATERIALIZER, not a game loader.
	 * @note Both map naming schemes are supported (ExMy and MAPxx); the map is selected by
	 * 1-based index in directory order (setMapIndex) or by explicit name (setMapName).
	 * @extends EmEn::AssetLoaders::Interface This is an asset loader.
	 */
	class EMEN_API WADLoader final : public Interface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WADLoader"};

			/** @brief Doom map unit scale: 32 map units per meter (player height 56 u ≈ 1.75 m). */
			static constexpr auto MapUnitsPerMeter{32.0F};

			/**
			 * @brief Constructs the WAD loader.
			 * @param resources A reference to the resource manager.
			 */
			explicit WADLoader (Resources::Manager & resources) noexcept;

			/**
			 * @brief Selects the map to load by 1-based index, in WAD directory order.
			 * @note Works for both ExMy (Doom) and MAPxx (Doom II) naming. Default 1.
			 * @param index The 1-based map index.
			 * @return void
			 */
			void
			setMapIndex (uint32_t index) noexcept
			{
				m_mapIndex = index > 0 ? index : 1;
				m_mapName.clear();
			}

			/**
			 * @brief Selects the map to load by explicit marker name (e.g. "E1M1", "MAP01").
			 * @note Overrides setMapIndex().
			 * @param name The map marker lump name.
			 * @return void
			 */
			void
			setMapName (std::string name) noexcept
			{
				m_mapName = std::move(name);
			}

			/**
			 * @brief Returns the marker name of the map actually loaded (empty before load()).
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			loadedMapName () const noexcept
			{
				return m_loadedMapName;
			}

			/**
			 * @brief Returns the Player 1 start position in ENGINE WORLD space (Y-down),
			 * i.e. after the consumer's 180° X rotation, at the sector floor height.
			 * @note Valid after load(). The eye height is up to the caller.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			playerStartPosition () const noexcept
			{
				return m_playerStartPosition;
			}

			/**
			 * @brief Returns the Player 1 facing direction in ENGINE WORLD space (Y-down), unit XZ vector.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			playerStartDirection () const noexcept
			{
				return m_playerStartDirection;
			}

			/** @copydoc EmEn::AssetLoaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, AssetData & output) noexcept override;

			/** @copydoc EmEn::AssetLoaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".wad" || extension == ".WAD";
			}

		private:

			Resources::Manager & m_resources;
			std::string m_mapName;
			std::string m_loadedMapName;
			Base::Math::Vector< 3, float > m_playerStartPosition;
			Base::Math::Vector< 3, float > m_playerStartDirection{0.0F, 0.0F, 1.0F};
			uint32_t m_mapIndex{1};
	};
}
