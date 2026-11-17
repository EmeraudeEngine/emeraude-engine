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
#include <limits>
#include <numbers>

/* Local inclusions. */
#include "Input/Types.hpp"
#include "Math/Base.hpp"
#include "Math/CartesianFrame.hpp"
#include "Mutex.hpp"
#include "Node.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;

	float
	OrbitController::Orbit::distance () const noexcept
	{
		const auto unclamped = referenceDistance * std::pow(DollyStepFactor, static_cast< float >(dollyStepIndex));

		return std::clamp(unclamped, minimumDistance, maximumDistance);
	}

	float
	OrbitController::clampedElevation (float elevation) noexcept
	{
		constexpr auto ElevationLimit = (std::numbers::pi_v< float > * 0.5F) - PoleGap;

		return std::clamp(elevation, -ElevationLimit, ElevationLimit);
	}

	void
	OrbitController::setTarget (const Vector< 3, float > & target) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_orbit.target = target;
		m_orbitChanged = true;
	}

	Vector< 3, float >
	OrbitController::target () const noexcept
	{
		const MutexLock lock{m_stateAccess};

		return m_orbit.target;
	}

	void
	OrbitController::setDistance (float distance) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_orbit.referenceDistance = std::clamp(distance, m_orbit.minimumDistance, m_orbit.maximumDistance);
		m_orbit.dollyStepIndex = 0;
		m_pendingDollySteps = 0.0F;
		m_orbitChanged = true;
	}

	float
	OrbitController::distance () const noexcept
	{
		const MutexLock lock{m_stateAccess};

		return m_orbit.distance();
	}

	void
	OrbitController::setDistanceLimits (float minimum, float maximum) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_orbit.minimumDistance = std::max(minimum, std::numeric_limits< float >::epsilon());
		m_orbit.maximumDistance = std::max(maximum, m_orbit.minimumDistance);
		m_orbitChanged = true;
	}

	void
	OrbitController::setOrientation (float azimuth, float elevation) noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_orbit.azimuth = getClampedRadian(azimuth);
		m_orbit.elevation = clampedElevation(elevation);
		m_orbitChanged = true;
	}

	float
	OrbitController::azimuth () const noexcept
	{
		const MutexLock lock{m_stateAccess};

		return m_orbit.azimuth;
	}

	float
	OrbitController::elevation () const noexcept
	{
		const MutexLock lock{m_stateAccess};

		return m_orbit.elevation;
	}

	void
	OrbitController::onNodeControlled () noexcept
	{
		/* NOTE: The new node is placed from the canonical state, whatever was applied to the previous one. */
		const MutexLock lock{m_stateAccess};

		m_orbitChanged = true;
	}

	void
	OrbitController::onNodeReleased () noexcept
	{
		const MutexLock lock{m_stateAccess};

		m_dragActive = false;
	}

	bool
	OrbitController::onPointerMove (float positionX, float positionY) noexcept
	{
		if ( !this->hasNode() )
		{
			return false;
		}

		const MutexLock lock{m_stateAccess};

		if ( !m_dragActive )
		{
			return false;
		}

		const auto deltaX = positionX - m_lastPointerX;
		const auto deltaY = positionY - m_lastPointerY;

		m_lastPointerX = positionX;
		m_lastPointerY = positionY;

		/* NOTE: "Grab the object" convention. Dragging right turns the scene
		 * with the cursor, dragging down raises the point of view. */
		m_orbit.azimuth = getClampedRadian(m_orbit.azimuth - (deltaX * RotationStep));
		m_orbit.elevation = clampedElevation(m_orbit.elevation + (deltaY * RotationStep));
		m_orbitChanged = true;

		return true;
	}

	bool
	OrbitController::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Input::Button1Left || !this->hasNode() )
		{
			return false;
		}

		const MutexLock lock{m_stateAccess};

		m_dragActive = true;
		m_lastPointerX = positionX;
		m_lastPointerY = positionY;

		return true;
	}

	bool
	OrbitController::onButtonRelease (float /*positionX*/, float /*positionY*/, int32_t buttonNumber, int32_t /*modifiers*/) noexcept
	{
		if ( buttonNumber != Input::Button1Left )
		{
			return false;
		}

		const MutexLock lock{m_stateAccess};

		if ( !m_dragActive )
		{
			return false;
		}

		m_dragActive = false;

		return true;
	}

	bool
	OrbitController::onMouseWheel (float /*positionX*/, float /*positionY*/, float /*xOffset*/, float yOffset, int32_t /*modifiers*/) noexcept
	{
		if ( !this->hasNode() )
		{
			return false;
		}

		const MutexLock lock{m_stateAccess};

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
			const auto candidateIndex = m_orbit.dollyStepIndex + direction;
			const auto candidateDistance = m_orbit.referenceDistance * std::pow(DollyStepFactor, static_cast< float >(candidateIndex));

			if ( candidateDistance < m_orbit.minimumDistance || candidateDistance > m_orbit.maximumDistance )
			{
				break;
			}

			m_orbit.dollyStepIndex = candidateIndex;
		}

		m_orbitChanged = true;

		return true;
	}

	void
	OrbitController::onUpdate (Node & node) noexcept
	{
		/* The canonical state, taken under the lock when it changed; the node is written without it. */
		Orbit orbit;

		{
			const MutexLock lock{m_stateAccess};

			if ( !m_orbitChanged )
			{
				return;
			}

			orbit = m_orbit;
			m_orbitChanged = false;
		}

		const auto distance = orbit.distance();
		const auto planeRadius = distance * std::cos(orbit.elevation);

		/* NOTE: Y-up world. A positive elevation places the point of view above the target. */
		const Vector< 3, float > position{
			orbit.target[X] + (planeRadius * std::sin(orbit.azimuth)),
			orbit.target[Y] + (distance * std::sin(orbit.elevation)),
			orbit.target[Z] + (planeRadius * std::cos(orbit.azimuth))};

		/* NOTE: Parent space equals world space for a direct child of the scene root. */
		node.setPosition(position, TransformSpace::Parent);
		node.lookAt(orbit.target, false);
	}
}
