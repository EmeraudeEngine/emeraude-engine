/*
 * src/Scenes/Loaders/WADLoader.hpp
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

namespace EmEn::Scenes::Loaders
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
	 * @extends EmEn::Scenes::Loaders::Interface This is an asset loader.
	 */
	class EMEN_API WADLoader final : public Interface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WADLoader"};

			/** @brief Doom map unit scale: 32 map units per meter (player height 56 u ≈ 1.75 m). */
			static constexpr auto MapUnitsPerMeter{32.0F};

			/**
			 * @brief Luminance of a FULLY-LIT map surface, in nits (cd/m2).
			 * @note A Doom map carries no photometry: its sector light levels are 0-255 ordinals
			 * authored for a CRT, not luminances. Emitted raw into a photometric pipeline they land
			 * near zero (measured: 0.038 mean output), so the map needs an ABSOLUTE anchor. The
			 * surfaces are declared self-illuminating rather than lit — same reasoning as a skybox:
			 * they carry their own baked lighting and must not be re-lit.
			 * @note ⚠️ THIS VALUE AND THE DEMO'S FIXED EXPOSURE ARE ONE JOINT CALIBRATION. Never
			 * move one without the other. A fully-lit Doom surface is treated as a SUNLIT surface
			 * (~2000-5000 nits in the real world) rather than as white on an SDR monitor (~250), for
			 * one reason: the map has to share the frame with a sky. At 250 nits the map read well
			 * but sat 5 stops under a daylight sky, so the exposure that rendered the map correctly
			 * blew the sky to pure white over half the frame on any outdoor map — the map itself was
			 * exposed correctly the whole time, which is what makes that failure so confusing to
			 * diagnose. At 2000 nits the map sits just under a clear sky and both fit in one frame.
			 * The demo pairs this with f/8 at 1/125 s and ISO 125 (EV100 12.64, clipping at 7680
			 * nits), which puts a fully-lit surface at 0.26 display-linear and leaves 26 of the 28
			 * shipped skies unclipped. See DoomLoader::onEnabled() for the derivation.
			 */
			static constexpr auto FullBrightLuminance{2000.0F};

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
			 * @brief Returns the Player 1 start position in ENGINE WORLD space (Y-up),
			 * at the sector floor height. The consumer applies NO rotation: since the Y-up
			 * flip the import is the identity.
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
			 * @brief Returns the Player 1 facing direction in ENGINE WORLD space (Y-up), unit XZ vector.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			playerStartDirection () const noexcept
			{
				return m_playerStartDirection;
			}

			/** @copydoc EmEn::Scenes::Loaders::Interface::load() */
			[[nodiscard]]
			bool load (const std::filesystem::path & filepath, SceneData & output) noexcept override;

			/** @copydoc EmEn::Scenes::Loaders::Interface::supportsExtension() */
			[[nodiscard]]
			bool
			supportsExtension (std::string_view extension) const noexcept override
			{
				return extension == ".wad" || extension == ".WAD";
			}

			/**
			 * @copydoc EmEn::Scenes::Loaders::Interface::capabilities()
			 * @note A Doom level carries its lighting BAKED into vertex colours on unlit
			 * materials — there is no punctual light to deliver, by design. See
			 * MeshDescriptor::lightingEnabled.
			 */
			[[nodiscard]]
			uint32_t
			capabilities () const noexcept override
			{
				return Geometry;
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
