/*
 * src/Libs/VertexFactory/TextureCoordinates.hpp
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
#include <cmath>
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/Math/Base.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Libs::VertexFactory::TextureCoordinates
{
	/**
	 * @brief Generates cubic texture coordinates.
	 * @tparam float_t The type of floating point number. Default float;
	 * @param position The vertex position.
	 * @param normal The vertex normal vector.
	 * @return Vector< 3, type_t >
	 */
	template< typename float_t = float >
	Math::Vector< 3, float_t >
	generateCubicCoordinates (const Math::Vector< 3, float_t > & position, const Math::Vector< 3, float_t > & normal) noexcept requires (std::is_floating_point_v< float_t >)
	{
		const auto absNX = std::abs(normal[Math::X]);
		const auto absNY = std::abs(normal[Math::Y]);
		const auto absNZ = std::abs(normal[Math::Z]);

		float_t texCoordU = 0;
		float_t texCoordV = 0;
		float_t texCoordW = 0;

		/* From ZY Plane */
		if ( absNX > absNZ && absNX > absNY )
		{
			texCoordU = normal[Math::X] > 0 ? ( position[Math::Z] > 0 ? 0 : 1 ) : ( position[Math::Z] > 0 ? 1 : 0 );
			texCoordV = position[Math::Y] > 0 ? 1 : 0;
		}
		/* From XZ Plane */
		else if ( absNY > absNX && absNY > absNZ )
		{
			texCoordU = position[Math::X] > 0 ? 1 : 0;
			texCoordV = normal[Math::Y] > 0 ? ( position[Math::Z] > 0 ? 0 : 1 ) : ( position[Math::Z] > 0 ? 1 : 0 );
		}
		/* From XY Plane */
		else if ( absNZ > absNX && absNZ > absNY )
		{
			texCoordU = normal[Math::Z] > 0 ? ( position[Math::X] > 0 ? 1 : 0 ) : ( position[Math::X] > 0 ? 0 : 1 );
			texCoordV = position[Math::Y] > 0 ? 1 : 0;
		}

		return {texCoordU, texCoordV, texCoordW};
	}

	/**
	 * @brief Generates spherical texture coordinates.
	 * @tparam float_t The type of floating point number. Default float.
	 * @param position A reference to a vector.
	 * @param radius The radius of the sphere.
	 * @return Math::Vector< 3, type_t >
	 */
	template< typename float_t = float >
	Math::Vector< 3, float_t >
	generateSphericalCoordinates (const Math::Vector< 3, float_t > & position, float_t radius = 1) noexcept requires (std::is_floating_point_v< float_t >)
	{
		return {
			/* S angle (longitude). */
			std::atan2(position[Math::X], position[Math::Z]) + Math::Pi< float_t >,
			/* T angle (latitude). */
			static_cast< float_t >(1.0) - std::acos(position[Math::Y] / radius) / Math::Pi< float_t >,
			static_cast< float_t >(0)
		};
	}
}
