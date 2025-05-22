/*
 * src/Scenes/Component/DirectionalPushModifier.cpp
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

#include "DirectionalPushModifier.hpp"

/* Local inclusions. */
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Physics;

	DirectionalPushModifier::DirectionalPushModifier (const std::string & name, const AbstractEntity & parentEntity) noexcept
		: AbstractModifier(name, parentEntity), m_direction(parentEntity.getWorldCoordinates().backwardVector())
	{

	}

	void
	DirectionalPushModifier::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* Updates the force vector using the Node
		 * forward vector and the modifier magnitude. */
		if ( this->isFlagEnabled(UseCoordinatesDirection) )
		{
			m_direction = worldCoordinates.backwardVector();
		}
	}

	void
	DirectionalPushModifier::processLogics (const Scene & scene) noexcept
	{
		this->updateAnimations(scene.cycle());
	}

	bool
	DirectionalPushModifier::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case State :
				this->enable(value.asBool());
				return true;

			case Magnitude :
				this->setMagnitude(value.asFloat());
				return true;

			default:
				return false;
		}
	}

	Vector< 3, float >
	DirectionalPushModifier::getForceAppliedToEntity (const CartesianFrame< float > & worldCoordinates, const Space3D::Sphere< float > & worldBoundingSphere) const noexcept
	{
		if ( !this->isEnabled() )
		{
			return {};
		}

		auto magnitudeValue = m_magnitude;

		if ( this->hasInfluenceArea() )
		{
			const auto influence = this->influenceArea()->influenceStrength(worldCoordinates, worldBoundingSphere);

			if ( influence <= 0.0F )
			{
				return {};
			}

			magnitudeValue *= influence;
		}

		return m_direction.scaled(magnitudeValue);
	}

	Vector< 3, float >
	DirectionalPushModifier::getForceAppliedToEntity (const CartesianFrame< float > & worldCoordinates, const Space3D::AACuboid< float > & worldBoundingBox) const noexcept
	{
		if ( !this->isEnabled() )
		{
			return {};
		}

		auto magnitudeValue = m_magnitude;

		if ( this->hasInfluenceArea() )
		{
			const auto influence = this->influenceArea()->influenceStrength(worldCoordinates, worldBoundingBox);

			if ( influence <= 0.0F )
			{
				return {};
			}

			magnitudeValue *= influence;
		}

		return m_direction.scaled(magnitudeValue);
	}

	void
	DirectionalPushModifier::setCustomDirection (const Vector< 3, float > & direction) noexcept
	{
		m_direction = direction;

		this->enableFlag(UseCoordinatesDirection);
	}

	void
	DirectionalPushModifier::disableCustomDirection () noexcept
	{
		m_direction = this->parentEntity().getWorldCoordinates().backwardVector();

		this->disableFlag(UseCoordinatesDirection);
	}
}
