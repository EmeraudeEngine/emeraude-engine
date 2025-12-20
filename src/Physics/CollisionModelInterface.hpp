/*
 * src/Physics/CollisionModelInterface.hpp
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

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Enumeration of collision model types for internal dispatch.
	 * @note This is used internally for double dispatch, not exposed in the public contract.
	 */
	enum class CollisionModelType : uint8_t
	{
		Point,
		Sphere,
		AABB,
		Capsule
	};

	/**
	 * @brief Results of a collision detection test.
	 */
	struct CollisionDetectionResults
	{
		Libs::Math::Vector< 3, float > m_MTV{};          /**< Minimum Translation Vector to separate shapes. */
		Libs::Math::Vector< 3, float > m_contact{};      /**< Absolute contact point in world space. */
		Libs::Math::Vector< 3, float > m_impactNormal{}; /**< Normal of the impact surface. */
		float m_depth{0.0F};                             /**< Penetration depth. */
		bool m_collisionDetected{false};                 /**< Whether a collision was detected. */
	};

	/**
	 * @brief Abstract interface for collision models.
	 *
	 * This interface defines the contract for collision detection primitives.
	 * The design is STATELESS: world positions are injected at test time,
	 * not stored in the model. This allows:
	 * - Complete decoupling of shape and position
	 * - Easy testing with any position
	 * - No synchronization when entities move
	 * - Potential sharing of collision models between identical entities
	 *
	 * @since 0.8.43
	 */
	class CollisionModelInterface
	{
		public:

			virtual ~CollisionModelInterface () = default;

			/**
			 * @brief Returns the type of this collision model.
			 * @note Used internally for double dispatch.
			 * @return CollisionModelType
			 */
			[[nodiscard]]
			virtual CollisionModelType modelType () const noexcept = 0;

			/**
			 * @brief Tests collision with another collision model.
			 * @param thisWorldFrame World-space frame of this model.
			 * @param other The other collision model to test against.
			 * @param otherWorldFrame World-space frame of the other model.
			 * @return CollisionDetectionResults containing collision information.
			 */
			[[nodiscard]]
			virtual CollisionDetectionResults isCollidingWith (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept = 0;

			/**
			 * @brief Returns the axis-aligned bounding box in local space.
			 * @return AACuboid representing the local-space AABB.
			 */
			[[nodiscard]]
			virtual Libs::Math::Space3D::AACuboid< float > getAABB () const noexcept = 0;

			/**
			 * @brief Returns the axis-aligned bounding box in world space.
			 * @param worldFrame World-space frame to transform the AABB.
			 * @return AACuboid representing the world-space AABB.
			 */
			[[nodiscard]]
			virtual Libs::Math::Space3D::AACuboid< float > getAABB (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept = 0;

			/**
			 * @brief Returns the maximum bounding radius of the collision shape.
			 * @note This is the radius of the smallest sphere that can contain the shape.
			 *       - Point: 0
			 *       - Sphere: radius
			 *       - AABB: max(halfWidth, halfHeight, halfDepth)
			 *       - Capsule: half-height + radius
			 * @return float The maximum bounding radius.
			 */
			[[nodiscard]]
			virtual float getRadius () const noexcept = 0;

			/**
			 * @brief Sets the bounding shape parameters.
			 * @note The interpretation of dimensions depends on the collision model type:
			 *       - Point: dimensions ignored
			 *       - Sphere: radius = max(dimensions) * 0.5
			 *       - AABB: halfExtents = dimensions * 0.5
			 *       - Capsule: radius = max(width, depth) * 0.5, height = dimensions.y
			 * @param dimensions The dimensions (width, height, depth) of the bounding shape.
			 * @param centerOffset The offset of the shape center from the entity's origin.
			 */
			virtual void overrideShapeParameters (const Libs::Math::Vector< 3, float > & dimensions, const Libs::Math::Vector< 3, float > & centerOffset = {}) noexcept = 0;

			/**
			 * @brief Returns whether the shape parameters have been manually overridden.
			 * @return bool True if overrideShapeParameters() was called.
			 */
			[[nodiscard]]
			virtual bool areShapeParametersOverridden () const noexcept = 0;

			/**
			 * @brief Merges/expands the bounding shape to encompass the given dimensions.
			 * @note Unlike overrideShapeParameters(), this does NOT set the override flag.
			 *       Used for automatic shape computation from component geometries.
			 * @note The interpretation of dimensions depends on the collision model type:
			 *       - Point: dimensions ignored
			 *       - Sphere: radius = max(current radius, max(dimensions) * 0.5)
			 *       - AABB: merge with the given dimensions/offset
			 *       - Capsule: expand radius and height if necessary
			 * @param dimensions The dimensions (width, height, depth) to merge.
			 * @param centerOffset The offset of the shape center from the entity's origin.
			 */
			virtual void mergeShapeParameters (const Libs::Math::Vector< 3, float > & dimensions, const Libs::Math::Vector< 3, float > & centerOffset = {}) noexcept = 0;

			/**
			 * @brief Merges/expands the bounding shape to encompass the given AABB.
			 * @note Unlike overrideShapeParameters(), this does NOT set the override flag.
			 * @param aabb The axis-aligned bounding box to merge.
			 */
			virtual void mergeShapeParameters (const Libs::Math::Space3D::AACuboid< float > & aabb) noexcept = 0;

			/**
			 * @brief Merges/expands the bounding shape to encompass the given sphere.
			 * @note Unlike overrideShapeParameters(), this does NOT set the override flag.
			 * @param sphere The bounding sphere to merge.
			 */
			virtual void mergeShapeParameters (const Libs::Math::Space3D::Sphere< float > & sphere) noexcept = 0;

			/**
			 * @brief Resets the shape parameters to their initial empty state.
			 * @note This is used before merging component bounding boxes to recalculate from scratch.
			 *       Does NOT affect the override flag.
			 */
			virtual void resetShapeParameters () noexcept = 0;

		protected:

			CollisionModelInterface () = default;
	};
}
