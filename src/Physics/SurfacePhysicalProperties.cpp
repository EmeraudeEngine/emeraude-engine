/*
 * src/Physics/SurfacePhysicalProperties.cpp
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

#include "SurfacePhysicalProperties.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Physics
{
	std::ostream &
	operator<< (std::ostream & out, const SurfacePhysicalProperties & obj)
	{
		return out <<
			"Surface physical properties :" "\n"
			"Density : " << obj.m_density << " ???" "\n";
	}

	std::string
	to_string (const SurfacePhysicalProperties & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
