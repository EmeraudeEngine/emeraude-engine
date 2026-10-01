/*
 * src/Physics/PointCollisionModel.hpp
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
#include <limits>
#include <optional>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Collision model using a single point (zero-volume).
	 *
	 * The point is at the local origin. World position is injected
	 * at collision test time via CartesianFrame.
	 *
	 * @note A point has no volume, so Point vs Point collision is always false.
	 *	   Point is useful for raycasting endpoints or trigger detection.
	 *
	 * @since 0.8.43
	 */
	class EMEN_API PointCollisionModel final : public CollisionModelInterface
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

			/** @copydoc CollisionModelInterface::getAABB() */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB () const noexcept override
			{
				/* Use the smallest possible valid AABB. */
				constexpr auto epsilon = std::numeric_limits< float >::epsilon();

				return Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{epsilon, epsilon, epsilon},
					Base::Math::Space3D::Point< float >{-epsilon, -epsilon, -epsilon}
				};
			}

			/** @copydoc CollisionModelInterface::getAABB(const Base::Math::CartesianFrame< float > &) */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float >
			getAABB (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept override
			{
				const auto & pos = worldFrame.position();
				constexpr auto epsilon = std::numeric_limits< float >::epsilon();

				return Base::Math::Space3D::AACuboid< float >{
					Base::Math::Space3D::Point< float >{pos[0] + epsilon, pos[1] + epsilon, pos[2] + epsilon},
					Base::Math::Space3D::Point< float >{pos[0] - epsilon, pos[1] - epsilon, pos[2] - epsilon}
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
			 * @return Base::Math::Space3D::Point< float >
			 */
			[[nodiscard]]
			static
			Base::Math::Space3D::Point< float >
			toWorldPoint (const Base::Math::CartesianFrame< float > & worldFrame) noexcept
			{
				return worldFrame.position();
			}

			/** @copydoc CollisionModelInterface::centerOfMassOffset() */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			centerOfMassOffset (const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				return {};
			}

			/** @copydoc CollisionModelInterface::solidInertia() */
			[[nodiscard]]
			std::optional< Base::Math::Matrix< 3, float > >
			solidInertia (float /*mass*/, const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				/* A point has no extent: it does not rotate. */
				return std::nullopt;
			}

			/** @copydoc CollisionModelInterface::overrideShapeParameters() */
			void
			overrideShapeParameters (const Base::Math::Vector< 3, float > & /*dimensions*/, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
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
			mergeShapeParameters (const Base::Math::Vector< 3, float > & /*dimensions*/, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{
				/* Point has no shape parameters to merge. */
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::AACuboid< float > & /*aabb*/) noexcept override
			{
				/* Point has no shape parameters to merge. */
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::Sphere< float > & /*sphere*/) noexcept override
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
