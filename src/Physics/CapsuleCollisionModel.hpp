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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <optional>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

/* Local inclusions for usages. */
#include "Math/RigidBody.hpp"
#include "Math/Space3D/Capsule.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Collision model using a capsule (swept sphere) primitive.
	 *
	 * The capsule is defined in local space by its axis segment and radius.
	 * World position and orientation are injected at collision test time via CartesianFrame.
	 *
	 * @since 0.8.43
	 */
	class EMEN_API CapsuleCollisionModel final : public CollisionModelInterface
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
					Base::Math::Space3D::Point< float >{0.0F, height * 0.5F, 0.0F},
					Base::Math::Space3D::Point< float >{0.0F, -height * 0.5F, 0.0F},
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
			CapsuleCollisionModel (const Base::Math::Space3D::Point< float > & startPoint, const Base::Math::Space3D::Point< float > & endPoint, float radius, bool parametersOverridden = false) noexcept
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
			CapsuleCollisionModel (const Base::Math::Space3D::Capsule< float > & localCapsule, bool parametersOverridden = false) noexcept
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

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
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

				return Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{maxX, maxY, maxZ},
					Base::Math::Space3D::Point< float >{minX, minY, minZ}
				};
			}

			/** @copydoc CollisionModelInterface::getAABB(const Base::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept override
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

				return Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{maxX, maxY, maxZ},
					Base::Math::Space3D::Point< float >{minX, minY, minZ}
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
			 * @return const Base::Math::Space3D::Capsule< float > &
			 */
			[[nodiscard]]
			const Base::Math::Space3D::Capsule< float > &
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
			 * @return Base::Math::Space3D::Capsule< float >
			 */
			[[nodiscard]]
			Base::Math::Space3D::Capsule< float >
			toWorldCapsule (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				const auto rotMatrix = worldFrame.getRotationMatrix3();
				const auto & pos = worldFrame.position();

				return Base::Math::Space3D::Capsule< float >{
					pos + rotMatrix * m_localCapsule.startPoint(),
					pos + rotMatrix * m_localCapsule.endPoint(),
					m_localCapsule.radius()
				};
			}

			/** @copydoc CollisionModelInterface::centerOfMassOffset() */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			centerOfMassOffset (const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				/* The capsule ignores the frame's scaling (toWorldCapsule()). */
				return (m_localCapsule.startPoint() + m_localCapsule.endPoint()) * 0.5F;
			}

			/** @copydoc CollisionModelInterface::solidInertia() */
			[[nodiscard]]
			std::optional< Base::Math::Matrix< 3, float > >
			solidInertia (float mass, const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				if ( !(mass > 0.0F) || !m_localCapsule.isValid() )
				{
					return std::nullopt;
				}

				const auto axis = m_localCapsule.endPoint() - m_localCapsule.startPoint();
				const auto axisLength = axis.length();
				const auto alongY = Base::Math::RigidBody::solidCapsuleInertia(mass, m_localCapsule.radius(), axisLength);

				if ( !alongY.has_value() || !(axisLength > 0.0F) )
				{
					/* No axis: a sphere, the same tensor in every orientation. */
					return alongY;
				}

				/* An axisymmetric body about the unit axis a: I = I⊥ Id + (I∥ − I⊥) a aᵀ. */
				const auto direction = axis * (1.0F / axisLength);
				const auto perpendicular = alongY.value()[Base::Math::M3x3Col0Row0];
				const auto difference = alongY.value()[Base::Math::M3x3Col1Row1] - perpendicular;

				const auto xx = (difference * direction[0] * direction[0]) + perpendicular;
				const auto yy = (difference * direction[1] * direction[1]) + perpendicular;
				const auto zz = (difference * direction[2] * direction[2]) + perpendicular;
				const auto xy = difference * direction[0] * direction[1];
				const auto xz = difference * direction[0] * direction[2];
				const auto yz = difference * direction[1] * direction[2];

				/* Symmetric: the column-major storage reads the same as the row-major. */
				const std::array< float, 9 > values{
					xx, xy, xz,
					xy, yy, yz,
					xz, yz, zz
				};

				return Base::Math::Matrix< 3, float >{values};
			}

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Base::Math::Vector< 3, float > & dimensions, const Base::Math::Vector< 3, float > & centerOffset) noexcept override
			{
				/* Capsule radius from horizontal dimensions (width, depth). */
				const auto radius = std::max(dimensions[0], dimensions[2]) * 0.5F;

				/* Axis half-length: total height minus the two hemispheres. */
				const auto halfAxisLength = std::max(0.0F, (dimensions[1] - 2.0F * radius) * 0.5F);

				/* Build vertical capsule (along Y axis) centered at offset. */
				m_localCapsule = Base::Math::Space3D::Capsule< float >{
					Base::Math::Space3D::Point< float >{centerOffset[0], centerOffset[1] - halfAxisLength, centerOffset[2]},
					Base::Math::Space3D::Point< float >{centerOffset[0], centerOffset[1] + halfAxisLength, centerOffset[2]},
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
			mergeShapeParameters (const Base::Math::Vector< 3, float > & dimensions, const Base::Math::Vector< 3, float > & centerOffset) noexcept override
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
				m_localCapsule = Base::Math::Space3D::Capsule< float >{
					Base::Math::Space3D::Point< float >{centerOffset[0], mergedCenterY - mergedHalfAxisLength, centerOffset[2]},
					Base::Math::Space3D::Point< float >{centerOffset[0], mergedCenterY + mergedHalfAxisLength, centerOffset[2]},
					mergedRadius
				};
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::AACuboid< float > & aabb) noexcept override
			{
				if ( aabb.isValid() )
				{
					const Base::Math::Vector< 3, float > dimensions{aabb.width(), aabb.height(), aabb.depth()};
					this->mergeShapeParameters(dimensions, aabb.centroid());
				}
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::Sphere< float > & sphere) noexcept override
			{
				/* For sphere, use diameter as all dimensions. */
				const auto diameter = sphere.radius() * 2.0F;
				const Base::Math::Vector< 3, float > dimensions{diameter, diameter, diameter};
				this->mergeShapeParameters(dimensions, sphere.position());
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				m_localCapsule = Base::Math::Space3D::Capsule< float >{};
			}

		private:

			Base::Math::Space3D::Capsule< float > m_localCapsule;
			bool m_parametersOverridden{false};
	};
}
