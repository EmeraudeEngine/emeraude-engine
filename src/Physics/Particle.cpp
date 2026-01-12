/*
 * src/Physics/Particle.cpp
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

#include "Particle.hpp"

/* Local inclusions. */
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Physics
{
	using namespace Libs;
	using namespace Libs::Math;

	void
	Particle::initialize (uint32_t initialLife, float initialSize, float spreadingRadius, const CartesianFrame< float > & initialLocation) noexcept
	{
		/* Removing velocity */
		m_linearVelocity.reset();

		/* Reset basic properties. */
		m_lifetime = initialLife;
		m_size = std::abs(initialSize);

		/* Reset the particles coordinates. */
		m_cartesianFrame = initialLocation;
		if ( !Utility::isZero(spreadingRadius)  )
		{
			this->shiftLocation(spreadingRadius);
		}
		m_cartesianFrame.setScalingFactor(m_size);
	}

	void
	Particle::setPosition (const Vector< 3, float > & position, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setPosition(position);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setPosition(position);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setPosition(position);
				break;
		}
	}

	void
	Particle::setXPosition (float position, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setXPosition(position);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setXPosition(position);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setXPosition(position);
				break;
		}
	}

	void
	Particle::setYPosition (float position, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setYPosition(position);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setYPosition(position);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setYPosition(position);
				break;
		}
	}

	void
	Particle::setZPosition (float position, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setZPosition(position);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setZPosition(position);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setZPosition(position);
				break;
		}
	}

	void
	Particle::move (const Vector< 3, float > & distance, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translate(distance, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.translate(distance, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.translate(distance, false);
				break;
		}
	}

	void
	Particle::moveX (float distance, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateX(distance, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.translateX(distance, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.translateX(distance, false);
				break;
		}
	}

	void
	Particle::moveY (float distance, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateY(distance, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.translateY(distance, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.translateY(distance, false);
				break;
		}
	}

	void
	Particle::moveZ (float distance, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateZ(distance, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.translateZ(distance, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.translateZ(distance, false);
				break;
		}
	}

	void
	Particle::rotate (float radian, const Vector< 3, float > & axis, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.rotate(radian, axis, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.rotate(radian, axis, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.rotate(radian, axis, false);
				break;
		}
	}

	void
	Particle::pitch (float radian, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.pitch(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.pitch(radian, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.pitch(radian, false);
				break;
		}
	}

	void
	Particle::yaw (float radian, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.yaw(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.yaw(radian, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.yaw(radian, false);
				break;
		}
	}

	void
	Particle::roll (float radian, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.roll(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.roll(radian, false);
				break;

			case TransformSpace::World :
				m_cartesianFrame.roll(radian, false);
				break;
		}
	}

	void
	Particle::scale (const Vector< 3, float > & factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setScalingFactor(factor);
				break;
		}
	}

	void
	Particle::scale (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setScalingFactor(factor);
				break;
		}
	}

	void
	Particle::scaleX (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingXFactor(factor);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setScalingXFactor(factor);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setScalingXFactor(factor);
				break;
		}
	}

	void
	Particle::scaleY (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingYFactor(factor);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setScalingYFactor(factor);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setScalingYFactor(factor);
				break;
		}
	}

	void
	Particle::scaleZ (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingZFactor(factor);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.setScalingZFactor(factor);
				break;

			case TransformSpace::World :
				m_cartesianFrame.setScalingZFactor(factor);
				break;
		}
	}

	void
	Particle::shiftLocation (float magnitude) noexcept
	{
		const auto half = magnitude * 0.5F;

		/* Sets chaos on position. */
		m_cartesianFrame.translate(
			Utility::quickRandom(-half, +half),
			Utility::quickRandom(-half, +half),
			Utility::quickRandom(-half, +half),
			true
		);
	}

	bool
	Particle::updateSimulation (const Scenes::Scene & scene, const BodyPhysicalProperties & particleProperties, const CartesianFrame< float > & worldCoordinates) noexcept
	{
		if ( particleProperties.isMassNull() )
		{
			return false;
		}

		/* Apply scene modifiers. */
		scene.forEachModifiers([this, &worldCoordinates, &particleProperties] (const auto & modifier) {
			const auto force = modifier.getForceAppliedTo(worldCoordinates, m_size * 0.5F);

			m_linearVelocity += force * particleProperties.inverseMass() * EngineUpdateCycleDurationS< float >;
		});

		const EnvironmentPhysicalProperties & envProperties = scene.physicalEnvironmentProperties();

		/* Apply gravity. */
		m_linearVelocity[Y] += envProperties.steppedSurfaceGravity();

		/* Apply drag force. */
		const auto linearSpeed = this->linearSpeed();

		if ( linearSpeed <= 0.0F )
		{
			return false;
		}

		const auto dragMagnitude = Physics::getDragMagnitude(
			particleProperties.dragCoefficient(),
			envProperties.atmosphericDensity(),
			linearSpeed,
			particleProperties.surface()
		);

		const auto force = m_linearVelocity.normalized().scale(-dragMagnitude);

		m_linearVelocity += force * particleProperties.inverseMass() * EngineUpdateCycleDurationS< float >;

		return true;
	}

	void
	Particle::update (const Vector< 3, float > & velocity, float sizeDelta, float chaos) noexcept
	{
		if ( m_lifetime >= EngineUpdateCycleDurationMS< uint32_t > )
		{
			m_lifetime -= EngineUpdateCycleDurationMS< uint32_t >;
		}
		else
		{
			m_lifetime = 0;
		}

		/* Checks the new size if requested. */
		if ( !Utility::isZero(sizeDelta) )
		{
			/* Updates the current size. */
			m_size += sizeDelta;

			/* NOTE: Kill the particle if below 0.0F. */
			if ( m_size < 0.0F )
			{
				m_lifetime = 0;

				return;
			}

			m_cartesianFrame.setScalingFactor(m_size);
		}

		if ( !this->isDead() )
		{
			m_linearVelocity = velocity;

			/* Apply the velocity vector to particle position. */
			m_cartesianFrame.translate(m_linearVelocity * EngineUpdateCycleDurationS< float >, false);

			if ( !Utility::isZero(chaos) )
			{
				this->shiftLocation(chaos);
			}
		}
	}
}
