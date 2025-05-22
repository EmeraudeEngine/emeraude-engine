/*
 * src/Physics/Collider.cpp
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

#include "Collider.hpp"

/* Local inclusions. */
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/OrientedCuboid.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Physics/MovableTrait.hpp"

namespace EmEn::Physics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Scenes;

	bool
	Collider::checkCollisionAgainstMovable (AbstractEntity & movableEntityA, AbstractEntity & movableEntityB) noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( &movableEntityA == &movableEntityB )
			{
				Tracer::error(ClassId, "Collision search on the same entity detected !");

				return false;
			}
		}

		Vector< 3, float > minimalTranslationVector;
		auto collisionOverflow = 0.0F;

		/* TODO: Add sphere <> box collision. */
		if ( movableEntityA.sphereCollisionIsEnabled() && movableEntityB.sphereCollisionIsEnabled() )
		{
			const auto sphereA = movableEntityA.getWorldBoundingSphere();
			const auto sphereB = movableEntityB.getWorldBoundingSphere();

			if ( !Space3D::isColliding(sphereA, sphereB, minimalTranslationVector) )
			{
				return false;
			}

			TraceDebug{ClassId} << "Sphere (" << movableEntityB.name() << ") to sphere (" << movableEntityA.name() << ") collision !\nMTV : " << minimalTranslationVector;

			movableEntityA.move(-minimalTranslationVector, TransformSpace::World);
		}
		else
		{
			if ( !isBoxCollisionWith(movableEntityB, movableEntityA, collisionOverflow, minimalTranslationVector) )
			{
				return false;
			}

			TraceDebug{ClassId} << "Box (" << movableEntityB.name() << ") to box (" << movableEntityA.name() << ") collision !\nMTV : " << minimalTranslationVector;
		}

		/* NOTE: Collision mid-point must be done after the move back! */
		const auto collisionPosition = Vector< 3, float >::midPoint(
			movableEntityA.getWorldCoordinates().position(),
			movableEntityB.getWorldCoordinates().position()
		);

		this->addCollision(CollisionType::StaticEntity, &movableEntityB, collisionPosition, -minimalTranslationVector.normalize());
		
		return true;
	}
	
	bool
	Collider::checkCollisionAgainstStatic (AbstractEntity & movableEntityA, AbstractEntity & staticEntityB) noexcept
	{
		Vector< 3, float > collisionDirection;
		auto collisionOverflow = 0.0F;

		/* TODO: Add sphere <> box collision. */
		if ( movableEntityA.sphereCollisionIsEnabled() && staticEntityB.sphereCollisionIsEnabled() )
		{
			//TraceDebug{ClassId} << "Sphere (" << staticEntityB.name() << ") to sphere (" << movableEntityA.name() << ") collision ...";

			if ( !isSphereCollisionWith(staticEntityB, movableEntityA, collisionOverflow, collisionDirection) )
			{
				return false;
			}
		}
		else
		{
			//TraceDebug{ClassId} << "Box (" << staticEntityB.name() << ") to box (" << movableEntityA.name() << ") collision ...";

			if ( !isBoxCollisionWith(staticEntityB, movableEntityA, collisionOverflow, collisionDirection) )
			{
				return false;
			}
		}

		/* NOTE: Location correction. */
		movableEntityA.move(collisionDirection.scaled(collisionOverflow), TransformSpace::World);

		/* NOTE: Collision declaration. */
		const auto collisionPosition = Vector< 3, float >::midPoint(
			movableEntityA.getWorldCoordinates().position(),
			staticEntityB.getWorldCoordinates().position()
		);

		this->addCollision(CollisionType::StaticEntity, &staticEntityB, collisionPosition, collisionDirection);

		return true;
	}

	void
	Collider::resolveCollisions (AbstractEntity & targetEntity) noexcept
	{
		for ( const auto & collision : m_collisions )
		{
			switch ( collision.type() )
			{
				/* TODO: Get physical properties from the scene. */
				case CollisionType::SceneBoundary :
				{
					auto * movableTrait = targetEntity.getMovableTrait();
					const auto speedAtHit = movableTrait->deflect(collision.direction(), 1.0F);

					movableTrait->onHit(speedAtHit * targetEntity.physicalObjectProperties().mass());
				}
					break;

				/* TODO: Get physical properties from the scene ground. */
				case CollisionType::SceneGround :
				{
					auto * movableTrait = targetEntity.getMovableTrait();
					const auto speedAtHit = movableTrait->deflect(collision.direction(), 0.5F);

					movableTrait->onHit(speedAtHit * targetEntity.physicalObjectProperties().mass());
				}
					break;

				/* NOTE: As the main entity is static, we only modify the collided movable trajectory. */
				case CollisionType::StaticEntity :
				{
					auto * movableTrait = targetEntity.getMovableTrait();
					const auto speedAtHit = movableTrait->deflect(collision.direction(), collision.entity()->physicalObjectProperties().bounciness());

					movableTrait->onHit(speedAtHit * targetEntity.physicalObjectProperties().mass());
				}
					break;

				/* NOTE: Both entities are movable, we modify both trajectories. */
				case CollisionType::MovableEntity :
				{
					const auto & objectAProperties = targetEntity.physicalObjectProperties();
					const auto & objectBProperties = collision.entity()->physicalObjectProperties();

					auto * movableTraitA = targetEntity.getMovableTrait();
					auto * movableTraitB = collision.entity()->getMovableTrait();

					/* Force transfer. Momentum problem.
					 * TODO: Incorrect impact forces distribution.
					 * NOTE : https://www.youtube.com/watch?v=VVC_9Qi6zvw */
					const auto totalMass = objectAProperties.mass() + objectBProperties.mass();
					const auto speedMassA = movableTraitA->linearSpeed() * targetEntity.physicalObjectProperties().mass();
					const auto speedMassB = movableTraitB->linearSpeed() * collision.entity()->physicalObjectProperties().mass();
					const auto force = (speedMassA - speedMassB) / totalMass;
					const auto totalSpeedMass = speedMassA + speedMassB;

					const auto transferredSpeedB = movableTraitB->linearSpeed() * objectBProperties.bounciness();
					movableTraitA->addAcceleration(collision.direction().scaled(force + transferredSpeedB));
					movableTraitA->stopMovement();
					movableTraitA->onHit(totalSpeedMass);

					const auto transferredSpeedA = movableTraitA->linearSpeed() * objectAProperties.bounciness();
					movableTraitB->addAcceleration(-collision.direction().scaled(force + transferredSpeedA));
					movableTraitB->stopMovement();
					movableTraitB->onHit(totalSpeedMass);
				}
					break;
			}
		}

		m_collisions.clear();
	}

	bool
	Collider::isSphereCollisionWith (const AbstractEntity & sphereEntityA, const AbstractEntity & sphereEntityB, float & overflow, Vector< 3, float > & direction) noexcept
	{
		const auto sphereA = sphereEntityA.getWorldBoundingSphere();
		const auto sphereB = sphereEntityB.getWorldBoundingSphere();

		if ( !Space3D::isColliding(sphereA, sphereB, direction) )
		{
			return false;
		}

		/*overflow = Space3D::Sphere< float >::getIntersectionOverlap(sphereA, sphereB);

		if ( overflow <= 0.0F )
		{
			return false;
		}

		direction = (sphereB.position() - sphereA.position()).normalize();*/

		return true;
	}

	bool
	Collider::isBoxCollisionWith (const AbstractEntity & boxEntityA, const AbstractEntity & boxEntityB, float & overflow, Vector< 3, float > & direction) noexcept
	{
		/* NOTE: We check first with axis-aligned bounding box ... */
		if ( !Space3D::isColliding(boxEntityA.getWorldBoundingBox(), boxEntityB.getWorldBoundingBox()) )
		{
			return false;
		}

		/* NOTE: Then with oriented bounding box ... */
		overflow = OrientedCuboid< float >::isIntersecting(
			{boxEntityA.localBoundingBox(), boxEntityA.getWorldCoordinates()},
			{boxEntityB.localBoundingBox(), boxEntityB.getWorldCoordinates()},
			direction
		);

		return overflow > 0.0F;
	}

	bool
	Collider::isBoxSphereCollisionWith (const AbstractEntity & /*boxEntity*/, const AbstractEntity & /*sphereEntity*/, float & /*overflow*/, Vector< 3, float > & /*direction*/) noexcept
	{
		// TODO ...

		return false;
	}
}
