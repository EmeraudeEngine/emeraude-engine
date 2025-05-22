/*
* src/Libs/Math/Space2D/Intersections.hpp
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
#include "Libs/Math/Space2D/Line.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a line is intersecting a rectangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param rectangle A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const AARectangle< precision_t > & rectangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO...

		return false;
	}

	/**
	 * @brief Checks if a line is intersecting a rectangle and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param rectangle A reference to a rectangle.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const AARectangle< precision_t > & rectangle, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO...

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, AARectangle Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AARectangle< precision_t > & rectangle, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isIntersecting(const Line< precision_t > &, AARectangle Triangle< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const AARectangle< precision_t > & rectangle, const Line< precision_t > & line, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, rectangle, intersection);
	}
}
