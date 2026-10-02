/*
 * src/Physics/TriangleMeshCollisionModel.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <optional>

/* Local inclusions for inheritances. */
#include "CollisionModelInterface.hpp"

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"
#include "Math/Matrix.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Triangle.hpp"
#include "Math/Space3D/TriangleMesh.hpp"
#include "VertexFactory/Shape.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief A STATIC triangle-mesh collision model (physics overhaul P5, owner decisions 13): a building, a staircase or
	 * a ramp collides as its triangles instead of a box.
	 * @note Explicit: an entity gets it with setCollisionModel() (Toolkit::generateTriangleMeshInstance()); nothing
	 * converts a visual to it. ONE-SIDED by default — only the front face (the winding normal, (B - A) × (C - A))
	 * collides, like the ground — two-sided on request (a thin panel, an open surface). The triangles live in the
	 * entity's local space (its frame's scaling applies) in an emeraude-base `Space3D::TriangleMesh` (a bounding-volume
	 * hierarchy, active edges), shared between the entities that use the same mesh.
	 * @note Never solved as a body (no mass, no inertia, no centre of mass): a movable entity carrying it is KINEMATIC
	 * — it moves as its node moves (an animated platform) and what touches it is pushed.
	 * @extends EmEn::Physics::CollisionModelInterface
	 */
	class EMEN_API TriangleMeshCollisionModel final : public CollisionModelInterface
	{
		public:

			/** @brief Jolt's default: a convex edge under 5° is flat (inactive). */
			static constexpr float DefaultActiveEdgeCosine{0.99619470F};

			/** @brief The bounds are thickened by this much (m) on each side: a flat mesh has a valid box. */
			static constexpr float BoundsPadding{0.001F};

			/**
			 * @brief Constructs a model on a built mesh.
			 * @param mesh The mesh (shared, non-null, not empty).
			 * @param twoSided Whether the back faces collide too. Default false.
			 */
			explicit TriangleMeshCollisionModel (std::shared_ptr< const Base::Math::Space3D::TriangleMesh< float > > mesh, bool twoSided = false) noexcept;

			/** @brief The out-of-line destructor (the key function: one typeinfo across shared libraries). */
			~TriangleMeshCollisionModel () override;

			TriangleMeshCollisionModel (const TriangleMeshCollisionModel &) = default;
			TriangleMeshCollisionModel (TriangleMeshCollisionModel &&) = default;
			TriangleMeshCollisionModel & operator= (const TriangleMeshCollisionModel &) = default;
			TriangleMeshCollisionModel & operator= (TriangleMeshCollisionModel &&) = default;

			/**
			 * @brief Builds a model from the triangles of a shape (the geometry a Toolkit instance is made of).
			 * @note A triangle whose winding opposes its vertices' normals is reversed: the front face is the one the
			 * author's normals point out of, whatever the generator's winding.
			 * @param shape A reference to the shape.
			 * @param twoSided Whether the back faces collide too. Default false.
			 * @param activeEdgeCosine The cosine under which a convex edge is flat. Default DefaultActiveEdgeCosine.
			 * @return std::unique_ptr< TriangleMeshCollisionModel > nullptr when the shape has no valid triangle.
			 */
			[[nodiscard]]
			static std::unique_ptr< TriangleMeshCollisionModel > fromShape (const Base::VertexFactory::Shape< float > & shape, bool twoSided = false, float activeEdgeCosine = DefaultActiveEdgeCosine) noexcept;

			/** @copydoc CollisionModelInterface::modelType() */
			[[nodiscard]]
			CollisionModelType
			modelType () const noexcept override
			{
				return CollisionModelType::TriangleMesh;
			}

			/** @copydoc CollisionModelInterface::getAABB() const */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float > getAABB () const noexcept override;

			/** @copydoc CollisionModelInterface::getAABB(const Base::Math::CartesianFrame< float > &) const */
			[[nodiscard]]
			Base::Math::Space3D::AACuboid< float > getAABB (const Base::Math::CartesianFrame< float > & worldFrame) const noexcept override;

			/** @copydoc CollisionModelInterface::getRadius() */
			[[nodiscard]]
			float getRadius () const noexcept override;

			/** @copydoc CollisionModelInterface::centerOfMassOffset() */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			centerOfMassOffset (const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				/* Static only: never solved as a body. */
				return {};
			}

			/** @copydoc CollisionModelInterface::solidInertia() */
			[[nodiscard]]
			std::optional< Base::Math::Matrix< 3, float > >
			solidInertia (float /*mass*/, const Base::Math::Vector< 3, float > & /*scaling*/) const noexcept override
			{
				return std::nullopt;
			}

			/** @copydoc CollisionModelInterface::overrideShapeParameters() @note The triangles ARE the shape: nothing to override. */
			void
			overrideShapeParameters (const Base::Math::Vector< 3, float > & /*dimensions*/, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{

			}

			/** @copydoc CollisionModelInterface::areShapeParametersOverridden() @note Always: the entity never fits it to its visuals. */
			[[nodiscard]]
			bool
			areShapeParametersOverridden () const noexcept override
			{
				return true;
			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Vector< 3, float > &, const Base::Math::Vector< 3, float > &) */
			void
			mergeShapeParameters (const Base::Math::Vector< 3, float > & /*dimensions*/, const Base::Math::Vector< 3, float > & /*centerOffset*/) noexcept override
			{

			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::AACuboid< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::AACuboid< float > & /*aabb*/) noexcept override
			{

			}

			/** @copydoc CollisionModelInterface::mergeShapeParameters(const Base::Math::Space3D::Sphere< float > &) */
			void
			mergeShapeParameters (const Base::Math::Space3D::Sphere< float > & /*sphere*/) noexcept override
			{

			}

			/** @copydoc CollisionModelInterface::resetShapeParameters() */
			void
			resetShapeParameters () noexcept override
			{

			}

			/** @brief Returns the mesh. */
			[[nodiscard]]
			const Base::Math::Space3D::TriangleMesh< float > &
			mesh () const noexcept
			{
				return *m_mesh;
			}

			/** @brief Returns whether the back faces collide too. */
			[[nodiscard]]
			bool
			isTwoSided () const noexcept
			{
				return m_twoSided;
			}

			/**
			 * @brief Calls a function with every triangle, IN WORLD SPACE, whose bounds may overlap a world box.
			 * @tparam function_t A callable taking (const Triangle< float > & worldTriangle, const Vector< 3, float > &
			 * worldFaceNormal, uint32_t triangleIndex, uint8_t activeEdges).
			 * @param worldFrame The entity's world frame.
			 * @param worldRegion The world box.
			 * @param function The callable.
			 * @return void
			 */
			template< typename function_t >
			void
			forEachWorldTriangle (const Base::Math::CartesianFrame< float > & worldFrame, const Base::Math::Space3D::AACuboid< float > & worldRegion, const function_t & function) const noexcept
			{
				const auto transform = WorldTransform::of(worldFrame);

				if ( !transform.valid )
				{
					return;
				}

				Base::Math::Vector< 3, float > localMinimum;
				Base::Math::Vector< 3, float > localMaximum;

				transform.localBounds(worldRegion.minimum(), worldRegion.maximum(), localMinimum, localMaximum);

				m_mesh->visit(localMinimum, localMaximum, [&] (uint32_t index) {
					const auto & local = m_mesh->triangle(index);
					const Base::Math::Space3D::Triangle< float > world{transform.toWorld(local.pointA()), transform.toWorld(local.pointB()), transform.toWorld(local.pointC())};
					Base::Math::Vector< 3, float > normal;

					/* A mirroring frame (an odd number of negative scales) reverses the winding: the front face keeps
					 * pointing out of the mesh. */
					if ( !Base::Math::Space3D::TriangleDetail::unitNormal(world, normal) )
					{
						return;
					}

					if ( transform.mirrored )
					{
						normal = -normal;
					}

					function(world, normal, index, m_mesh->activeEdges(index));
				});
			}

		private:

			/** @brief The entity's frame as a point transform (scaling, then rotation, then translation). */
			struct WorldTransform final
			{
				Base::Math::Matrix< 3, float > rotation;
				Base::Math::Vector< 3, float > position;
				Base::Math::Vector< 3, float > scaling;
				Base::Math::Vector< 3, float > inverseScaling;
				bool mirrored{false};
				bool valid{false};

				[[nodiscard]]
				static WorldTransform of (const Base::Math::CartesianFrame< float > & frame) noexcept;

				[[nodiscard]]
				Base::Math::Vector< 3, float >
				toWorld (const Base::Math::Vector< 3, float > & local) const noexcept
				{
					const Base::Math::Vector< 3, float > scaled{local[Base::Math::X] * scaling[Base::Math::X], local[Base::Math::Y] * scaling[Base::Math::Y], local[Base::Math::Z] * scaling[Base::Math::Z]};

					return position + (rotation * scaled);
				}

				/** @brief The local box enclosing a world box (its 8 corners taken back). */
				void localBounds (const Base::Math::Vector< 3, float > & worldMinimum, const Base::Math::Vector< 3, float > & worldMaximum, Base::Math::Vector< 3, float > & localMinimum, Base::Math::Vector< 3, float > & localMaximum) const noexcept;
			};

			std::shared_ptr< const Base::Math::Space3D::TriangleMesh< float > > m_mesh;
			bool m_twoSided{false};
	};
}
