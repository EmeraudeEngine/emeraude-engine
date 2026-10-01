/*
 * src/Physics/BoxCollisionModel.hpp
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
#include <cmath>
#include <optional>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

/* Local inclusions for usages. */
#include "Math/OrientedCuboid.hpp"
#include "Math/RigidBody.hpp"
#include "Math/Space3D/OrientedBox.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Collision model using a box primitive that turns with its entity.
	 *
	 * The box is defined in local space (an axis-aligned cuboid, possibly off-centre). World placement is injected at
	 * test time via CartesianFrame: the narrow phase collides the ORIENTED world box (toWorldBox()), the broad phase
	 * (octree) uses its world axis-aligned envelope (getAABB(frame)).
	 *
	 * @note Until 2026-10-02 this was `BoxCollisionModel` and the narrow phase collided the world envelope: a 45° box
	 * was 1.41× wider than itself for its neighbours (physics overhaul, P3, decision 8a).
	 *
	 * @since 0.8.43
	 */
	class EMEN_API BoxCollisionModel final : public CollisionModelInterface
	{
		public:

			/**
			 * @brief Constructs a default box collision model.
			 */
			BoxCollisionModel () noexcept = default;

			/**
			 * @brief Constructs a box collision model with uniform half-extents.
			 * @param halfExtent The half-extent in all directions.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			BoxCollisionModel (float halfExtent, bool parametersOverridden = false) noexcept
				: m_localBox{halfExtent},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a box collision model with separate half-extents.
			 * @param halfWidth The half-extent along X axis.
			 * @param halfHeight The half-extent along Y axis.
			 * @param halfDepth The half-extent along Z axis.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			BoxCollisionModel (float halfWidth, float halfHeight, float halfDepth, bool parametersOverridden = false) noexcept
				: m_localBox{
					Base::Math::Space3D::Point< float >{halfWidth, halfHeight, halfDepth},
					Base::Math::Space3D::Point< float >{-halfWidth, -halfHeight, -halfDepth}
				},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a box collision model from min/max bounds.
			 * @param maximum The maximum corner.
			 * @param minimum The minimum corner.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			BoxCollisionModel (const Base::Math::Space3D::Point< float > & maximum, const Base::Math::Space3D::Point< float > & minimum, bool parametersOverridden = false) noexcept
				: m_localBox{maximum, minimum},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/**
			 * @brief Constructs a box collision model from an existing AABB.
			 * @param localBox The box in local space.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			BoxCollisionModel (const Base::Math::Space3D::AACuboid< float > & localBox, bool parametersOverridden = false) noexcept
				: m_localBox{localBox},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::Box;
			}

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				return m_localBox;
			}

			/** @copydoc CollisionModelInterface::getAABB(const Base::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				const Base::Math::OrientedCuboid< float > obb{m_localBox, worldFrame};

				return obb.getAxisAlignedBox();
			}

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float
			getRadius () const noexcept override
			{
				if ( !m_localBox.isValid() )
				{
					return 0.0F;
				}

				return std::max({m_localBox.width(), m_localBox.height(), m_localBox.depth()}) * 0.5F;
			}

			/**
			 * @brief Returns the box in the entity's local space.
			 * @return const Base::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			localBox () const noexcept
			{
				return m_localBox;
			}

			/**
			 * @brief Returns the world oriented box placed by a frame (its rotation, scaling and position).
			 * @param worldFrame The world frame of the entity.
			 * @return Base::Math::Space3D::OrientedBox< float >
			 */
			[[nodiscard]]
			Base::Math::Space3D::OrientedBox< float >
			toWorldBox (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				return Base::Math::Space3D::OrientedBox< float >::fromCuboid(m_localBox, worldFrame);
			}

			/** @copydoc CollisionModelInterface::centerOfMassOffset() */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			centerOfMassOffset (const Base::Math::Vector< 3, float > & scaling) const noexcept override
			{
				if ( !m_localBox.isValid() )
				{
					return {};
				}

				const auto centroid = m_localBox.centroid();

				return {centroid[0] * scaling[0], centroid[1] * scaling[1], centroid[2] * scaling[2]};
			}

			/** @copydoc CollisionModelInterface::solidInertia() */
			[[nodiscard]]
			std::optional< Base::Math::Matrix< 3, float > >
			solidInertia (float mass, const Base::Math::Vector< 3, float > & scaling) const noexcept override
			{
				if ( !(mass > 0.0F) || !m_localBox.isValid() )
				{
					return std::nullopt;
				}

				return Base::Math::RigidBody::solidBoxInertia(mass, Base::Math::Vector< 3, float >{
					std::abs(m_localBox.width() * scaling[0]),
					std::abs(m_localBox.height() * scaling[1]),
					std::abs(m_localBox.depth() * scaling[2])
				});
			}

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Base::Math::Vector< 3, float > & dimensions, const Base::Math::Vector< 3, float > & centerOffset) noexcept override
			{
				const auto halfExtents = dimensions * 0.5F;

				m_localBox = Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{centerOffset[0] + halfExtents[0], centerOffset[1] + halfExtents[1], centerOffset[2] + halfExtents[2]},
					Base::Math::Space3D::Point< float >{centerOffset[0] - halfExtents[0], centerOffset[1] - halfExtents[1], centerOffset[2] - halfExtents[2]}
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
				const auto halfExtents = dimensions * 0.5F;

				const Base::Math::Space3D::AACuboid< float > newAABB{
					Base::Math::Space3D::Point< float >{centerOffset[0] + halfExtents[0], centerOffset[1] + halfExtents[1], centerOffset[2] + halfExtents[2]},
					Base::Math::Space3D::Point< float >{centerOffset[0] - halfExtents[0], centerOffset[1] - halfExtents[1], centerOffset[2] - halfExtents[2]}
				};

				m_localBox.merge(newAABB);
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::AACuboid< float > & aabb) noexcept override
			{
				if ( aabb.isValid() )
				{
					m_localBox.merge(aabb);
				}
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::Sphere< float > & sphere) noexcept override
			{
				const auto r = sphere.radius();
				const auto & pos = sphere.position();

				const Base::Math::Space3D::AACuboid< float > sphereAABB{
					Base::Math::Space3D::Point< float >{pos[0] + r, pos[1] + r, pos[2] + r},
					Base::Math::Space3D::Point< float >{pos[0] - r, pos[1] - r, pos[2] - r}
				};

				m_localBox.merge(sphereAABB);
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				m_localBox = Base::Math::Space3D::AACuboid< float >{};
			}

		private:

			Base::Math::Space3D::AACuboid< float > m_localBox;
			bool m_parametersOverridden{false};
	};
}
