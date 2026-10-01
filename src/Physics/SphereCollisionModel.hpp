/*
 * src/Physics/SphereCollisionModel.hpp
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
#include <optional>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

/* Local inclusions for usages. */
#include "Math/RigidBody.hpp"
#include "Math/Space3D/Sphere.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Collision model using a sphere primitive.
	 *
	 * The sphere is defined by its radius only (centered at local origin).
	 * World position is injected at collision test time via CartesianFrame.
	 *
	 * @since 0.8.43
	 */
	class EMEN_API SphereCollisionModel final : public CollisionModelInterface
	{
		public:

			/**
			 * @brief Constructs a default sphere collision model.
			 */
			SphereCollisionModel () noexcept = default;

			/**
			 * @brief Constructs a sphere collision model with the given radius.
			 * @param radius The sphere radius.
			 * @param parametersOverridden Set the parameters overridden. Default false.
			 */
			explicit
			SphereCollisionModel (float radius, bool parametersOverridden = false) noexcept
				: m_radius{radius},
				m_parametersOverridden{parametersOverridden}
			{

			}

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::Sphere;
			}

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				return Base::Math::Space3D::AACuboid< float >{m_radius};
			}

			/** @copydoc CollisionModelInterface::getAABB(const Base::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				const auto & pos = worldFrame.position();

				return Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{pos[0] + m_radius, pos[1] + m_radius, pos[2] + m_radius},
					Base::Math::Space3D::Point< float >{pos[0] - m_radius, pos[1] - m_radius, pos[2] - m_radius}
				};
			}

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float
			getRadius () const noexcept override
			{
				return m_radius;
			}

			/**
			 * @brief Returns the sphere radius.
			 * @return float
			 */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

			/**
			 * @brief Creates a world-space sphere from the given frame.
			 * @param worldFrame The world frame providing position.
			 * @return Base::Math::Space3D::Sphere< float >
			 */
			[[nodiscard]]
			Base::Math::Space3D::Sphere< float >
			toWorldSphere (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept
			{
				return Base::Math::Space3D::Sphere< float >{m_radius, worldFrame.position()};
			}

			/** @copydoc CollisionModelInterface::centerOfMassOffset() */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			centerOfMassOffset (const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				/* The sphere is centred on the entity's origin. */
				return {};
			}

			/** @copydoc CollisionModelInterface::solidInertia() */
			[[nodiscard]]
			std::optional< Base::Math::Matrix< 3, float > >
			solidInertia (float mass, const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				if ( !(mass > 0.0F) || !(m_radius > 0.0F) )
				{
					return std::nullopt;
				}

				return Base::Math::RigidBody::solidSphereInertia(mass, m_radius);
			}

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Base::Math::Vector< 3, float > & dimensions, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{
				m_radius = std::max({dimensions[0], dimensions[1], dimensions[2]}) * 0.5F;
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
			mergeShapeParameters (const Base::Math::Vector< 3, float > & dimensions, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{
				const auto newRadius = std::max({dimensions[0], dimensions[1], dimensions[2]}) * 0.5F;
				m_radius = std::max(m_radius, newRadius);
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::AACuboid< float > & aabb) noexcept override
			{
				if ( aabb.isValid() )
				{
					const auto newRadius = std::max({aabb.width(), aabb.height(), aabb.depth()}) * 0.5F;
					m_radius = std::max(m_radius, newRadius);
				}
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::Sphere< float > & sphere) noexcept override
			{
				m_radius = std::max(m_radius, sphere.radius());
			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{
				m_radius = 0.0F;
			}

		private:

			float m_radius{1.0F};
			bool m_parametersOverridden{false};
	};
}
