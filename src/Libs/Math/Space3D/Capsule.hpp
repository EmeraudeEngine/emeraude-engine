/*
 * src/Libs/Math/Space3D/Capsule.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cmath>
#include <numbers>
#include <sstream>
#include <string>

/* Local inclusions for usages. */
#include "Segment.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Class for a capsule (swept sphere / stadium solid) in 3D space.
	 * @note A capsule is defined by a line segment (axis) and a radius,
	 *	   forming a cylinder with hemispherical caps at each end.
	 * @tparam precision_t The precision type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Capsule final
	{
		public:

			/**
			 * @brief Constructs a default capsule (invalid).
			 */
			constexpr Capsule () noexcept = default;

			/**
			 * @brief Constructs a capsule with radius only (degenerate to sphere at origin).
			 * @param radius The radius of the capsule.
			 */
			explicit
			constexpr
			Capsule (precision_t radius) noexcept
				: m_radius{std::abs(radius)}
			{

			}

			/**
			 * @brief Constructs a capsule from two endpoints and a radius.
			 * @param startPoint First endpoint of the central axis.
			 * @param endPoint Second endpoint of the central axis.
			 * @param radius The radius of the capsule.
			 */
			constexpr
			Capsule (const Point< precision_t > & startPoint, const Point< precision_t > & endPoint, precision_t radius) noexcept
				: m_axis{startPoint, endPoint},
				m_radius{std::abs(radius)}
			{

			}

			/**
			 * @brief Constructs a capsule from a segment and a radius.
			 * @param axis The central axis segment.
			 * @param radius The radius of the capsule.
			 */
			constexpr
			Capsule (const Segment< precision_t > & axis, precision_t radius) noexcept
				: m_axis{axis},
				m_radius{std::abs(radius)}
			{

			}

			/**
			 * @brief Checks if the capsule is valid.
			 * @note A capsule is valid if it has a positive radius.
			 *	   A degenerate capsule (zero-length axis) is still valid and behaves as a sphere.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				return m_radius > 0;
			}

			/**
			 * @brief Returns whether the capsule is degenerate (zero-length axis).
			 * @note A degenerate capsule behaves as a sphere centered at the start point.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDegenerate () const noexcept
			{
				return !m_axis.isValid();
			}

			/**
			 * @brief Returns the central axis segment.
			 * @return const Segment< precision_t > &
			 */
			[[nodiscard]]
			const Segment< precision_t > &
			axis () const noexcept
			{
				return m_axis;
			}

			/**
			 * @brief Returns the central axis segment (mutable).
			 * @return Segment< precision_t > &
			 */
			[[nodiscard]]
			Segment< precision_t > &
			axis () noexcept
			{
				return m_axis;
			}

			/**
			 * @brief Sets the central axis.
			 * @param axis The new axis segment.
			 * @return void
			 */
			void
			setAxis (const Segment< precision_t > & axis) noexcept
			{
				m_axis = axis;
			}

			/**
			 * @brief Returns the start point of the axis.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			startPoint () const noexcept
			{
				return m_axis.startPoint();
			}

			/**
			 * @brief Returns the end point of the axis.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			endPoint () const noexcept
			{
				return m_axis.endPoint();
			}

			/**
			 * @brief Sets the start point of the axis.
			 * @param point The new start point.
			 * @return void
			 */
			void
			setStartPoint (const Point< precision_t > & point) noexcept
			{
				m_axis.setStart(point);
			}

			/**
			 * @brief Sets the end point of the axis.
			 * @param point The new end point.
			 * @return void
			 */
			void
			setEndPoint (const Point< precision_t > & point) noexcept
			{
				m_axis.setEnd(point);
			}

			/**
			 * @brief Returns the radius.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Returns the squared radius.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			squaredRadius () const noexcept
			{
				return m_radius * m_radius;
			}

			/**
			 * @brief Sets the radius.
			 * @param radius The new radius (absolute value taken).
			 * @return void
			 */
			void
			setRadius (precision_t radius) noexcept
			{
				m_radius = std::abs(radius);
			}

			/**
			 * @brief Returns the length of the central axis.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getAxisLength () const noexcept
			{
				return m_axis.getLength();
			}

			/**
			 * @brief Returns the total height (axis length + 2 * radius).
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getTotalHeight () const noexcept
			{
				return m_axis.getLength() + (static_cast< precision_t >(2) * m_radius);
			}

			/**
			 * @brief Returns the centroid (midpoint) of the capsule.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			Point< precision_t >
			centroid () const noexcept
			{
				return (m_axis.startPoint() + m_axis.endPoint()) * static_cast< precision_t >(0.5);
			}

			/**
			 * @brief Returns the capsule volume.
			 * @note V = pi * r^2 * (4/3 * r + h) where h is axis length.
			 *	   This is the volume of a cylinder + sphere.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getVolume () const noexcept
			{
				const auto h = m_axis.getLength();
				const auto r = m_radius;

				/* Cylinder volume: pi * r^2 * h
				 * Sphere volume: 4/3 * pi * r^3
				 * Total: pi * r^2 * (h + 4/3 * r) */
				return std::numbers::pi_v< precision_t > * r * r * (h + (static_cast< precision_t >(4.0 / 3.0) * r));
			}

			/**
			 * @brief Resets the capsule to default state (invalid).
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_axis.reset();
				m_radius = 0;
			}

			/**
			 * @brief Returns the closest point on the capsule axis to a given point.
			 * @param point The external point.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			Point< precision_t >
			closestPointOnAxis (const Point< precision_t > & point) const noexcept
			{
				/* NOTE: If the axis is degenerate (zero length), return the start point. */
				if ( !m_axis.isValid() )
				{
					return m_axis.startPoint();
				}

				const auto & a = m_axis.startPoint();
				const auto & b = m_axis.endPoint();
				const auto ab = b - a;
				const auto ap = point - a;

				const auto lengthSq = ab.lengthSquared();

				/* NOTE: Project point onto line and clamp to segment. */
				const precision_t t = Vector< 3, precision_t >::dotProduct(ap, ab) / lengthSq;
				const precision_t clampedT = std::clamp(t, static_cast< precision_t >(0), static_cast< precision_t >(1));

				return a + (ab * clampedT);
			}

			/**
			 * @brief Returns the squared distance from a point to the capsule surface.
			 * @note Negative value means the point is inside the capsule.
			 * @param point The external point.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			squaredDistanceToSurface (const Point< precision_t > & point) const noexcept
			{
				const auto closest = this->closestPointOnAxis(point);
				const auto distToAxis = Vector< 3, precision_t >::distance(point, closest);

				return (distToAxis - m_radius) * (distToAxis - m_radius);
			}

			/**
			 * @brief Checks if a point is inside the capsule.
			 * @param point The point to test.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			contains (const Point< precision_t > & point) const noexcept
			{
				const auto closest = this->closestPointOnAxis(point);

				return Vector< 3, precision_t >::distanceSquared(point, closest) <= this->squaredRadius();
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Capsule & obj) noexcept
			{
				return out <<
					"Capsule volume data :\n"
					"Start point : " << obj.m_axis.startPoint() << "\n"
					"End point : " << obj.m_axis.endPoint() << "\n"
					"Radius : " << obj.m_radius << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Capsule & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Segment< precision_t > m_axis{};
			precision_t m_radius{0};
	};
}
