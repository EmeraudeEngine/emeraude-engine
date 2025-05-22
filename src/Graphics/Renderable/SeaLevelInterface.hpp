/*
 * src/Graphics/Renderable/SeaLevelInterface.hpp
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
	 * @brief Interface to define a visible sea level in a scene.
	 * @extends EmEn::Graphics::Renderable::Interface This class is a renderable object in the 3D world.
	 */
	class SeaLevelInterface : public Interface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SeaLevelInterface (SeaLevelInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (const SeaLevelInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SeaLevelInterface &
			 */
			SeaLevelInterface & operator= (SeaLevelInterface && copy) noexcept = delete;
		
			/**
			 * @brief Destructs the renderable sea level.
			 */
			~SeaLevelInterface () override = default;

		protected:

			/**
			 * @brief Constructs a renderable sea level.
			 * @param name A reference to a string for the resource name.
			 * @param resourceFlags The resource flag bits.
			 */
			explicit SeaLevelInterface (const std::string & name, uint32_t resourceFlags) noexcept;
	};
}
