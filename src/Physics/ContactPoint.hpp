/*
 * src/Physics/ContactPoint.hpp
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

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "MovableTrait.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Represents a single contact point between two bodies in a collision.
	 * @note This structure stores contact geometry and accumulated impulses for iterative solving.
	 */
	class ContactPoint final
	{
		public:

			/**
			 * @brief Constructs a default contact point.
			 */
			constexpr
			ContactPoint () noexcept = default;

			/**
			 * @brief Constructs a contact point with geometry.
			 * @param worldPosition The contact position in world space.
			 * @param worldNormal The contact normal from body A to body B (unit vector).
			 * @param depth The penetration depth (positive when penetrating).
			 * @param bodyA Pointer to the first movable body.
			 * @param bodyB Pointer to the second movable body.
			 */
			constexpr
			ContactPoint (const Libs::Math::Vector< 3, float > & worldPosition, const Libs::Math::Vector< 3, float > & worldNormal, float depth, MovableTrait * bodyA, MovableTrait * bodyB) noexcept
				: m_positionWorld{worldPosition},
				  m_normal{worldNormal},
				  m_penetrationDepth{depth},
				  m_bodyA{bodyA},
				  m_bodyB{bodyB}
			{

			}

			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			positionWorld () const noexcept
			{
				return m_positionWorld;
			}

			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			normal () const noexcept
			{
				return m_normal;
			}

			[[nodiscard]]
			float
			penetrationDepth () const noexcept
			{
				return m_penetrationDepth;
			}

			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			rA () const noexcept
			{
				return m_rA;
			}

			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			rB () const noexcept
			{
				return m_rB;
			}

			[[nodiscard]]
			MovableTrait *
			bodyA () const noexcept
			{
				return m_bodyA;
			}

			[[nodiscard]]
			MovableTrait *
			bodyB () const noexcept
			{
				return m_bodyB;
			}

			void prepare () noexcept;

			void setEffectiveMass (float mass) noexcept;

			[[nodiscard]]
			float
			effectiveMass () const noexcept
			{
				return m_effectiveMass;
			}

			void
			setVelocityBias (float bias) noexcept
			{
				m_velocityBias = bias;
			}

			[[nodiscard]]
			float
			velocityBias () const noexcept
			{
				return m_velocityBias;
			}

			void updateAccumulatedNormalImpulse (float & lambda) noexcept;

			/**
			 * @brief Resets accumulated impulses (for new frame).
			 */
			void
			reset () noexcept
			{
				m_accumulatedNormalImpulse = 0.0F;
				m_accumulatedTangentImpulse[0] = 0.0F;
				m_accumulatedTangentImpulse[1] = 0.0F;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const ContactPoint & obj);

			/* Contact geometry */
			Libs::Math::Vector< 3, float > m_positionWorld;   ///< Contact point in world space
			Libs::Math::Vector< 3, float > m_normal;          ///< Contact normal (from A to B)
			float m_penetrationDepth{0.0F};                   ///< Penetration depth (positive = penetrating)

			/* Contact relative positions (computed once per manifold) */
			Libs::Math::Vector< 3, float > m_rA;              ///< Vector from body A center of mass to contact
			Libs::Math::Vector< 3, float > m_rB;              ///< Vector from body B center of mass to contact

			/* Bodies involved */
			MovableTrait * m_bodyA{nullptr};                  ///< First body (can be nullptr for static)
			MovableTrait * m_bodyB{nullptr};                  ///< Second body (can be nullptr for static)

			/* Accumulated impulses (for warm starting and clamping) */
			float m_accumulatedNormalImpulse{0.0F};           ///< Accumulated impulse along normal
			std::array< float, 2 > m_accumulatedTangentImpulse{0.0F, 0.0F}; ///< Accumulated friction impulses (2D tangent space)

			/* Solver cache (computed per iteration) */
			float m_effectiveMass{0.0F};                      ///< Cached 1/(K_normal) for this contact
			float m_velocityBias{0.0F};                       ///< Baumgarte stabilization bias
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const ContactPoint & obj)
	{
		return out <<
			"Position : " << obj.m_positionWorld << "\n"
			"Normal : " << obj.m_normal << "\n"
			"Penetration depth : " << obj.m_penetrationDepth << "\n";
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const ContactPoint & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
