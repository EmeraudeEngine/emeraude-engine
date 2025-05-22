/*
 * src/Graphics/Renderable/SceneAreaInterface.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

namespace EmEn::Graphics::Renderable
{
	/**
	 * @brief Interface to define a physical and visible floor in a scene.
	 * @extends EmEn::Graphics::Renderable::Interface This class is a renderable object in the 3D world.
	 */
	class SceneAreaInterface : public Interface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SceneAreaInterface (const SceneAreaInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SceneAreaInterface (SceneAreaInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SceneAreaInterface &
			 */
			SceneAreaInterface & operator= (const SceneAreaInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SceneAreaInterface &
			 */
			SceneAreaInterface & operator= (SceneAreaInterface && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract scene area.
			 */
			~SceneAreaInterface () override = default;

			/**
			 * @brief Returns the ground level under the given position.
			 * @param worldPosition An absolute position.
			 * @return float
			 */
			[[nodiscard]]
			virtual float getLevelAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

			/**
			 * @brief Returns a position where Y is completed by the level at X,Z position.
			 * @param positionX The X coordinates.
			 * @param positionZ The Z coordinates.
			 * @param deltaY A difference value to add to the height.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getLevelAt (float positionX, float positionZ, float deltaY) const noexcept = 0;

			/**
			 * @brief Returns the normal vector under the given position.
			 * @param worldPosition An absolute position.
			 * @return Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getNormalAt (const Libs::Math::Vector< 3, float > & worldPosition) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a renderable scene area.
			 * @param name A reference to a string for the resource name.
			 * @param resourceFlags The resource flag bits.
			 */
			explicit SceneAreaInterface (const std::string & name, uint32_t resourceFlags) noexcept;
	};
}
