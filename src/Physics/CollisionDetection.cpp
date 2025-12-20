/*
 * src/Physics/CollisionDetection.cpp
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

#include "CollisionDetection.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Physics/MovableTrait.hpp"
#include "Physics/ContactManifold.hpp"
#include "Physics/CollisionModelInterface.hpp"

namespace EmEn::Physics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Scenes;

	bool
	detectCollisionMovableToMovable (AbstractEntity & movableEntityA, AbstractEntity & movableEntityB, std::vector< ContactManifold > & outManifolds) noexcept
	{
		auto * movableA = movableEntityA.getMovableTrait();
		auto * movableB = movableEntityB.getMovableTrait();

		if ( movableA == nullptr || movableB == nullptr )
		{
			return false;
		}

		/* Both entities must have collision models. */
		if ( !movableEntityA.hasCollisionModel() || !movableEntityB.hasCollisionModel() )
		{
			return false;
		}

		const auto * modelA = movableEntityA.collisionModel();
		const auto * modelB = movableEntityB.collisionModel();

		const auto worldFrameA = movableEntityA.getWorldCoordinates();
		const auto worldFrameB = movableEntityB.getWorldCoordinates();

		/* Use the unified collision detection interface. */
		const auto results = modelA->isCollidingWith(worldFrameA, *modelB, worldFrameB);

		if ( !results.m_collisionDetected )
		{
			return false;
		}

		/* Create contact manifold for both movable bodies. */
		ContactManifold manifold{movableA, movableB};

		/* Add contact from collision results. */
		manifold.addContact(results.m_contact, results.m_impactNormal, results.m_depth);

		/* Add manifold to output vector. */
		outManifolds.push_back(manifold);

		return true;
	}

	bool
	detectCollisionMovableToStatic (AbstractEntity & movableEntity, const AbstractEntity & staticEntity, std::vector< ContactManifold > & outManifolds) noexcept
	{
		auto * movable = movableEntity.getMovableTrait();

		if ( movable == nullptr )
		{
			return false;
		}

		/* Both entities must have collision models. */
		if ( !movableEntity.hasCollisionModel() || !staticEntity.hasCollisionModel() )
		{
			return false;
		}

		const auto * modelA = movableEntity.collisionModel();
		const auto * modelB = staticEntity.collisionModel();

		const auto worldFrameA = movableEntity.getWorldCoordinates();
		const auto worldFrameB = staticEntity.getWorldCoordinates();

		/* Use the unified collision detection interface. */
		const auto results = modelA->isCollidingWith(worldFrameA, *modelB, worldFrameB);

		if ( !results.m_collisionDetected )
		{
			return false;
		}

		/* Create contact manifold with only the movable body (static has infinite mass). */
		ContactManifold manifold(movable);

		/* Add contact from collision results.
		 * Invert normal since we want it pointing from movable towards static. */
		manifold.addContact(results.m_contact, -results.m_impactNormal, results.m_depth);

		/* Add manifold to output vector. */
		outManifolds.push_back(manifold);

		return true;
	}
}
