/*
 * src/Scenes/Component/Weight.cpp
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

#include "Weight.hpp"

/* STL inclusions. */
#include <cstddef>
#include <string>

/* Local inclusions. */
#include "Physics/PhysicalObjectProperties.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

	Weight::Weight (const std::string & name, const AbstractEntity & parentEntity) noexcept
		: Abstract(name, parentEntity)
	{

	}

	Weight::Weight (const std::string & name, const AbstractEntity & parentEntity, const Physics::PhysicalObjectProperties & initialProperties) noexcept
		: Abstract(name, parentEntity)
	{
		this->physicalObjectProperties().setProperties(initialProperties);
	}

	void
	Weight::move (const CartesianFrame< float > & /*worldCoordinates*/) noexcept
	{

	}

	void
	Weight::processLogics (const Scene & scene) noexcept
	{
		this->updateAnimations(scene.cycle());
	}

	bool
	Weight::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case Mass :
				this->physicalObjectProperties().setMass(value.asFloat());
				return true;

			case Surface :
				this->physicalObjectProperties().setSurface(value.asFloat());
				return true;

			case DragCoefficient :
				this->physicalObjectProperties().setDragCoefficient(value.asFloat());
				return true;

			case Bounciness :
				this->physicalObjectProperties().setBounciness(value.asFloat());
				return true;

			case Stickiness :
				this->physicalObjectProperties().setStickiness(value.asFloat());
				return true;

			default :
				return false;
		}
	}

	void
	Weight::setRadius (float radius) noexcept
	{
		m_boundingSphere.setRadius(radius);

		this->notify(ComponentContentModified);
	}

	void
	Weight::setBoxSize (float size) noexcept
	{
		m_boundingBox.set(size * 0.5F);

		this->notify(ComponentContentModified);
	}

	void
	Weight::setBoxSize (float xSize, float ySize, float zSize) noexcept
	{
		m_boundingBox.set({xSize * 0.5F, ySize * 0.5F, zSize * 0.5F}, {-xSize * 0.5F, -ySize * 0.5F, -zSize * 0.5F});

		this->notify(ComponentContentModified);
	}
}
