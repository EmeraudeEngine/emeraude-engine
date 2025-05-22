/*
* src/Libs/Math/Space2D/Collisions.hpp
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
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Circle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a triangle is colliding with a circle.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param circle A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO ...

		return false;
	}

	/**
	 * @brief Checks if a triangle is colliding with a circle and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param circle A reference to a circle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Circle< precision_t > & circle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO ...

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const Circle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, circle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const Circle< precision_t > &, Vector< 2, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const Triangle< precision_t > & triangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, circle, minimumTranslationVector);
	}
}
