/*
 * src/Scenes/SeaLevelInterface.hpp
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

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	struct SeaSurfaceWaves;
}

namespace EmEn::Scenes
{
	/* Forward declarations. */
	class GroundLevelInterface;
	class WaterMedium;

	/**
	 * @brief Interface to define a visible and physical sea level in a scene.
	 */
	class EMEN_API SeaLevelInterface
	{
		public:

			/**
			 * @brief Destructs the sea level interface.
			 */
			virtual ~SeaLevelInterface () = default;

			/**
			 * @brief Copy constructor (deleted: a polymorphic interface, copying would slice).
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (SeaLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (SeaLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Returns the constant water level height.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevel () const noexcept = 0;

			/**
			 * @brief Returns the water level at the given position.
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevelAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns a position where Y is completed by the water level at X,Z position.
			 * @param positionX The X coordinates.
			 * @param positionZ The Z coordinates.
			 * @param deltaY A difference value to add to the Y component. Default 0.0F.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY = 0.0F) const noexcept = 0;

			/**
			 * @brief Returns the normal vector at the given position.
			 * @param worldPosition An absolute position.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getNormalAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Checks if a world position is submerged (below water level).
			 * @param worldPosition An absolute position.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isSubmerged (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns the depth at a given position (positive if submerged, negative if above).
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getDepthAt (const Base::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns the water the sea declares, or nullptr when it declares none.
			 * @note The sea level answers the optics of its water (owner decision 2026-11-23, engine item
			 * underwater-view): what the underwater view reads. A renderable sea answers the water its surface material
			 * declares (Graphics::Material::Interface::waterVolume(), owner decision 2026-11-24): one statement, read by
			 * the surface's shading and by the view from below. A sea without declared water has no underwater view and
			 * is drawn as a surface seen from above.
			 * @return const WaterMedium *
			 */
			[[nodiscard]]
			virtual const WaterMedium * waterMedium () const noexcept = 0;

			/**
			 * @brief The scene presents its ground to its sea: the sea adapts to it.
			 * @note Called by the scene whenever it holds both (Scene::setGroundLevel(), Scene::setSeaLevel(), its
			 * construction), again if either changes. A sea whose surface moves learns there where the land shelters
			 * its water — a basin enclosed in an island, behind a passage too narrow for the swell — and calms its waves
			 * there (owner decisions 2026-11-24, engine item ocean-sheltered-water); a surface at rest has nothing to
			 * adapt.
			 * @note Synchronous, on the thread that builds the scene: a bounded computation over the scene's square.
			 * @param ground A reference to the scene's ground. Only read during the call.
			 * @param boundary The scene's boundary, in metres: the ground is read over the square [-boundary, boundary]
			 * on X and Z.
			 */
			virtual void adaptToGround (const GroundLevelInterface & ground, float boundary) noexcept = 0;

			/**
			 * @brief Returns what a pass binds to know where the MOVING surface stands (the displacement its mesh is
			 * drawn with), or nullptr when the surface is at rest — it then is the level, everywhere.
			 * @note ⚠️ RENDER THREAD, valid for the frame it is asked in. Null too while a moving surface has not been
			 * drawn yet: its displacement holds nothing before its first update.
			 * @return const Graphics::SeaSurfaceWaves *
			 */
			[[nodiscard]]
			virtual const Graphics::SeaSurfaceWaves * surfaceWaves () const noexcept = 0;

			/**
			 * @brief Updates the water surface visibility from the camera position.
			 * @note This is not frustum-culling, but helps the water to know where the point of view is located.
			 * @param worldPosition The camera world position.
			 */
			virtual void updateVisibility (const Base::Math::Vector< 3, float > & worldPosition) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable sea level interface.
			 */
			SeaLevelInterface () noexcept = default;
	};
}
