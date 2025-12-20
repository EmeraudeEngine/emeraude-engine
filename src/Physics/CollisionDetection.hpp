/*
 * src/Physics/CollisionDetection.hpp
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
#include <vector>

/* Forward declarations. */
namespace EmEn
{
	namespace Physics
	{
		class ContactManifold;
	}

	namespace Scenes
	{
		class AbstractEntity;
	}
}

namespace EmEn::Physics
{
	/**
	 * @brief Detects collision between two movable entities and creates a contact manifold.
	 *
	 * Uses the entities' CollisionModelInterface for unified collision detection.
	 * Both entities must have collision models set via setCollisionModel().
	 *
	 * @param movableEntityA Reference to first movable entity.
	 * @param movableEntityB Reference to second movable entity.
	 * @param outManifolds Vector to store generated manifolds.
	 * @return bool True if collision detected.
	 */
	bool detectCollisionMovableToMovable (Scenes::AbstractEntity & movableEntityA, Scenes::AbstractEntity & movableEntityB, std::vector< ContactManifold > & outManifolds) noexcept;

	/**
	 * @brief Detects collision between a movable entity and a static entity, and creates a contact manifold.
	 *
	 * Uses the entities' CollisionModelInterface for unified collision detection.
	 * Both entities must have collision models set via setCollisionModel().
	 *
	 * @param movableEntity Reference to a movable entity.
	 * @param staticEntity Reference to a static entity.
	 * @param outManifolds Vector to store generated manifolds.
	 * @return bool True if collision detected.
	 */
	bool detectCollisionMovableToStatic (Scenes::AbstractEntity & movableEntity, const Scenes::AbstractEntity & staticEntity, std::vector< ContactManifold > & outManifolds) noexcept;
}
