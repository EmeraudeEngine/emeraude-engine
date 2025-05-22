/*
 * src/Libs/VertexFactory/Normal.hpp
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
#include <type_traits>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Libs::VertexFactory::Normal
{
	/**
	 * @brief Generates spherical texture coordinates.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @param normal A vector.
	 * @return PixelFactory::Color< vertex_data_t >
	 */
	template< typename vertex_data_t = float >
	PixelFactory::Color< vertex_data_t >
	normalToRGBA (Math::Vector< 3, vertex_data_t > normal) noexcept requires (std::is_floating_point_v< vertex_data_t >)
	{
		/* NOTE : transforms from [-1,1] to [0,1] */
		normal *= 0.5;
		normal += 0.5;

		return {normal.x(), normal.y(), normal.z(), static_cast< vertex_data_t >(1)};
	}

	/**
	 * @brief Generates spherical texture coordinates.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @param normal A vector.
	 * @return PixelFactory::Color< vertex_data_t >
	 */
	template< typename vertex_data_t = float >
	PixelFactory::Color< vertex_data_t >
	normalToRGBA (Math::Vector< 4, vertex_data_t > normal) noexcept requires (std::is_floating_point_v< vertex_data_t >)
	{
		/* NOTE : transforms from [-1,1] to [0,1] */
		normal *= 0.5;
		normal += 0.5;

		return {normal.x(), normal.y(), normal.z(), static_cast< vertex_data_t >(1)};
	}

	/**
	 * @brief Generates spherical texture coordinates.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @param direction A reference to vector.
	 * @return PixelFactory::Color< vertex_data_t >
	 */
	template< typename vertex_data_t = float >
	PixelFactory::Color< vertex_data_t >
	directionToRGBA (const Math::Vector< 3, vertex_data_t > & direction) noexcept requires (std::is_floating_point_v< vertex_data_t >)
	{
		return normalToRGBA(direction.normalized());
	}

	/**
	 * @brief Generates spherical texture coordinates.
	 * @tparam vertex_data_t The precision type of vertex data. Default float.
	 * @param direction A reference to vector.
	 * @return PixelFactory::Color< vertex_data_t >
	 */
	template< typename vertex_data_t = float >
	PixelFactory::Color< vertex_data_t >
	directionToRGBA (const Math::Vector< 4, vertex_data_t > & direction) noexcept requires (std::is_floating_point_v< vertex_data_t >)
	{
		return normalToRGBA(direction.normalized());
	}
}
