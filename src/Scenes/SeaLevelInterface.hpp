/*
 * src/Scenes/SeaLevelInterface.hpp
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

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Interface to define a visible and physical sea level in a scene.
	 */
	class SeaLevelInterface
	{
		public:

			/**
			 * @brief Destructs the sea level interface.
			 */
			virtual ~SeaLevelInterface () = default;

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
			virtual float getLevelAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns a position where Y is completed by the water level at X,Z position.
			 * @param positionX The X coordinates.
			 * @param positionZ The Z coordinates.
			 * @param deltaY A difference value to add to the Y component. Default 0.0F.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY = 0.0F) const noexcept = 0;

			/**
			 * @brief Returns the normal vector at the given position.
			 * @param worldPosition An absolute position.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getNormalAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Checks if a world position is submerged (below water level).
			 * @param worldPosition An absolute position.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isSubmerged (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns the depth at a given position (positive if submerged, negative if above).
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getDepthAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Updates the water surface visibility from the camera position.
			 * @note This is not frustum-culling, but helps the water to know where the point of view is located.
			 * @param worldPosition The camera world position.
			 */
			virtual void updateVisibility (const Libs::Math::Vector< 3, float > & worldPosition) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable sea level interface.
			 */
			SeaLevelInterface () noexcept = default;
	};
}
