/*
 * src/Libs/Math/Plane.hpp
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
#include <cassert>
#include <cmath>
#include <type_traits>
#include <sstream>
#include <string>

/* Local inclusions. */
#include "Vector.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief A class representing a plane in space.
	 * @tparam data_t The type of floating point number. Default float.
	 * @note Plan equation "ax + by + cy + d = 0"
	 */
	template< typename data_t = float >
	requires (std::is_floating_point_v< data_t >)
	class Plane final
	{
		public:

			/**
			 * @brief Constructs a default plane.
			 * @note The plane normal will point toward Z+.
			 */
			Plane () noexcept = default;

			/**
			 * @brief Constructs a plane from a normal and a point.
			 * @param direction A reference to a vector. Will be normalized.
			 * @param point A reference to a vector.
			 */
			Plane (const Vector< 3, data_t > & direction, const Vector< 3, data_t > & point)
				: m_normal(direction.normalized()),
				m_distance(-m_normal.dotProduct(point))
			{

			}

			/**
			 * @brief Constructs a plane from 3 points.
			 * @param pointA A reference to a vector.
			 * @param pointB A reference to a vector.
			 * @param pointC A reference to a vector.
			 */
			Plane (const Vector< 3, data_t > & pointA, const Vector< 3, data_t > & pointB, const Vector< 3, data_t > & pointC)
				: m_normal(Vector< 3, data_t >::normal(pointA, pointB, pointC)),
				m_distance(-m_normal.dotProduct(pointA))
			{

			}

			/**
			 * @brief Constructs a plane with defined data.
			 * @param normal A reference to a vector. Must be normalized!
			 * @param distance A value.
			 */
			Plane (const Vector< 3, data_t > & normal, data_t distance) noexcept
				: m_normal(normal),
				m_distance(distance)
			{
				assert(std::abs(m_normal.lengthSquared() - static_cast< data_t >(1.0)) < static_cast< data_t >(1e-5) && "Plane(normal, distance) requires the normal vector to be normalized!");
			}

			/**
			 * @brief Returns the distance between a point and the plane.
			 * @param point A reference to a vector.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			getDistanceBetween (const Vector< 3, data_t > & point) const noexcept
			{
				return std::abs(m_normal.dotProduct(point) + m_distance);
			}

			/**
			 * @brief Returns the normal to the plane.
			 * @return const Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Vector< 3, data_t > &
			normal () const noexcept
			{
				return m_normal;
			}

			/**
			 * @brief Returns the distance.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			distance () const noexcept
			{
				return m_distance;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Plane & obj)
			{
				out <<
					"Plane data :" "\n"
					"Normal " << obj.normal() << "\n"
					"Distance : " << obj.distance() << '\n';

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Plane & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Vector< 3, data_t > m_normal{0, 0, 1};
			data_t m_distance{0};
	};
}
