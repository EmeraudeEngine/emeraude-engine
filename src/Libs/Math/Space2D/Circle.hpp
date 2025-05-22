/*
 * src/Libs/Math/Space2D/Circle.hpp
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
#include <sstream>
#include <string>

/* Local inclusions for usages. */
#include "Libs/Math/Base.hpp"
#include "Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Class for a circle in 2D space.
	 * @tparam precision_t The precision type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_arithmetic_v< precision_t >)
	class Circle final
	{
		public:

			/**
			 * @brief Constructs a default circle.
			 */
			constexpr Circle () noexcept = default;

			/**
			 * @brief Constructs a circle.
			 * @param radius The initial radius. Default 0.
			 * @param position A reference to a point. Default origin.
			 */
			explicit
			constexpr
			Circle (precision_t radius = 0, const Point< precision_t > & position = {}) noexcept
				: m_position(position), m_radius(radius < 0 ? -radius : radius)
			{

			}

			/**
			 * @brief Returns whether another circle is different.
			 * @param operand A reference to a circle.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator== (const Circle & operand) const noexcept
			{
				if ( m_position != operand.m_position )
				{
					return false;
				}

				return std::abs(m_radius - operand.m_radius) <= std::numeric_limits< precision_t >::epsilon() * std::max(precision_t(1.0), std::max(std::abs(m_radius), std::abs(operand.m_radius)));
			}

			/**
			 * @brief Returns whether another circle is different.
			 * @param operand A reference to a circle.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			operator!= (const Circle & operand) const noexcept
			{
				return !(*this == operand);
			}

			/**
			 * @brief Returns whether the circle is a coherent surface.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			isValid () const noexcept
			{
				return m_radius > static_cast< precision_t >(0);
			}

			/**
			 * @brief Sets the position of the circle in space.
			 * @param position A reference to a point.
			 * @return void
			 */
			void
			setPosition (const Point< precision_t > & position) noexcept
			{
				m_position = position;
			}

			/**
			 * @brief Sets the radius of the circle.
			 * @param radius The radius in engine metrics.
			 * @return void
			 */
			void
			setRadius (precision_t radius) noexcept
			{
				m_radius = std::abs(radius);
			}

			/**
			 * @brief Returns the position of the circle in space.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			position () const noexcept
			{
				return m_position;
			}

			/**
			 * @brief Returns the radius of the circle in engine metrics.
			 * @return data_t
			 */
			[[nodiscard]]
			precision_t
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Returns the radius² of the circle in engine metrics.
			 * @return data_t
			 */
			[[nodiscard]]
			precision_t
			squaredRadius () const noexcept
			{
				return m_radius * m_radius;
			}

			/**
			 * @brief Reset the circle to null value.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_position.reset();
				m_radius = 0;
			}

			/**
			 * @brief Returns the circle perimeter.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			getPerimeter () const noexcept
			{
				return circleCircumference(m_radius);
			}

			/**
			 * @brief Returns the circle area.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			getArea () const noexcept
			{
				return circleArea(m_radius);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Circle & obj)
			{
				return out << "Circle data :\n"
					"Position : " << obj.m_position << "\n"
					"Radius : " << obj.m_radius << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Circle & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Point< precision_t > m_position;
			precision_t m_radius;
	};
}
