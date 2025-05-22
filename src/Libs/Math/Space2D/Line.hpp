/*
* src/Libs/Math/Space2D/Line.hpp
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

/* Local inclusions. */
#include "Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Class for a line in 2D space.
	 * @tparam precision_t The data precision. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Line final
	{
		public:

			/**
			 * @brief Constructs a default line.
			 */
			constexpr Line () noexcept = default;

			/**
			 * @brief Constructs a line from a direction.
			 * @param direction A reference to a vector.
			 */
			explicit
			constexpr
			Line (const Vector< 2, precision_t > & direction) noexcept
				: m_direction(direction.normalized())
			{

			}

			/**
			 * @brief Constructs a line from a point and a direction.
			 * @param origin A reference to a point.
			 * @param direction A reference to a vector.
			 */
			constexpr
			Line (const Point< precision_t > & origin, const Vector< 2, precision_t > & direction) noexcept
				: m_origin(origin),
				m_direction(direction.normalized())
			{

			}

			/**
			 * @brief Sets the line origin.
			 * @param origin A reference to a point.
			 * @return void
			 */
			void
			setOrigin (const Point< precision_t > & origin) noexcept
			{
				m_origin = origin;
			}

			/**
			 * @brief Sets the line direction.
			 * @param direction A reference to a vector.
			 * @return void
			 */
			void
			setDirection (const Vector< 2, precision_t > & direction) noexcept
			{
				m_direction = direction.normalized();
			}

			/**
			 * @brief Returns the line origin.
			 * @return const Point< precision_t > &
			 */
			const Point< precision_t > &
			origin () const noexcept
			{
				return m_origin;
			}

			/**
			 * @brief Returns the line direction.
			 * @return const Vector< 3, precision_t > & direction
			 */
			const Vector< 2, precision_t > &
			direction () const noexcept
			{
				return m_direction;
			}

			/**
			 * @brief Reset the line to null value.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_origin.reset();
				m_direction = Vector< 2, precision_t >::positiveX();
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Line & obj) noexcept
			{
				return out << "Line (2D) data :" "\n"
					"Origin : " << obj.m_origin << "\n"
					"Direction : " << obj.m_direction << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Line & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Point< precision_t > m_origin{0, 0};
			Vector< 3, precision_t > m_direction{1, 0};
	};
}
