/*
 * src/Physics/Collision.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class AbstractEntity;
}

namespace EmEn::Physics
{
	/** @brief The type of collision enumeration. */
	enum class CollisionType : uint8_t
	{
		/** @brief The imaginary limits of the scene. */
		SceneBoundary,
		/** @brief The scene ground if exists. */
		SceneGround,
		/** @brief Against a static entity. */
		StaticEntity,
		/** @brief Against an entity that has a MovableTrait. */
		MovableEntity
	};

	/**
	 * @brief This class defines a collision.
	 */
	class Collision final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Collision"};

			/**
			 * @brief Constructs a collision structure against the scene area.
			 * @param type The type of collision.
			 * @param entity A pointer to an entity in case of a collision.
			 * @param position A reference to a vector for the collision world position.
			 * @param direction A reference to a vector for the collision direction.
			 */
			Collision (CollisionType type, Scenes::AbstractEntity * entity, const Libs::Math::Vector< 3, float > & position, const Libs::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Returns the type of collision.
			 * @return CollisionType
			 */
			[[nodiscard]]
			CollisionType
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the entity involved in the collision when the type is StaticEntity or MovableEntity.
			 * @warning This method can return nullptr if no entity is involved, check the type first!
			 * @return Scenes::AbstractEntity *
			 */
			[[nodiscard]]
			Scenes::AbstractEntity *
			entity () const noexcept
			{
				return m_entity;
			}

			/**
			 * @brief Returns the world position of the collision.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			position () const noexcept
			{
				return m_position;
			}

			/**
			 * @brief Returns the world direction vector of the collision.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			direction () const noexcept
			{
				return m_direction;
			}

		private:

			CollisionType m_type;
			Scenes::AbstractEntity * m_entity;
			Libs::Math::Vector< 3, float > m_position;
			Libs::Math::Vector< 3, float > m_direction;
	};
}
