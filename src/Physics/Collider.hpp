/*
 * src/Physics/Collider.hpp
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
#include <algorithm>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Collision.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief The Collider class
	 */
	class Collider final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Collider"};

			/**
			 * @brief Constructs a default collider.
			 */
			Collider () noexcept = default;

			/**
			 * @brief Checks if a collision occurs with another movable entity.
			 * @param movableEntityA A reference to a movable entity.
			 * @param movableEntityB A reference to a movable entity.
			 * @return bool
			 */
			bool checkCollisionAgainstMovable (Scenes::AbstractEntity & movableEntityA, Scenes::AbstractEntity & movableEntityB) noexcept;

			/**
			 * @brief Checks if a collision occurs with a static entity.
			 * @param movableEntityA A reference to a movable entity.
			 * @param staticEntityB A reference to a static entity.
			 * @return bool
			 */
			bool checkCollisionAgainstStatic (Scenes::AbstractEntity & movableEntityA, Scenes::AbstractEntity & staticEntityB) noexcept;

			/**
			 * @brief Adds a collision to the collection.
			 * @param type The collision type.
			 * @param entity A pointer to an entity in case of collision with an entity.
			 * @param position The position vector of the hit.
			 * @param direction The direction vector of the hit.
			 * @return void
			 */
			void
			addCollision (CollisionType type, Scenes::AbstractEntity * entity, const Libs::Math::Vector< 3, float > & position, const Libs::Math::Vector< 3, float > & direction) noexcept
			{
				m_collisions.emplace_back(type, entity, position, direction);
			}

			/**
			 * @brief Returns whether there is collision.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasCollisions () const noexcept
			{
				return !m_collisions.empty();
			}

			/**
			 * @brief Returns whether a collision has already been registered with an entity.
			 * @note This is useful for cross octree sector computation only.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasCollisionWith (Scenes::AbstractEntity & entity) const noexcept
			{
				return std::ranges::any_of(m_collisions, [&entity] (const auto & collision) {
					return &entity == collision.entity();
				});
			}

			/**
			 * @brief Resolves the collisions to set the entity to a valid location.
			 * @param targetEntity A reference to a movable entity.
			 * @return void
			 */
			void resolveCollisions (Scenes::AbstractEntity & targetEntity) noexcept;

			/**
			 * @brief Removes all collision registered.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_collisions.clear();
			}

		private:

			/**
			 * @brief Returns the possible overflow between two entities using the sphere collision model.
			 * @param sphereEntityA A reference to the first entity.
			 * @param sphereEntityB A reference to the second entity.
			 * @param overflow A reference to put the distance of collision overflow.
			 * @param direction A reference to put the direction of collision if exists.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isSphereCollisionWith (const Scenes::AbstractEntity & sphereEntityA, const Scenes::AbstractEntity & sphereEntityB, float & overflow, Libs::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Returns the possible overflow between two entities using the box collision model.
			 * @param boxEntityA A reference to the first entity.
			 * @param boxEntityB A reference to the second entity.
			 * @param overflow A reference to put the distance of collision overflow.
			 * @param direction A reference to put the direction of collision if exists.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isBoxCollisionWith (const Scenes::AbstractEntity & boxEntityA, const Scenes::AbstractEntity & boxEntityB, float & overflow, Libs::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Returns the possible overflow between two entities using the sphere versus box collision model.
			 * @param boxEntity A reference to the second entity using box.
			 * @param sphereEntity A reference to the first entity using a sphere.
			 * @param overflow A reference to put the distance of collision overflow.
			 * @param direction A reference to put the direction of collision if exists.
			 * @return bool
			 */
			[[nodiscard]]
			static bool isBoxSphereCollisionWith (const Scenes::AbstractEntity & boxEntity, const Scenes::AbstractEntity & sphereEntity, float & overflow, Libs::Math::Vector< 3, float > & direction) noexcept;

			std::vector< Collision > m_collisions;
	};
}
