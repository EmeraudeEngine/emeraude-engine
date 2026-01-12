/*
 * src/Physics/CapsuleCollisionModel.hpp
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
#include <cmath>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Capsule.hpp"

namespace EmEn::Physics
{
	/* Forward declarations for internal dispatch. */
	class PointCollisionModel;
	class SphereCollisionModel;
	class AABBCollisionModel;

	/**
	 * @brief Collision model using a capsule (swept sphere) primitive.
	 *
	 * The capsule is defined in local space by its axis segment and radius.
	 * World position and orientation are injected at collision test time via CartesianFrame.
	 *
	 * @since 0.8.43
	 */
	class CapsuleCollisionModel final : public CollisionModelInterface
	{
		public:

			/**
			 * @brief Constructs a default capsule collision model.
			 */
			CapsuleCollisionModel () noexcept = default;

			/**
			 * @brief Constructs a capsule collision model (degenerate to sphere).
			 * @param radius The capsule radius.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			CapsuleCollisionModel (float radius, bool parametersOverridden = false) noexcept
				: m_localCapsule{radius},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a vertical capsule collision model with radius and height.
			 * @param radius The capsule radius.
			 * @param height The total height of the capsule (along Y axis).
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			CapsuleCollisionModel (float radius, float height, bool parametersOverridden = false) noexcept
				: m_localCapsule{
					Libs::Math::Space3D::Point< float >{0.0F, height * 0.5F, 0.0F},
					Libs::Math::Space3D::Point< float >{0.0F, -height * 0.5F, 0.0F},
					radius
				},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a capsule collision model from endpoints and radius.
			 * @param startPoint Start point of the axis in local space.
			 * @param endPoint End point of the axis in local space.
			 * @param radius The capsule radius.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			CapsuleCollisionModel (const Libs::Math::Space3D::Point< float > & startPoint, const Libs::Math::Space3D::Point< float > & endPoint, float radius, bool parametersOverridden = false) noexcept
				: m_localCapsule{startPoint, endPoint, radius},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a capsule collision model from an existing capsule.
			 * @param localCapsule The local-space capsule.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			CapsuleCollisionModel (const Libs::Math::Space3D::Capsule< float > & localCapsule, bool parametersOverridden = false) noexcept
				: m_localCapsule{localCapsule},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::Capsule;
			}

			/** @copydoc CollisionModelInterface::isCollidingWith() */
			[[nodiscard]]
			CollisionDetectionResults isCollidingWith (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept override;

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				const auto & start = m_localCapsule.startPoint();
				const auto & end = m_localCapsule.endPoint();
				const auto r = m_localCapsule.radius();

				const auto minX = std::min(start[0], end[0]) - r;
				const auto maxX = std::max(start[0], end[0]) + r;
				const auto minY = std::min(start[1], end[1]) - r;
				const auto maxY = std::max(start[1], end[1]) + r;
				const auto minZ = std::min(start[2], end[2]) - r;
				const auto maxZ = std::max(start[2], end[2]) + r;

				return Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{maxX, maxY, maxZ},
					Libs::Math::Space3D::Point< float >{minX, minY, minZ}
				};
			}

			/** @copydoc CollisionModelInterface::getAABB(const Libs::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getAABB (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				/* Transform capsule endpoints to world space. */
				const auto rotMatrix = worldFrame.getRotationMatrix3();
				const auto & pos = worldFrame.position();
				const auto worldStart = pos + rotMatrix * m_localCapsule.startPoint();
				const auto worldEnd = pos + rotMatrix * m_localCapsule.endPoint();
				const auto r = m_localCapsule.radius();

				const auto minX = std::min(worldStart[0], worldEnd[0]) - r;
				const auto maxX = std::max(worldStart[0], worldEnd[0]) + r;
				const auto minY = std::min(worldStart[1], worldEnd[1]) - r;
				const auto maxY = std::max(worldStart[1], worldEnd[1]) + r;
				const auto minZ = std::min(worldStart[2], worldEnd[2]) - r;
				const auto maxZ = std::max(worldStart[2], worldEnd[2]) + r;

				return Libs::Math::Space3D::AACuboid< float >{
					Libs::Math::Space3D::Point< float >{maxX, maxY, maxZ},
					Libs::Math::Space3D::Point< float >{minX, minY, minZ}
				};
			}

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float
			getRadius () const noexcept override
			{
				if ( !m_localCapsule.isValid() )
				{
					return 0.0F;
				}

				const auto halfAxisLength = (m_localCapsule.endPoint() - m_localCapsule.startPoint()).length() * 0.5F;

				return halfAxisLength + m_localCapsule.radius();
			}

			/**
			 * @brief Returns the local-space capsule.
			 * @return const Libs::Math::Space3D::Capsule< float > &
			 */
			[[nodiscard]]
			const Libs::Math::Space3D::Capsule< float > &
			localCapsule () const noexcept
			{
				return m_localCapsule;
			}

			/**
			 * @brief Returns the capsule radius.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_localCapsule.radius();
			}

			/**
			 * @brief Creates a world-space capsule from the given frame.
			 * @param worldFrame The world frame providing position and orientation.
			 * @return Libs::Math::Space3D::Capsule< float >
			 */
			[[nodiscard]]
			Libs::Math::Space3D::Capsule< float >
			toWorldCapsule (const Libs::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				const auto rotMatrix = worldFrame.getRotationMatrix3();
				const auto & pos = worldFrame.position();

				return Libs::Math::Space3D::Capsule< float >{
					pos + rotMatrix * m_localCapsule.startPoint(),
					pos + rotMatrix * m_localCapsule.endPoint(),
					m_localCapsule.radius()
				};
			}

			/**
			 * @brief Collision test: Capsule vs Point.
			 * @param thisWorldFrame World frame of this capsule.
			 * @param other The point model.
			 * @param otherWorldFrame World frame of the point.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithPoint (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Capsule vs Sphere.
			 * @param thisWorldFrame World frame of this capsule.
			 * @param other The sphere model.
			 * @param otherWorldFrame World frame of the sphere.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithSphere (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Capsule vs AABB.
			 * @param thisWorldFrame World frame of this capsule.
			 * @param other The AABB model.
			 * @param otherWorldFrame World frame of the AABB.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithAABB (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/**
			 * @brief Collision test: Capsule vs Capsule.
			 * @param thisWorldFrame World frame of this capsule.
			 * @param other The other capsule model.
			 * @param otherWorldFrame World frame of the other capsule.
			 * @return CollisionDetectionResults
			 */
			[[nodiscard]]
			CollisionDetectionResults collideWithCapsule (const Libs::Math::CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const Libs::Math::CartesianFrame< float > & otherWorldFrame) const noexcept;

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Libs::Math::Vector< 3, float > & dimensions, const Libs::Math::Vector< 3, float > & centerOffset) noexcept override
			{
				/* Capsule radius from horizontal dimensions (width, depth). */
				const auto radius = std::max(dimensions[0], dimensions[2]) * 0.5F;

				/* Axis half-length: total height minus the two hemispheres. */
				const auto halfAxisLength = std::max(0.0F, (dimensions[1] - 2.0F * radius) * 0.5F);

				/* Build vertical capsule (along Y axis) centered at offset. */
				m_localCapsule = Libs::Math::Space3D::Capsule< float >{
					Libs::Math::Space3D::Point< float >{centerOffset[0], centerOffset[1] - halfAxisLength, centerOffset[2]},
					Libs::Math::Space3D::Point< float >{centerOffset[0], centerOffset[1] + halfAxisLength, centerOffset[2]},
					radius
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
				/* Calculate new potential radius from horizontal dimensions. */
				const auto newRadius = std::max(dimensions[0], dimensions[2]) * 0.5F;

				/* Calculate new potential half-axis length. */
				const auto newHalfAxisLength = std::max(0.0F, (dimensions[1] - 2.0F * newRadius) * 0.5F);

				/* Get current capsule parameters. */
				const auto currentRadius = m_localCapsule.radius();
				const auto & start = m_localCapsule.startPoint();
				const auto & end = m_localCapsule.endPoint();
				const auto currentCenter = (start + end) * 0.5F;
				const auto currentHalfAxisLength = (end - start).length() * 0.5F;

				/* Expand if necessary. */
				const auto mergedRadius = std::max(currentRadius, newRadius);
				const auto mergedHalfAxisLength = std::max(currentHalfAxisLength, newHalfAxisLength);

				/* Merge centers (use component-wise min/max to encompass both). */
				const auto mergedCenterY = (currentCenter[1] + centerOffset[1]) * 0.5F;

				/* Rebuild capsule with merged parameters. */
				m_localCapsule = Libs::Math::Space3D::Capsule< float >{
					Libs::Math::Space3D::Point< float >{centerOffset[0], mergedCenterY - mergedHalfAxisLength, centerOffset[2]},
					Libs::Math::Space3D::Point< float >{centerOffset[0], mergedCenterY + mergedHalfAxisLength, centerOffset[2]},
					mergedRadius
				};
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::AACuboid< float > & aabb) noexcept override
			{
				if ( aabb.isValid() )
				{
					const Libs::Math::Vector< 3, float > dimensions{aabb.width(), aabb.height(), aabb.depth()};
					this->mergeShapeParameters(dimensions, aabb.centroid());
				}
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Libs::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Libs::Math::Space3D::Sphere< float > & sphere) noexcept override
			{
				/* For sphere, use diameter as all dimensions. */
				const auto diameter = sphere.radius() * 2.0F;
				const Libs::Math::Vector< 3, float > dimensions{diameter, diameter, diameter};
				this->mergeShapeParameters(dimensions, sphere.position());
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				m_localCapsule = Libs::Math::Space3D::Capsule< float >{};
			}

		private:

			Libs::Math::Space3D::Capsule< float > m_localCapsule;
			bool m_parametersOverridden{false};
	};
}
