/*
 * src/Physics/PointCollisionModel.hpp
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
#include <limits>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

namespace EmEn::Physics
{
	/* Forward declarations for internal dispatch. */
	class SphereCollisionModel;
	class AABBCollisionModel;
	class CapsuleCollisionModel;

	/**
	 * @brief Collision model using a single point (zero-volume).
	 *
	 * The point is at the local origin. World position is injected
	 * at collision test time via CartesianFrame.
	 *
	 * @note A point has no volume, so Point vs Point collision is always false.
	 *       Point is useful for raycasting endpoints or trigger detection.
	 *
	 * @since 0.8.43
	 */
	class PointCollisionModel final : public CollisionModelInterface
	{
		public:

			/**
			 * @brief Constructs a point collision model.
			 */
			PointCollisionModel () noexcept = default;

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::Point;
			}

			/** @copydoc CollisionModelInterface::isCollidingWith() */
			[[nodiscard]]
			CollisionDetectionResults isCollidingWith (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept override;

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				/* Use smallest possible valid AABB. */
				constexpr auto epsilon = std::numeric_limits< float >::epsilon();

				return Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{epsilon, epsilon, epsilon},
					Libs::Math::Space3D::Point< float >{-epsilon, -epsilon, -epsilon}
				};
			}

			/** @copydoc CollisionModelInterface::getAABB(const Libs::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				const auto & pos = worldFrame.position();
				constexpr auto epsilon = std::numeric_limits< float >::epsilon();

				return Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{pos[0] + epsilon, pos[1] + epsilon, pos[2] + epsilon},
					Libs::Math::Space3D::Point< float >{pos[0] - epsilon, pos[1] - epsilon, pos[2] - epsilon}
				};
			}

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float
			getRadius () const noexcept override
			{
				return 0.0F;
			}

			/**
			 * @brief Returns the world-space point from the given frame.
			 * @param worldFrame The world frame providing position.
			 * @return Libs::Math::Space3D::Point< float >
			 */
			[[nodiscard]]
			Libs::Math::Space3D::Point< float >
			toWorldPoint (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				return worldFrame.position();
			}

			/**
			 * @brief Collision test: Point vs Point.
			 * @note Always returns false (two points cannot collide).
			 * @param thisWorldFrame World frame of this point.
			 * @param other The other point model.
			 * @param otherWorldFrame World frame of the other point.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithPoint (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Point vs Sphere.
			 * @param thisWorldFrame World frame of this point.
			 * @param other The sphere model.
			 * @param otherWorldFrame World frame of the sphere.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithSphere (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Point vs AABB.
			 * @param thisWorldFrame World frame of this point.
			 * @param other The AABB model.
			 * @param otherWorldFrame World frame of the AABB.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithAABB (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Point vs Capsule.
			 * @param thisWorldFrame World frame of this point.
			 * @param other The capsule model.
			 * @param otherWorldFrame World frame of the capsule.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithCapsule (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Libs::Math::Vector< 3, float > & /*dimensions*/, const Libs::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{
				/* Point has no shape parameters to set. */
			}

			/** @copydoc CollisionModelInterface::areShapeParametersOverridden() */
			[[nodiscard]]
			bool
			areShapeParametersOverridden () const noexcept override
			{
				/* Point has no shape parameters, always returns false. */
				return false;
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters() */
			void
			mergeShapeParameters (const Libs::Math::Vector< 3, float > & /*dimensions*/, const Libs::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{
				/* Point has no shape parameters to merge. */
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::AACuboid< float > & /*aabb*/) noexcept override
			{
				/* Point has no shape parameters to merge. */
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::Sphere< float > & /*sphere*/) noexcept override
			{
				/* Point has no shape parameters to merge. */
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				/* Point has no shape parameters to reset. */
			}
	};
}
