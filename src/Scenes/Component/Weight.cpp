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
	using namespace Libs;
	using namespace Libs::Math;

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
}
