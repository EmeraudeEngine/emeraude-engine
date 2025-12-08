/*
 * src/Physics/ContactPoint.cpp
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

#include "ContactPoint.hpp"

/* STL inclusions. */
#include <cmath>

namespace EmEn::Physics
{
	void
	ContactPoint::prepare () noexcept
	{
		/* Compute relative positions from center of mass to contact point. */
		if ( m_bodyA != nullptr )
		{
			m_rA = m_positionWorld - m_bodyA->getWorldCenterOfMass();
		}

		if ( m_bodyB != nullptr )
		{
			m_rB = m_positionWorld - m_bodyB->getWorldCenterOfMass();
		}

		/* Compute tangent basis using Gram-Schmidt orthonormalization.
		 * We need two vectors perpendicular to the normal for friction. */
		if ( std::abs(m_normal[Libs::Math::X]) < 0.9F )
		{
			/* Normal is not aligned with X axis, use X as reference. */
			m_tangent1 = Libs::Math::Vector< 3, float >::crossProduct(m_normal, {1.0F, 0.0F, 0.0F});
		}
		else
		{
			/* Normal is mostly aligned with X axis, use Y as reference. */
			m_tangent1 = Libs::Math::Vector< 3, float >::crossProduct(m_normal, {0.0F, 1.0F, 0.0F});
		}

		m_tangent1.normalize();

		/* Second tangent is perpendicular to both normal and first tangent. */
		m_tangent2 = Libs::Math::Vector< 3, float >::crossProduct(m_normal, m_tangent1);
	}

	void
	ContactPoint::setEffectiveMass (float mass) noexcept
	{
		m_effectiveMass = mass;

		if ( m_effectiveMass > 0.0F )
		{
			m_effectiveMass = 1.0F / m_effectiveMass;
		}
	}

	void
	ContactPoint::updateAccumulatedNormalImpulse (float & lambda) noexcept
	{
		const float oldImpulse = m_accumulatedNormalImpulse;

		m_accumulatedNormalImpulse = std::max(0.0F, oldImpulse + lambda);

		lambda = m_accumulatedNormalImpulse - oldImpulse;
	}

	void
	ContactPoint::updateAccumulatedTangentImpulse (float & lambda, size_t tangentIndex, float maxFriction) noexcept
	{
		const float oldImpulse = m_accumulatedTangentImpulse[tangentIndex];
		const float newImpulse = std::clamp(oldImpulse + lambda, -maxFriction, maxFriction);

		m_accumulatedTangentImpulse[tangentIndex] = newImpulse;

		lambda = newImpulse - oldImpulse;
	}

	void
	ContactPoint::setEffectiveMassTangent1 (float mass) noexcept
	{
		m_effectiveMassTangent1 = mass;

		if ( m_effectiveMassTangent1 > 0.0F )
		{
			m_effectiveMassTangent1 = 1.0F / m_effectiveMassTangent1;
		}
	}

	void
	ContactPoint::setEffectiveMassTangent2 (float mass) noexcept
	{
		m_effectiveMassTangent2 = mass;

		if ( m_effectiveMassTangent2 > 0.0F )
		{
			m_effectiveMassTangent2 = 1.0F / m_effectiveMassTangent2;
		}
	}
}
