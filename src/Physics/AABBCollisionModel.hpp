/*
 * src/Physics/AABBCollisionModel.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <algorithm>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

namespace EmEn::Physics
{
	/* Forward declarations for internal dispatch. */
	class PointCollisionModel;
	class SphereCollisionModel;
	class CapsuleCollisionModel;

	/**
	 * @brief Collision model using an axis-aligned bounding box primitive.
	 *
	 * The AABB is defined in local space (centered at origin).
	 * World position is injected at collision test time via CartesianFrame.
	 *
	 * @note Rotation is NOT supported for AABB. The world frame's orientation
	 *	   is ignored - only the position is used for translation.
	 *
	 * @since 0.8.43
	 */
	class AABBCollisionModel final : public CollisionModelInterface
	{
		public:

			/**
			 * @brief Constructs a default AABB collision model.
			 */
			AABBCollisionModel () noexcept = default;

			/**
			 * @brief Constructs an AABB collision model with uniform half-extents.
			 * @param halfExtent The half-extent in all directions.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			AABBCollisionModel (float halfExtent, bool parametersOverridden = false) noexcept
				: m_localAABB{halfExtent},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs an AABB collision model with separate half-extents.
			 * @param halfWidth The half-extent along X axis.
			 * @param halfHeight The half-extent along Y axis.
			 * @param halfDepth The half-extent along Z axis.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			AABBCollisionModel (float halfWidth, float halfHeight, float halfDepth, bool parametersOverridden = false) noexcept
				: m_localAABB{
					Libs::Math::Space3D::Point< float >{halfWidth, halfHeight, halfDepth},
					Libs::Math::Space3D::Point< float >{-halfWidth, -halfHeight, -halfDepth}
				},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs an AABB collision model from min/max bounds.
			 * @param maximum The maximum corner.
			 * @param minimum The minimum corner.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			AABBCollisionModel (const Libs::Math::Space3D::Point< float > & maximum, const Libs::Math::Space3D::Point< float > & minimum, bool parametersOverridden = false) noexcept
				: m_localAABB{maximum, minimum},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs an AABB collision model from an existing AABB.
			 * @param localAABB The local-space AABB.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			AABBCollisionModel (const Libs::Math::Space3D::AACuboid< float > & localAABB, bool parametersOverridden = false) noexcept
				: m_localAABB{localAABB},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::AABB;
			}

			/** @copydoc CollisionModelInterface::isCollidingWith() */
			[[nodiscard]]
			CollisionDetectionResults isCollidingWith (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept override;

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				return m_localAABB;
			}

			/** @copydoc CollisionModelInterface::getAABB(const Libs::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				const auto & pos = worldFrame.position();

				return Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{
						m_localAABB.maximum()[0] + pos[0],
						m_localAABB.maximum()[1] + pos[1],
						m_localAABB.maximum()[2] + pos[2]
					},
					Libs::Math::Space3D::Point< float >{
						m_localAABB.minimum()[0] + pos[0],
						m_localAABB.minimum()[1] + pos[1],
						m_localAABB.minimum()[2] + pos[2]
					}
				};
			}

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float
			getRadius () const noexcept override
			{
				if ( !m_localAABB.isValid() )
				{
					return 0.0F;
				}

				return std::max({m_localAABB.width(), m_localAABB.height(), m_localAABB.depth()}) * 0.5F;
			}

			/**
			 * @brief Returns the local-space AABB.
			 * @return const Libs::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			localAABB () const noexcept
			{
				return m_localAABB;
			}

			/**
			 * @brief Creates a world-space AABB from the given frame.
			 * @param worldFrame The world frame providing position.
			 * @return Libs::Math::Space3D::AACuboid< float >
			 */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			toWorldAABB (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				return this->getAABB(worldFrame);
			}

			/**
			 * @brief Collision test: AABB vs Point.
			 * @param thisWorldFrame World frame of this AABB.
			 * @param other The point model.
			 * @param otherWorldFrame World frame of the point.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithPoint (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: AABB vs Sphere.
			 * @param thisWorldFrame World frame of this AABB.
			 * @param other The sphere model.
			 * @param otherWorldFrame World frame of the sphere.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithSphere (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: AABB vs AABB.
			 * @param thisWorldFrame World frame of this AABB.
			 * @param other The other AABB model.
			 * @param otherWorldFrame World frame of the other AABB.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithAABB (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: AABB vs Capsule.
			 * @param thisWorldFrame World frame of this AABB.
			 * @param other The capsule model.
			 * @param otherWorldFrame World frame of the capsule.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithCapsule (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Libs::Math::Vector< 3, float > & dimensions, const Libs::Math::Vector< 3, float > & centerOffset) noexcept override
			{
				const auto halfExtents = dimensions * 0.5F;

				m_localAABB = Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{centerOffset[0] + halfExtents[0], centerOffset[1] + halfExtents[1], centerOffset[2] + halfExtents[2]},
					Libs::Math::Space3D::Point< float >{centerOffset[0] - halfExtents[0], centerOffset[1] - halfExtents[1], centerOffset[2] - halfExtents[2]}
				};

				m_parametersOverridden = true;
			}

			/** @copydoc CollisionModelInterface::areShapeParametersOverridden() */
			[[nodiscard]]
			bool
			areShapeParametersOverridden () const noexcept override
			{
				return m_parametersOverridden;
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters() */
			void
			mergeShapeParameters (const Libs::Math::Vector< 3, float > & dimensions, const Libs::Math::Vector< 3, float > & centerOffset) noexcept override
			{
				const auto halfExtents = dimensions * 0.5F;

				const Libs::Math::Space3D::AACuboid< float > newAABB{
					Libs::Math::Space3D::Point< float >{centerOffset[0] + halfExtents[0], centerOffset[1] + halfExtents[1], centerOffset[2] + halfExtents[2]},
					Libs::Math::Space3D::Point< float >{centerOffset[0] - halfExtents[0], centerOffset[1] - halfExtents[1], centerOffset[2] - halfExtents[2]}
				};

				m_localAABB.merge(newAABB);
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::AACuboid< float > & aabb) noexcept override
			{
				if ( aabb.isValid() )
				{
					m_localAABB.merge(aabb);
				}
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::Sphere< float > & sphere) noexcept override
			{
				const auto r = sphere.radius();
				const auto & pos = sphere.position();

				const Libs::Math::Space3D::AACuboid< float > sphereAABB{
					Libs::Math::Space3D::Point< float >{pos[0] + r, pos[1] + r, pos[2] + r},
					Libs::Math::Space3D::Point< float >{pos[0] - r, pos[1] - r, pos[2] - r}
				};

				m_localAABB.merge(sphereAABB);
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				m_localAABB = Libs::Math::Space3D::AACuboid< float >{};
			}

		private:

			Libs::Math::Space3D::AACuboid< float > m_localAABB;
			bool m_parametersOverridden{false};
	};
}
