/*
 * src/Libs/Math/Space3D/Sphere.hpp
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

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Class for a sphere in 3D space.
	 * @tparam precision_t The precision type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Sphere final
	{
		public:

			/**
			 * @brief Constructs a default sphere.
			 */
			constexpr Sphere () noexcept = default;

			/**
			 * @brief Constructs a sphere volume.
			 * @param radius The radius of the sphere.
			 */
			explicit
			constexpr
			Sphere (precision_t radius) noexcept
				: m_radius(radius)
			{

			}

			/**
			 * @brief Constructs a sphere volume at a location.
			 * @param radius The radius of the sphere.
			 * @param position A reference to a position vector.
			 */
			constexpr
			Sphere (precision_t radius, const Vector< 3, precision_t > & position) noexcept
				: m_position(position), m_radius(radius)
			{

			}

			/**
			 * @brief Checks if the sphere volume consistency.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_radius > 0;
			}

			/**
			 * @brief Sets a position.
			 * @param position A reference to a position vector.
			 * @return void
			 */
			void
			setPosition (const Vector< 3, precision_t > & position) noexcept
			{
				m_position = position;
			}

			/**
			 * @brief Returns the position.
			 * @return const Vector< 3, data_t > &
			 */
			[[nodiscard]]
			const Vector< 3, precision_t > &
			position () const noexcept
			{
				return m_position;
			}

			/**
			 * @brief Sets the radius of the sphere.
			 * @param radius The radius of the sphere.
			 * @return void
			 */
			void
			setRadius (precision_t radius) noexcept
			{
				m_radius = std::abs(radius);
			}

			/**
			 * @brief Returns the radius of the sphere.
			 * @return data_t
			 */
			[[nodiscard]]
			precision_t
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Returns the radius² of the sphere.
			 * @return data_t
			 */
			[[nodiscard]]
			precision_t
			squaredRadius () const noexcept
			{
				return m_radius * m_radius;
			}

			/**
			 * @brief Reset the sphere to null value.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_position.reset();
				m_radius = 0;
			}

			/**
			 * @brief Extends the volume of this sphere with another one.
			 * @param other A reference to a sphere.
			 * @return void
			 */
			void
			merge (const Sphere< precision_t > & other) noexcept
			{
				if ( this == &other || !other.isValid() )
				{
					return;
				}

				if ( !this->isValid() )
				{
					*this = other;

					return;
				}

				const auto centerToCenter = other.m_position - m_position;
				const precision_t distance_sq = centerToCenter.lengthSquared();
				const precision_t distance = std::sqrt(distance_sq);

				/* NOTE: One sphere is entirely contained within the other. */
				if ( distance + other.m_radius <= m_radius )
				{
					return;
				}

				if ( distance + m_radius <= other.m_radius )
				{
					*this = other;

					return;
				}

				/* NOTE: The new radius is half the total distance from one extreme to the other. */
				const precision_t newRadius = (distance + m_radius + other.m_radius) * static_cast< precision_t >(0.5);

				/* NOTE: The new center is located on the line connecting the old centers.
				 * It is offset from the center of the 'this' sphere to 'other'. The direction is normalized.
				 * We handle the case where the distance is almost zero. */
				const auto direction = centerToCenter / (distance > std::numeric_limits< precision_t >::epsilon() ? distance : 1.0f);
				m_position = m_position + direction * (newRadius - m_radius);

				m_radius = newRadius;
			}

			/**
			 * @brief Returns the sphere perimeter.
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
			 * @brief Returns the sphere area (2D).
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
			 * @return Returns the sphere volume.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getVolume () const noexcept
			{
				return sphereVolume(m_radius);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Sphere & obj) noexcept
			{
				return out <<
					"Sphere volume data :\n"
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
			to_string (const Sphere & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Vector< 3, precision_t > m_position;
			precision_t m_radius{0};
	};
}
