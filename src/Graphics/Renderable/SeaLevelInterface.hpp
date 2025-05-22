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
		protected:

			/**
			 * @brief Constructs a renderable sea level interface.
			 * @param seaLevelName A string for the sea level resource name [std::move].
			 * @param renderableFlags The resource flag bits.
			 */
			explicit
			SeaLevelInterface (std::string seaLevelName, uint32_t renderableFlags) noexcept
				: Interface{std::move(seaLevelName), renderableFlags}
			{

			}
	};
}
