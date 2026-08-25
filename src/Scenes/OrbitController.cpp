/*
 * src/Scenes/OrbitController.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 */

#include "OrbitController.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numbers>

/* Local inclusions. */
#include "Input/Types.hpp"
#include "Math/Base.hpp"
#include "Node.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;

	void
	OrbitController::setTarget (const Vector< 3, float > & target) noexcept
	{
		m_target = target;

		this->applyToNode();
	}

	void
	OrbitController::setDistance (float distance) noexcept
	{
		m_referenceDistance = std::clamp(distance, m_minimumDistance, m_maximumDistance);
		m_dollyStepIndex = 0;
		m_pendingDollySteps = 0.0F;

		this->applyToNode();
	}

	float
	OrbitController::distance () const noexcept
	{
		const auto distance = m_referenceDistance * std::pow(DollyStepFactor, static_cast< float >(m_dollyStepIndex));

		return std::clamp(distance, m_minimumDistance, m_maximumDistance);
	}

	void
	OrbitController::setDistanceLimits (float minimum, float maximum) noexcept
	{
		m_minimumDistance = std::max(minimum, std::numeric_limits< float >::epsilon());
		m_maximumDistance = std::max(maximum, m_minimumDistance);

		this->applyToNode();
	}

	void
	OrbitController::setOrientation (float azimuth, float elevation) noexcept
	{
		constexpr auto ElevationLimit = std::numbers::pi_v< float > * 0.5F - PoleGap;

		m_azimuth = getClampedRadian(azimuth);
		m_elevation = std::clamp(elevation, -ElevationLimit, ElevationLimit);

		this->applyToNode();
	}

	bool
	OrbitController::onPointerMove (float positionX, float positionY) noexcept
	{
		if ( !m_dragActive || m_controlledNode == nullptr )
		{
			return false;
		}

		const auto deltaX = positionX - m_lastPointerX;
		const auto deltaY = positionY - m_lastPointerY;

		m_lastPointerX = positionX;
		m_lastPointerY = positionY;

		/* NOTE: "Grab the object" convention. Dragging right turns the scene
		 * with the cursor, dragging down raises the point of view. */
		this->setOrientation(m_azimuth - deltaX * RotationStep, m_elevation + deltaY * RotationStep);

		return true;
	}

	bool
	OrbitController::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( m_controlledNode == nullptr || buttonNumber != Input::Button1Left )
		{
			return false;
		}

		m_dragActive = true;
		m_lastPointerX = positionX;
		m_lastPointerY = positionY;

		return true;
	}

	bool
	OrbitController::onButtonRelease (float /*positionX*/, float /*positionY*/, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( !m_dragActive || buttonNumber != Input::Button1Left )
		{
			return false;
		}

		m_dragActive = false;

		return true;
	}

	bool
	OrbitController::onMouseWheel (float /*positionX*/, float /*positionY*/, float /*xOffset*/, float yOffset, int32_t /*modifiers*/) noexcept
	{
		if ( m_controlledNode == nullptr )
		{
			return false;
		}

		/* NOTE: Whole wheel notches move the dolly, fractional motion
		 * (touchpads) is kept until a whole step is reached. */
		m_pendingDollySteps += yOffset;

		const auto steps = static_cast< int32_t >(m_pendingDollySteps);

		if ( steps == 0 )
		{
			return true;
		}

		m_pendingDollySteps -= static_cast< float >(steps);

		/* NOTE: Wheel up brings the point of view closer. The distance always derives
		 * from the reference distance and an integer step index, one step at a time,
		 * stopping at the limits so the index never drifts past them. */
		const auto direction = steps > 0 ? -1 : 1;

		for ( int32_t stepCount = std::abs(steps); stepCount > 0; stepCount-- )
		{
			const auto candidateIndex = m_dollyStepIndex + direction;
			const auto candidateDistance = m_referenceDistance * std::pow(DollyStepFactor, static_cast< float >(candidateIndex));

			if ( candidateDistance < m_minimumDistance || candidateDistance > m_maximumDistance )
			{
				break;
			}

			m_dollyStepIndex = candidateIndex;
		}

		this->applyToNode();

		return true;
	}

	void
	OrbitController::applyToNode () noexcept
	{
		if ( m_controlledNode == nullptr )
		{
			return;
		}

		const auto distance = this->distance();
		const auto planeRadius = distance * std::cos(m_elevation);

		/* NOTE: Y-up world. A positive elevation places the point of view above the target. */
		const Vector< 3, float > position{
			m_target[X] + planeRadius * std::sin(m_azimuth),
			m_target[Y] + distance * std::sin(m_elevation),
			m_target[Z] + planeRadius * std::cos(m_azimuth)
		};

		/* NOTE: Parent space equals world space for a direct child of the scene root. */
		m_controlledNode->setPosition(position, TransformSpace::Parent);
		m_controlledNode->lookAt(m_target, false);
	}
}
