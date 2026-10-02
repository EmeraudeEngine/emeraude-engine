/*
 * src/Scenes/Scene.physics.cpp
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

#include "Scene.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <span>

/* Local inclusions. */
#include "Math/RigidBody.hpp"
#include "Math/Space3D/Contacts/ContactManifold.hpp"
#include "Physics/BodyInertia.hpp"
#include "Physics/CapsuleCollisionModel.hpp"
#include "Physics/CharacterController.hpp"
#include "Physics/NarrowPhase.hpp"
#include "Physics/TriangleMeshCollisionModel.hpp"
#include "Component/CharacterController.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Physics;

	namespace
	{
		/** @brief Below this speed (m/s) a body counts as slow for its island's sleep (P5, decision 13). */
		constexpr float SleepLinearSpeed{0.05F};

		/** @brief Below this angular speed (rad/s) a body counts as slow. */
		constexpr float SleepAngularSpeed{0.05F};

		/** @brief How many consecutive slow steps every body of an island needs before it sleeps (0.5 s at 60 Hz). */
		constexpr uint16_t SleepSteps{30};

		/** @brief Above this speed (m/s) a kinematic body (a platform, a character) wakes a sleeping one it touches. */
		constexpr float KinematicWakeSpeed{1.0e-3F};

		/**
		 * @brief Whether an entity is a body the solver moves: movable, not a kinematic character, not a triangle mesh (a
		 * mesh is never solved as a body — on a movable entity it is kinematic, P5).
		 */
		[[nodiscard]]
		bool
		isSolvedBody (const AbstractEntity & entity, const MovableTrait * movable) noexcept
		{
			if ( movable == nullptr || !movable->isMovable() || entity.characterController() != nullptr )
			{
				return false;
			}

			const auto * model = entity.collisionModel();

			return model == nullptr || model->modelType() != CollisionModelType::TriangleMesh;
		}

		/**
		 * @brief Applies complete collision response: velocity bounce + grounded state.
		 * @param movable The movable trait to update.
		 * @param surfaceNormal The dominant collision surface normal.
		 * @param groundPenetration The ground penetration depth (0 if no direct ground collision).
		 * @param dominantSource The source of the dominant collision (Ground, Boundary, or Entity).
		 * @param groundedOnEntity Pointer to the entity we collided with (if source is Entity).
		 */
		void
		applyCollisionResponse (MovableTrait * movable, const Vector< 3, float > & surfaceNormal, float groundPenetration, GroundedSource dominantSource, const MovableTrait * groundedOnEntity) noexcept
		{
			auto velocity = movable->linearVelocity();
			const float vn = Vector< 3, float >::dotProduct(velocity, surfaceNormal);

			/* Apply velocity bounce if moving into surface.
			 * vn > 0 means velocity is going INTO the surface (same direction as normal). */
			if ( vn > 0.0F )
			{
				velocity -= surfaceNormal * vn * (1.0F + movable->getBodyPhysicalProperties().bounciness());
				movable->setLinearVelocity(velocity);
			}

			/* Apply grounded response if standing on a surface.
			 * Surface is considered "ground" if:
			 * - Direct ground collision (groundPenetration > 0), OR
			 * - Normal points downward (Y < -0.7 in Y-up = surface faces up) */
			constexpr auto GroundNormalThreshold{0.7F}; /* ~45 degrees */
			const bool isOnSurface = (groundPenetration > 0.0F) || (surfaceNormal[Y] < -GroundNormalThreshold);

			/* Only apply grounded response if not bouncing away (velocity Y near zero or negative). */
			if ( isOnSurface && velocity[Y] <= 0.1F )
			{
				velocity[Y] = 0.0F;
				movable->setLinearVelocity(velocity);

				/* Set grounded with appropriate source.
				 * Priority: Ground > Boundary > Entity (ground is always ground if detected). */
				if ( groundPenetration > 0.0F )
				{
					movable->setGrounded(GroundedSource::Ground);
				}
				else
				{
					movable->setGrounded(dominantSource, groundedOnEntity);
				}
			}
		}
	}

	namespace
	{
		/** @brief The persistent key B of a ground manifold (no entity has this creation number). */
		constexpr uint64_t GroundKey{std::numeric_limits< uint64_t >::max()};

		/** @brief The default ground material (P2: GroundLevelInterface carries none): the body's own feel dominates. */
		constexpr float GroundFriction{1.0F};
		constexpr float GroundRestitution{0.0F};

		/** @brief A support is a contact whose normal is within ~45° of the gravity direction. */
		constexpr float SupportCosine{0.7F};

		/** @brief Under this approach speed (m/s) a contact is no impact: a body RESTING on another approaches at ~1e-6
		 * m/s, and notifying NodeCollision for it fired once per cycle per resting body (1001 per cycle in
		 * balls-of-steel, 2026-10-01) — the former pipeline fired nothing at rest. */
		constexpr float ImpactSpeedThreshold{0.05F};

		/**
		 * @brief Converts a base contact manifold into a solver manifold (the margin is removed from the depth).
		 */
		void
		appendManifold (const Space3D::ContactManifold< float > & contact, uint32_t bodyA, uint32_t bodyB, uint64_t keyA, uint64_t keyB, uint64_t keySub, float friction, float restitution, std::vector< SoftStepSolver::Manifold > & manifolds) noexcept
		{
			if ( contact.empty() )
			{
				return;
			}

			SoftStepSolver::Manifold manifold;
			manifold.normal = contact.normal();
			manifold.keyA = keyA;
			manifold.keyB = keyB;
			manifold.keySub = keySub;
			manifold.bodyA = bodyA;
			manifold.bodyB = bodyB;
			manifold.friction = friction;
			manifold.restitution = restitution;

			for ( const auto & point : contact.points() )
			{
				SoftStepSolver::Point solverPoint;
				solverPoint.position = point.position();
				/* Shape A was inflated by the speculative margin: the true separation is margin − depth. */
				solverPoint.separation = SoftStepSolver::SpeculativeMargin - point.depth();
				solverPoint.featureId = point.featureId();

				manifold.points.push_back(solverPoint);
			}

			manifolds.push_back(manifold);
		}

		/**
		 * @brief Finds the height of a body's centre over the ground triangle vertically over or under it.
		 */
		class GroundHeightProbe final : public GroundTriangleVisitor
		{
			public:

				GroundHeightProbe (const CollisionModelInterface & model, const CartesianFrame< float > & frame) noexcept
					: m_model{model},
					m_frame{frame}
				{

				}

				void
				onTriangle (const Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
				{
					/* The first triangle in the grid order: deterministic for a centre on a shared edge. */
					if ( !m_found && NarrowPhase::heightOverGround(m_model, m_frame, triangle, m_height, m_normal) )
					{
						m_found = true;
					}
				}

				/**
				 * @brief Returns whether the centre is under the ground.
				 * @return bool
				 */
				[[nodiscard]]
				bool
				isUnderground () const noexcept
				{
					return m_found && m_height < 0.0F;
				}

				/**
				 * @brief Returns the centre's height over the surface under it (negative = under).
				 * @return float
				 */
				[[nodiscard]]
				float
				height () const noexcept
				{
					return m_height;
				}

				/**
				 * @brief Returns the normal of the triangle under the centre (up side).
				 * @return const Vector< 3, float > &
				 */
				[[nodiscard]]
				const Vector< 3, float > &
				normal () const noexcept
				{
					return m_normal;
				}

			private:

				const CollisionModelInterface & m_model;
				const CartesianFrame< float > & m_frame;
				Vector< 3, float > m_normal;
				float m_height{0.0F};
				bool m_found{false};
		};

		/**
		 * @brief Collects the ground triangles under a body into solver manifolds.
		 */
		class GroundContactCollector final : public GroundTriangleVisitor
		{
			public:

				GroundContactCollector (const CollisionModelInterface & model, const CartesianFrame< float > & frame, uint32_t bodyIndex, uint64_t creationNumber, float friction, float restitution, std::vector< SoftStepSolver::Manifold > & manifolds) noexcept
					: m_model{model},
					m_frame{frame},
					m_manifolds{manifolds},
					m_creationNumber{creationNumber},
					m_bodyIndex{bodyIndex},
					m_friction{friction},
					m_restitution{restitution}
				{

				}

				void
				onTriangle (const Space3D::Triangle< float > & triangle, uint32_t featureId) noexcept override
				{
					/* One-sided (NarrowPhase::generateGround()). */
					if ( NarrowPhase::generateGround(m_model, m_frame, triangle, SoftStepSolver::SpeculativeMargin, m_contact) )
					{
						/* A = the body, B = the static world; the sub key tells the triangles apart. */
						appendManifold(m_contact, m_bodyIndex, 0, m_creationNumber, GroundKey, static_cast< uint64_t >(featureId) + 1, m_friction, m_restitution, m_manifolds);
					}
				}

			private:

				const CollisionModelInterface & m_model;
				const CartesianFrame< float > & m_frame;
				std::vector< SoftStepSolver::Manifold > & m_manifolds;
				Space3D::ContactManifold< float > m_contact;
				uint64_t m_creationNumber;
				uint32_t m_bodyIndex;
				float m_friction;
				float m_restitution;
		};

		/** @brief A fast body sweeps when it moved more than this fraction of its round core in one step. */
		constexpr float ContinuousCoreFraction{0.5F};

		/** @brief The distance a swept body stops short of its first contact (m). */
		constexpr float ContinuousSlop{0.005F};

		/**
		 * @brief Sweeps a body's round core against the ground triangles of a region; keeps the earliest contact.
		 */
		class ContinuousGroundSweep final : public GroundTriangleVisitor
		{
			public:

				ContinuousGroundSweep (const CollisionModelInterface & model, const CartesianFrame< float > & frame, const Vector< 3, float > & motion) noexcept
					: m_model{model},
					m_frame{frame},
					m_motion{motion}
				{

				}

				void
				onTriangle (const Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
				{
					Space3D::CastHit< float > hit;

					/* Touching at the start is the contacts' business, not a crossing; and the ground is one-sided: a triangle
					 * met from UNDER it stops nothing (the recovery, step 1b, lifts a body under the ground). Without the
					 * second rule a ball stopped every step by a triangle the contacts ignored stayed frozen while gravity
					 * kept adding to its velocity — 110 m/s after 25 s. */
					if ( !NarrowPhase::sweepCore(m_model, m_frame, m_motion, triangle, hit) || hit.startedInside() || hit.normal()[Y] <= 0.0F )
					{
						return;
					}

					if ( hit.fraction() < m_earliest )
					{
						m_earliest = hit.fraction();
						m_normal = hit.normal();
						m_point = hit.point();
					}
				}

				/**
				 * @brief Returns the contact point of the earliest contact.
				 * @return const Vector< 3, float > &
				 */
				[[nodiscard]]
				const Vector< 3, float > &
				point () const noexcept
				{
					return m_point;
				}

				/**
				 * @brief Returns the surface normal at the earliest contact (towards the body).
				 * @return const Vector< 3, float > &
				 */
				[[nodiscard]]
				const Vector< 3, float > &
				normal () const noexcept
				{
					return m_normal;
				}

				/**
				 * @brief Returns the earliest contact fraction (1 = none).
				 * @return float
				 */
				[[nodiscard]]
				float
				earliest () const noexcept
				{
					return m_earliest;
				}

			private:

				const CollisionModelInterface & m_model;
				const CartesianFrame< float > & m_frame;
				const Vector< 3, float > & m_motion;
				Vector< 3, float > m_normal;
				Vector< 3, float > m_point;
				float m_earliest{1.0F};
		};

		/** @brief Whether two world AABBs overlap, the first inflated by a margin. */
		[[nodiscard]]
		bool boxesOverlap (const Space3D::AACuboid< float > & boxA, const Space3D::AACuboid< float > & boxB, float margin) noexcept;

		/** @brief The world box of a capsule, stretched along a motion. */
		[[nodiscard]]
		Space3D::AACuboid< float >
		sweptCapsuleBounds (const Space3D::Capsule< float > & capsule, const Vector< 3, float > & motion) noexcept
		{
			const auto radius = capsule.radius();
			const Vector< 3, float > reach{radius, radius, radius};
			const auto & start = capsule.startPoint();
			const auto & end = capsule.endPoint();

			Space3D::AACuboid< float > bounds{start + reach, start - reach};
			bounds.merge(Space3D::AACuboid< float >{end + reach, end - reach});
			bounds.merge(Space3D::AACuboid< float >{bounds.maximum() + motion, bounds.minimum() + motion});

			return bounds;
		}

		/**
		 * @brief The world as a kinematic character sees it during the physics step (P4): the ground's triangles and the
		 * physics entities, the character itself excluded.
		 */
		class SceneCharacterWorld final : public CharacterWorldInterface
		{
			public:

				SceneCharacterWorld (const std::vector< std::shared_ptr< AbstractEntity > > & entities, size_t self, const GroundLevelInterface * ground, const std::unordered_map< uint64_t, Vector< 3, float > > & lastKinematicPositions, float stepSeconds) noexcept
					: m_entities{entities},
					m_ground{ground},
					m_lastKinematicPositions{lastKinematicPositions},
					m_self{self},
					m_stepSeconds{stepSeconds}
				{

				}

				[[nodiscard]]
				bool
				sweep (const Space3D::Capsule< float > & capsule, const Vector< 3, float > & motion, Hit & hit) const noexcept override
				{
					const auto region = sweptCapsuleBounds(capsule, motion);
					bool found = false;

					/* The ground, one-sided: a triangle met from under it stops nothing. */
					if ( m_ground != nullptr )
					{
						class Sweep final : public GroundTriangleVisitor
						{
							public:

								Sweep (const Space3D::Capsule< float > & caster, const Vector< 3, float > & move, Hit & result, bool & foundAny) noexcept
									: m_caster{caster}, m_move{move}, m_result{result}, m_found{foundAny}
								{

								}

								void
								onTriangle (const Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
								{
									Space3D::CastHit< float > cast;

									if ( !NarrowPhase::sweepCapsule(m_caster, m_move, triangle, cast) || cast.startedInside() || cast.normal()[Y] <= 0.0F || cast.fraction() >= m_result.fraction )
									{
										return;
									}

									m_result = Hit{.point = cast.point(), .normal = cast.normal(), .velocity = {}, .key = GroundKey, .fraction = cast.fraction(), .bodyIndex = 0, .dynamic = false};
									m_found = true;
								}

							private:

								const Space3D::Capsule< float > & m_caster;
								const Vector< 3, float > & m_move;
								Hit & m_result;
								bool & m_found;
						};

						hit.fraction = 1.0F;

						Sweep visitor{capsule, motion, hit, found};
						static_cast< void >(m_ground->visitTriangles(region, visitor));
					}
					else
					{
						hit.fraction = 1.0F;
					}

					/* The entities. */
					for ( size_t index = 0; index < m_entities.size(); ++index )
					{
						const auto & entity = m_entities[index];

						if ( index == m_self || !entity->hasCollisionModel() || !entity->isCollidable() )
						{
							continue;
						}

						const auto frame = entity->getWorldCoordinates();

						if ( !boxesOverlap(region, entity->collisionModel()->getAABB(frame), 0.0F) )
						{
							continue;
						}

						Space3D::CastHit< float > cast;

						if ( entity->collisionModel()->modelType() == CollisionModelType::TriangleMesh )
						{
							/* A mesh: its earliest front-face hit (P5). */
							const auto & mesh = static_cast< const TriangleMeshCollisionModel & >(*entity->collisionModel());
							bool met = false;

							mesh.forEachWorldTriangle(frame, region, [&] (const Space3D::Triangle< float > & triangle, const Vector< 3, float > & faceNormal, uint32_t /*triangleIndex*/, uint8_t activeEdges) {
								Space3D::CastHit< float > candidate;

								if ( !NarrowPhase::sweepCapsule(capsule, motion, triangle, candidate) || candidate.startedInside() || candidate.fraction() >= hit.fraction || (met && candidate.fraction() >= cast.fraction()) )
								{
									return;
								}

								if ( NarrowPhase::acceptMeshHit(triangle, faceNormal, activeEdges, mesh.isTwoSided(), candidate) )
								{
									cast = candidate;
									met = true;
								}
							});

							if ( !met )
							{
								continue;
							}
						}
						else if ( !NarrowPhase::sweepCapsule(capsule, motion, *entity->collisionModel(), frame, cast) || cast.startedInside() || cast.fraction() >= hit.fraction )
						{
							continue;
						}

						const auto * movable = entity->getMovableTrait();
						const bool dynamic = isSolvedBody(*entity, movable);

						hit = Hit{.point = cast.point(), .normal = cast.normal(), .velocity = this->velocityOf(entity, movable, dynamic), .key = entity->creationNumber(), .fraction = cast.fraction(), .bodyIndex = static_cast< uint32_t >(index), .dynamic = dynamic};
						found = true;
					}

					return found;
				}

				[[nodiscard]]
				Vector< 3, float >
				penetrationCorrection (const Space3D::Capsule< float > & capsule) const noexcept override
				{
					Vector< 3, float > correction;
					Space3D::ContactManifold< float > contact;

					/* Out along each contact by its deepest depth, never twice along the same direction (two coplanar
					 * triangles under the capsule both report it). */
					const auto apply = [&correction] (const Space3D::ContactManifold< float > & manifold) {
						const auto out = -manifold.normal();
						const auto depth = manifold.maximumDepth();
						const auto along = Vector< 3, float >::dotProduct(correction, out);

						if ( depth > along )
						{
							correction += out * (depth - along);
						}
					};

					const auto region = sweptCapsuleBounds(capsule, Vector< 3, float >{});

					if ( m_ground != nullptr )
					{
						class Overlap final : public GroundTriangleVisitor
						{
							public:

								Overlap (const Space3D::Capsule< float > & caster, const decltype(apply) & onContact) noexcept
									: m_caster{caster}, m_onContact{onContact}
								{

								}

								void
								onTriangle (const Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
								{
									Space3D::ContactManifold< float > manifold;

									if ( NarrowPhase::capsuleGroundContacts(m_caster, triangle, manifold) )
									{
										m_onContact(manifold);
									}
								}

							private:

								const Space3D::Capsule< float > & m_caster;
								const decltype(apply) & m_onContact;
						};

						Overlap visitor{capsule, apply};
						static_cast< void >(m_ground->visitTriangles(region, visitor));
					}

					for ( size_t index = 0; index < m_entities.size(); ++index )
					{
						const auto & entity = m_entities[index];

						if ( index == m_self || !entity->hasCollisionModel() || !entity->isCollidable() )
						{
							continue;
						}

						/* A dynamic body never pushes a character (decision 9.4). */
						const auto * movable = entity->getMovableTrait();

						if ( isSolvedBody(*entity, movable) )
						{
							continue;
						}

						const auto frame = entity->getWorldCoordinates();

						if ( !boxesOverlap(region, entity->collisionModel()->getAABB(frame), 0.0F) )
						{
							continue;
						}

						if ( entity->collisionModel()->modelType() == CollisionModelType::TriangleMesh )
						{
							const auto & mesh = static_cast< const TriangleMeshCollisionModel & >(*entity->collisionModel());

							mesh.forEachWorldTriangle(frame, region, [&] (const Space3D::Triangle< float > & triangle, const Vector< 3, float > & faceNormal, uint32_t /*triangleIndex*/, uint8_t activeEdges) {
								if ( NarrowPhase::capsuleMeshTriangleContacts(capsule, triangle, faceNormal, activeEdges, mesh.isTwoSided(), contact) )
								{
									apply(contact);
								}
							});
						}
						else if ( NarrowPhase::capsuleContacts(capsule, *entity->collisionModel(), frame, contact) )
						{
							apply(contact);
						}
					}

					return correction;
				}

				void
				push (uint32_t bodyIndex, const Vector< 3, float > & point, const Vector< 3, float > & impulse) noexcept override
				{
					if ( bodyIndex >= m_entities.size() )
					{
						return;
					}

					const auto & entity = m_entities[bodyIndex];
					auto * movable = entity->getMovableTrait();

					if ( movable == nullptr || !movable->isMovable() )
					{
						return;
					}

					const auto & properties = entity->bodyPhysicalProperties();

					if ( properties.isMassNull() )
					{
						return;
					}

					movable->setLinearVelocity(movable->linearVelocity() + (impulse * properties.inverseMass()));

					if ( movable->isRotationPhysicsEnabled() && entity->hasCollisionModel() )
					{
						const auto frame = entity->getWorldCoordinates();
						const auto rotation = frame.getRotationMatrix3();
						const auto centre = frame.position() + (rotation * entity->collisionModel()->centerOfMassOffset(frame.scalingFactor()));
						const auto inverseInertia = worldInverseInertia(localInverseInertia(properties, entity->collisionModel(), frame.scalingFactor()), rotation);

						movable->setAngularVelocity(movable->angularVelocity() + (inverseInertia * Vector< 3, float >::crossProduct(point - centre, impulse)));
					}
				}

			private:

				/** @brief The velocity of an obstacle (a dynamic body's, or a kinematic body's motion since the last step). */
				[[nodiscard]]
				Vector< 3, float >
				velocityOf (const std::shared_ptr< AbstractEntity > & entity, const MovableTrait * movable, bool dynamic) const noexcept
				{
					if ( dynamic )
					{
						return movable->linearVelocity();
					}

					const auto previous = m_lastKinematicPositions.find(entity->creationNumber());

					if ( previous == m_lastKinematicPositions.end() )
					{
						return {};
					}

					return (entity->getWorldCoordinates().position() - previous->second) * (1.0F / m_stepSeconds);
				}

				const std::vector< std::shared_ptr< AbstractEntity > > & m_entities;
				const GroundLevelInterface * m_ground;
				const std::unordered_map< uint64_t, Vector< 3, float > > & m_lastKinematicPositions;
				size_t m_self;
				float m_stepSeconds;
		};

		/** @brief Whether two world AABBs overlap, the first inflated by a margin. */
		[[nodiscard]]
		bool
		boxesOverlap (const Space3D::AACuboid< float > & boxA, const Space3D::AACuboid< float > & boxB, float margin) noexcept
		{
			for ( size_t axis = 0; axis < 3; ++axis )
			{
				if ( boxA.minimum(axis) - margin > boxB.maximum(axis) || boxB.minimum(axis) > boxA.maximum(axis) + margin )
				{
					return false;
				}
			}

			return true;
		}
	}

	void
	Scene::resolveCollisions (std::vector< std::shared_ptr< AbstractEntity > > & movedEntities, std::vector< PhysicsImpact > & impacts) const noexcept
	{
		movedEntities.clear();
		impacts.clear();

		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* Lock the physics octree for the duration of the simulation to prevent
		 * concurrent modifications from other threads (e.g., checkEntityLocationInOctrees). */
		const std::scoped_lock lock{m_physicsOctreeAccess};

		constexpr float PhysicsStepSeconds = WorldPhysicsUpdateCycleDurationS< float >;
		constexpr float SpeculativeContactMargin = SoftStepSolver::SpeculativeMargin;

		auto & bodies = m_physicsBodies;
		auto & bodyEntities = m_physicsBodyEntities;
		auto & manifolds = m_physicsManifolds;
		auto & bodyIndices = m_physicsBodyIndices;

		bodies.clear();
		bodyEntities.clear();
		manifolds.clear();
		bodyIndices.clear();

		/* ============================================================
		 * 1. THE BODIES, in creation order (index 0 = the static world).
		 * ============================================================ */
		m_physicsOctree->forEachSector([&bodyEntities] (const OctreeSector< AbstractEntity, true > & /*sector*/, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			for ( size_t index = ownedOffset; index < candidates.size(); ++index )
			{
				bodyEntities.push_back(candidates[index]);
			}
		});

		std::ranges::sort(bodyEntities, [] (const auto & lhs, const auto & rhs) {
			return lhs->creationNumber() < rhs->creationNumber();
		});

		/* The octree files each entity in ONE sector, so each is met once (a duplicate made a character collide with
		 * itself and sink under the ground, 2026-10-02). Debug only: a per-cycle walk. */
		if constexpr ( IsDebug )
		{
			if ( const auto duplicate = std::ranges::adjacent_find(bodyEntities); duplicate != bodyEntities.end() )
			{
				TraceError{ClassId} << "The physics octree holds '" << (*duplicate)->name() << "' twice !";
			}
		}

		/* ============================================================
		 * 0. THE KINEMATIC CHARACTERS (P4): moved by their controller before anything is solved — collide and slide,
		 * steps, slopes, the support's velocity, the ground probe. They are kinematic bodies below (their velocity is
		 * their motion), so the dynamic bodies meet them where they now are.
		 * ============================================================ */
		m_physicsCharacters.clear();

		const auto & gravity = m_environmentPhysicalProperties.surfaceGravity();
		const auto gravityLength = gravity.length();
		const auto upward = gravityLength > 0.0F ? gravity * (-1.0F / gravityLength) : Vector< 3, float >::positiveY();

		for ( size_t index = 0; index < bodyEntities.size(); ++index )
		{
			const auto & entity = bodyEntities[index];
			auto character = entity->characterController();

			if ( character == nullptr )
			{
				continue;
			}

			auto & controller = character->controller();

			/* The entity's free fly mode makes its character fly (no gravity, the wanted velocity in 3D). */
			{
				const auto * movable = entity->getMovableTrait();

				controller.setFlying(movable != nullptr && movable->isFreeFlyModeEnabled());
			}

			/* The capsule follows the controller's size (set after linking, or changed during the game). */
			{
				const auto wanted = controller.localCapsule();
				const auto * model = entity->collisionModel();
				constexpr float SizeTolerance{1.0e-6F};
				bool current = model != nullptr && model->modelType() == CollisionModelType::Capsule;

				if ( current )
				{
					const auto & present = static_cast< const CapsuleCollisionModel * >(model)->localCapsule();

					current = std::abs(present.radius() - wanted.radius()) <= SizeTolerance &&
						(present.startPoint() - wanted.startPoint()).length() <= SizeTolerance &&
						(present.endPoint() - wanted.endPoint()).length() <= SizeTolerance;
				}

				if ( !current )
				{
					entity->setCollisionModel(std::make_unique< CapsuleCollisionModel >(wanted, true));
				}
			}

			SceneCharacterWorld world{bodyEntities, index, m_groundLevel.get(), m_kinematicLastPositions, PhysicsStepSeconds};
			const auto feet = entity->getWorldCoordinates().position();
			const auto reached = controller.step(world, feet, upward, gravityLength, PhysicsStepSeconds);

			if ( (reached - feet).lengthSquared() > 0.0F )
			{
				entity->setPosition(reached, TransformSpace::World);
				movedEntities.push_back(entity);
			}

			/* The grounded state the game already reads (MovableTrait). */
			if ( auto * movable = entity->getMovableTrait(); movable != nullptr )
			{
				if ( controller.isGrounded() )
				{
					movable->setGrounded(controller.supportKey() == GroundKey ? GroundedSource::Ground : GroundedSource::Entity);
				}
				else
				{
					movable->clearGrounded();
				}
			}

			m_physicsCharacters.push_back(std::move(character));
		}

		/* The static world first. */
		bodies.emplace_back();
		bodyEntities.insert(bodyEntities.begin(), nullptr);

		for ( size_t entityIndex = 1; entityIndex < bodyEntities.size(); ++entityIndex )
		{
			const auto & entity = bodyEntities[entityIndex];
			const auto frame = entity->getWorldCoordinates();

			if ( !entity->hasMovableAbility() || entity->getMovableTrait() == nullptr )
			{
				/* A static entity: part of the static world (it never moves). */
				bodyIndices[entity.get()] = 0;
				bodies.emplace_back();

				continue;
			}

			auto * movable = entity->getMovableTrait();
			const auto rotation = frame.getRotationMatrix3();
			const auto * model = entity->collisionModel();
			SoftStepSolver::Body body;
			/* P3: the body turns about the centroid of its shape (decision 8b). */
			body.centerOffset = model != nullptr ? rotation * model->centerOfMassOffset(frame.scalingFactor()) : Vector< 3, float >{};
			body.position = frame.position() + body.centerOffset;
			body.orientation = frame.toQuaternion();

			if ( isSolvedBody(*entity, movable) )
			{
				const auto & properties = movable->getBodyPhysicalProperties();

				body.movable = movable;
				body.linearVelocity = movable->linearVelocity();
				body.angularVelocity = movable->isRotationPhysicsEnabled() ? movable->angularVelocity() : Vector< 3, float >{};
				body.inverseMass = properties.inverseMass();
				/* P3: the explicit tensor, else the shape's as a uniform solid (decision 8c); zero = no rotation. */
				body.inverseInertia = movable->isRotationPhysicsEnabled() ? worldInverseInertia(localInverseInertia(properties, model, frame.scalingFactor()), rotation) : Matrix< 3, float >{std::array< float, 9 >{}};
				body.gravity = !movable->isFreeFlyModeEnabled() && !properties.isMassNull();
				/* A sleeping body is solid but still; contact with an awake one wakes it (below). */
				body.dynamic = !entity->isSimulationPaused();
			}
			else
			{
				/* KINEMATIC (an animated, non-movable node, or a character): infinite mass, the velocity of its own motion. */
				const auto creation = entity->creationNumber();

				if ( const auto previous = m_kinematicLastPositions.find(creation); previous != m_kinematicLastPositions.end() )
				{
					body.linearVelocity = (body.position - previous->second) * (1.0F / PhysicsStepSeconds);
				}

				m_kinematicLastPositions[creation] = body.position;
			}

			bodyIndices[entity.get()] = static_cast< uint32_t >(bodies.size());
			bodies.push_back(body);
		}

		/* ============================================================
		 * 1c. A SLEEPING ISLAND WAKES AS A WHOLE (P5, decision 13): one of its bodies woken since the last step — a
		 * contact, a push, a force, a move — wakes the others, which fell asleep with the same island key.
		 * ============================================================ */
		{
			auto & waking = m_physicsWakingIslands;

			waking.clear();

			for ( size_t index = 1; index < bodies.size(); ++index )
			{
				const auto & body = bodies[index];

				if ( body.movable != nullptr && body.dynamic && body.movable->sleepIsland() != 0 )
				{
					waking.push_back(body.movable->sleepIsland());
				}
			}

			if ( !waking.empty() )
			{
				std::ranges::sort(waking);

				const auto duplicates = std::ranges::unique(waking);

				waking.erase(duplicates.begin(), duplicates.end());

				for ( size_t index = 1; index < bodies.size(); ++index )
				{
					auto & body = bodies[index];

					if ( body.movable == nullptr || body.movable->sleepIsland() == 0 || !std::ranges::binary_search(waking, body.movable->sleepIsland()) )
					{
						continue;
					}

					body.movable->setSleepIsland(0);

					if ( !body.dynamic )
					{
						body.dynamic = true;
						bodyEntities[index]->pauseSimulation(false);
					}
				}
			}
		}

		/* ============================================================
		 * 1b. GROUND RECOVERY: a dynamic body whose centre crossed the ground within one step (a ball at 43 m/s moves
		 * 0.72 m per step, more than its radius) is put back ON it, before any contact is built. The ground is the top
		 * of a solid (owner, 2026-10-02): the bottom of the body's world box goes onto the surface vertically under its
		 * centre, and its velocity bounces off that surface as a contact would. Before, such a body got a contact normal
		 * pointing DOWN and went through (balls-of-steel: 82 of 400 balls under the terrain); recovering it with the
		 * solver's 3 m/s pushout let a ball entering a hillside at 26 m/s sink deeper faster than it came back up.
		 * ============================================================ */
		if ( m_groundLevel != nullptr )
		{
			for ( size_t index = 1; index < bodies.size(); ++index )
			{
				auto & body = bodies[index];
				const auto & entity = bodyEntities[index];

				if ( body.movable == nullptr || !body.dynamic || !entity->hasCollisionModel() )
				{
					continue;
				}

				const auto frame = entity->getWorldCoordinates();
				const auto * model = entity->collisionModel();
				const auto bounds = model->getAABB(frame);
				const Space3D::AACuboid< float > region{bounds.maximum() + Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}, bounds.minimum() - Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}};

				GroundHeightProbe probe{*model, frame};
				static_cast< void >(m_groundLevel->visitTriangles(region, probe));

				if ( !probe.isUnderground() )
				{
					continue;
				}

				/* The surface under the centre (body.position IS the shape's centroid, P3), the box bottom onto it. */
				const auto surface = body.position[Y] - probe.height();
				const auto lift = surface - bounds.minimum(Y);

				if ( !(lift > 0.0F) || !std::isfinite(lift) )
				{
					continue;
				}

				const Vector< 3, float > shift{0.0F, lift, 0.0F};

				body.position += shift;
				body.movable->moveFromPhysics(shift);
				movedEntities.push_back(entity);

				/* The bounce a contact would have given: v' = v − (1 + e) (v · n) n, approaching only. */
				const auto & normal = probe.normal();
				const auto approach = Vector< 3, float >::dotProduct(body.linearVelocity, normal);

				if ( approach < 0.0F )
				{
					const auto restitution = std::max(entity->bodyPhysicalProperties().bounciness(), GroundRestitution);

					body.linearVelocity -= normal * ((1.0F + restitution) * approach);
				}
			}
		}

		/* ============================================================
		 * 2. THE PAIRS (the octree's pairing contract: owned × owned, owned × inherited, each pair once).
		 * ============================================================ */
		const auto isActive = [&bodies] (uint32_t index) {
			return index != 0 && bodies[index].movable != nullptr && bodies[index].dynamic;
		};

		const auto isAsleep = [&bodies] (uint32_t index) {
			return index != 0 && bodies[index].movable != nullptr && !bodies[index].dynamic;
		};

		/* A kinematic body in motion (a platform, a character) wakes a sleeping body it touches (Box2D v3 does the same):
		 * a box asleep on a stopped platform would otherwise be crossed when the platform leaves. */
		const auto isMovingKinematic = [&bodies] (uint32_t index) {
			return index != 0 && bodies[index].movable == nullptr && bodies[index].linearVelocity.lengthSquared() > KinematicWakeSpeed * KinematicWakeSpeed;
		};

		Space3D::ContactManifold< float > contact;

		const auto testPair = [&] (const std::shared_ptr< AbstractEntity > & entityA, const std::shared_ptr< AbstractEntity > & entityB) {
			/* Canonical order: A has the lower creation number, whatever the octree's order. */
			const auto & first = entityA->creationNumber() < entityB->creationNumber() ? entityA : entityB;
			const auto & second = entityA->creationNumber() < entityB->creationNumber() ? entityB : entityA;
			const uint32_t indexA = bodyIndices[first.get()];
			const uint32_t indexB = bodyIndices[second.get()];

			/* At least one active body; two sleeping ones (or a sleeping one and the static world) need no test; a
			 * sleeping one and a moving kinematic one do (it wakes). */
			if ( !isActive(indexA) && !isActive(indexB) && !(isMovingKinematic(indexA) && isAsleep(indexB)) && !(isMovingKinematic(indexB) && isAsleep(indexA)) )
			{
				return;
			}

			if ( !first->hasCollisionModel() || !second->hasCollisionModel() )
			{
				return;
			}

			const auto frameA = first->getWorldCoordinates();
			const auto frameB = second->getWorldCoordinates();
			const auto * modelA = first->collisionModel();
			const auto * modelB = second->collisionModel();

			if ( !boxesOverlap(modelA->getAABB(frameA), modelB->getAABB(frameB), SpeculativeContactMargin) )
			{
				return;
			}

			const auto & materialA = first->bodyPhysicalProperties();
			const auto & materialB = second->bodyPhysicalProperties();
			/* Box2D's mixing: friction = geometric mean, restitution = the larger (owner, P2). */
			const float friction = std::sqrt(materialA.stickiness() * materialB.stickiness());
			const float restitution = std::max(materialA.bounciness(), materialB.bounciness());

			/* A TRIANGLE MESH (P5): one manifold per triangle met, the sub key tells them apart. Two meshes never meet. */
			const bool meshA = modelA->modelType() == CollisionModelType::TriangleMesh;
			const bool meshB = modelB->modelType() == CollisionModelType::TriangleMesh;

			if ( meshA && meshB )
			{
				return;
			}

			if ( meshA || meshB )
			{
				const auto & mesh = static_cast< const TriangleMeshCollisionModel & >(meshA ? *modelA : *modelB);
				const auto & meshFrame = meshA ? frameA : frameB;
				const auto * bodyModel = meshA ? modelB : modelA;
				const auto & bodyFrame = meshA ? frameB : frameA;
				const auto bounds = bodyModel->getAABB(bodyFrame);
				const Vector< 3, float > margin{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin};
				bool touched = false;

				mesh.forEachWorldTriangle(meshFrame, Space3D::AACuboid< float >{bounds.maximum() + margin, bounds.minimum() - margin}, [&] (const Space3D::Triangle< float > & triangle, const Vector< 3, float > & faceNormal, uint32_t triangleIndex, uint8_t activeEdges) {
					if ( !NarrowPhase::generateMeshTriangle(*bodyModel, bodyFrame, triangle, faceNormal, activeEdges, mesh.isTwoSided(), SpeculativeContactMargin, contact) )
					{
						return;
					}

					/* The manifold goes from A to B: from the body to the mesh, flipped when the mesh is A. */
					if ( meshA )
					{
						contact.flip();
					}

					touched = true;
					appendManifold(contact, indexA, indexB, first->creationNumber(), second->creationNumber(), static_cast< uint64_t >(triangleIndex) + 1, friction, restitution, manifolds);
				});

				if ( !touched )
				{
					return;
				}
			}
			else if ( !NarrowPhase::generate(*modelA, frameA, *modelB, frameB, SpeculativeContactMargin, contact) )
			{
				return;
			}

			/* Wake a sleeping body touched by an active (or moving kinematic) one: it takes part in this step, the rest of
			 * its island next step (1c). */
			if ( isAsleep(indexA) )
			{
				bodies[indexA].dynamic = true;
				bodies[indexA].movable->accountSlowness(false);
				first->pauseSimulation(false);
			}

			if ( isAsleep(indexB) )
			{
				bodies[indexB].dynamic = true;
				bodies[indexB].movable->accountSlowness(false);
				second->pauseSimulation(false);
			}

			if ( !meshA && !meshB )
			{
				appendManifold(contact, indexA, indexB, first->creationNumber(), second->creationNumber(), 0, friction, restitution, manifolds);
			}
		};

		m_physicsOctree->forEachSector([&testPair] (const OctreeSector< AbstractEntity, true > & /*sector*/, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			for ( size_t indexA = ownedOffset; indexA < candidates.size(); ++indexA )
			{
				for ( size_t indexB = indexA + 1; indexB < candidates.size(); ++indexB )
				{
					testPair(candidates[indexA], candidates[indexB]);
				}

				for ( size_t indexB = 0; indexB < ownedOffset; ++indexB )
				{
					testPair(candidates[indexA], candidates[indexB]);
				}
			}
		});

		/* ============================================================
		 * 3. THE GROUND: its rendered triangles under every active body.
		 * ============================================================ */
		if ( m_groundLevel != nullptr )
		{
			for ( size_t index = 1; index < bodies.size(); ++index )
			{
				if ( !isActive(static_cast< uint32_t >(index)) )
				{
					continue;
				}

				const auto & entity = bodyEntities[index];

				if ( !entity->hasCollisionModel() )
				{
					continue;
				}

				const auto frame = entity->getWorldCoordinates();
				const auto * model = entity->collisionModel();
				const auto & material = entity->bodyPhysicalProperties();
				const auto bounds = model->getAABB(frame);
				const Space3D::AACuboid< float > region{bounds.maximum() + Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}, bounds.minimum() - Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}};

				GroundContactCollector collector{*model, frame, static_cast< uint32_t >(index), entity->creationNumber(), std::sqrt(material.stickiness() * GroundFriction), std::max(material.bounciness(), GroundRestitution), manifolds};

				static_cast< void >(m_groundLevel->visitTriangles(region, collector));
			}
		}

		/* ============================================================
		 * 4. SOLVE, in a deterministic order.
		 * ============================================================ */
		std::ranges::sort(manifolds, [] (const auto & lhs, const auto & rhs) {
			if ( lhs.keyA != rhs.keyA )
			{
				return lhs.keyA < rhs.keyA;
			}

			if ( lhs.keyB != rhs.keyB )
			{
				return lhs.keyB < rhs.keyB;
			}

			return lhs.keySub < rhs.keySub;
		});

		m_softStepSolver.step(bodies, manifolds, m_environmentPhysicalProperties.surfaceGravity(), PhysicsStepSeconds);

		/* ============================================================
		 * 4b. CONTINUOUS COLLISION of the fast bodies (P5, the owner's idea: the segment from the old to the new
		 * position): a dynamic body that moved more than half its round core in this step sweeps that core along its
		 * motion against the static world (the ground's triangles, the static solids, the kinematic bodies) and is put
		 * back just before the first contact; the next step's contact then stops or bounces it. Without it a body
		 * moving more than its size per step went through a wall (and the ground, before step 1b).
		 * ============================================================ */
		auto & obstacles = m_continuousObstacles;
		obstacles.clear();

		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			const auto & entity = bodyEntities[index];

			if ( !bodies[index].dynamic && entity->hasCollisionModel() && entity->isCollidable() )
			{
				const auto obstacleFrame = entity->getWorldCoordinates();

				obstacles.push_back(ContinuousObstacle{.frame = obstacleFrame, .bounds = entity->collisionModel()->getAABB(obstacleFrame), .model = entity->collisionModel(), .index = static_cast< uint32_t >(index)});
			}
		}

		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			auto & body = bodies[index];
			const auto & entity = bodyEntities[index];

			if ( body.movable == nullptr || !body.dynamic || !entity->hasCollisionModel() )
			{
				continue;
			}

			const auto * model = entity->collisionModel();
			const auto frame = entity->getWorldCoordinates();
			const auto motion = body.deltaPosition;
			const auto distance = motion.length();
			const auto radius = NarrowPhase::coreRadius(*model, frame);

			if ( !(distance > std::max(radius * ContinuousCoreFraction, SpeculativeContactMargin)) )
			{
				continue;
			}

			/* The swept region: the body's box at the start, stretched along the motion. */
			const auto startBounds = model->getAABB(frame);
			auto sweptBounds = startBounds;
			sweptBounds.merge(Space3D::AACuboid< float >{startBounds.maximum() + motion, startBounds.minimum() + motion});

			float earliest = 1.0F;
			Vector< 3, float > normal;
			Vector< 3, float > point;
			float restitution = 0.0F;
			Space3D::CastHit< float > hit;
			const auto & material = entity->bodyPhysicalProperties();

			/* The ground's triangles. */
			if ( m_groundLevel != nullptr )
			{
				ContinuousGroundSweep sweep{*model, frame, motion};
				static_cast< void >(m_groundLevel->visitTriangles(sweptBounds, sweep));

				if ( sweep.earliest() < earliest )
				{
					earliest = sweep.earliest();
					normal = sweep.normal();
					point = sweep.point();
					restitution = std::max(material.bounciness(), GroundRestitution);
				}
			}

			/* The static solids, the kinematic and the sleeping bodies (the static world for this body). */
			for ( const auto & obstacle : obstacles )
			{
				if ( obstacle.index == index || !boxesOverlap(sweptBounds, obstacle.bounds, 0.0F) )
				{
					continue;
				}

				if ( obstacle.model->modelType() == CollisionModelType::TriangleMesh )
				{
					/* A mesh: its earliest front-face hit (P5). */
					const auto & mesh = static_cast< const TriangleMeshCollisionModel & >(*obstacle.model);

					mesh.forEachWorldTriangle(obstacle.frame, sweptBounds, [&] (const Space3D::Triangle< float > & triangle, const Vector< 3, float > & faceNormal, uint32_t /*triangleIndex*/, uint8_t activeEdges) {
						Space3D::CastHit< float > candidate;

						if ( NarrowPhase::sweepCore(*model, frame, motion, triangle, candidate) && !candidate.startedInside() && candidate.fraction() < earliest && NarrowPhase::acceptMeshHit(triangle, faceNormal, activeEdges, mesh.isTwoSided(), candidate) )
						{
							earliest = candidate.fraction();
							normal = candidate.normal();
							point = candidate.point();
							restitution = std::max(material.bounciness(), bodyEntities[obstacle.index]->bodyPhysicalProperties().bounciness());
						}
					});

					continue;
				}

				if ( NarrowPhase::sweepCore(*model, frame, motion, *obstacle.model, obstacle.frame, hit) && !hit.startedInside() && hit.fraction() < earliest )
				{
					earliest = hit.fraction();
					normal = hit.normal();
					point = hit.point();
					restitution = std::max(material.bounciness(), bodyEntities[obstacle.index]->bodyPhysicalProperties().bounciness());
				}
			}

			if ( earliest >= 1.0F )
			{
				continue;
			}

			/* A sweep only prevents a CROSSING: if the body's centre ends this step on the near side of the surface it
			 * met, the contacts handle it. Without this rule a walker sliding along a wall a few millimetres off was
			 * stopped every step by a tangential hit that bounced nothing, while its own controller kept accelerating
			 * it — frozen at 369 m/s. */
			if ( Vector< 3, float >::dotProduct(body.position - point, normal) >= 0.0F )
			{
				continue;
			}

			/* Back to just before the contact (a slop short of it, never behind the start). */
			const auto allowed = std::max(0.0F, (earliest * distance) - ContinuousSlop) / distance;
			const auto kept = motion * allowed;

			body.position += kept - motion;
			body.deltaPosition = kept;

			/* The impact, as a contact would give it: the approaching velocity bounces off the surface with the pair's
			 * restitution (the maximum, as the solver combines it). The next step's contact then sees a body leaving and
			 * adds nothing; a body the sweep stopped can no longer keep a velocity into the obstacle. */
			const auto approach = Vector< 3, float >::dotProduct(body.linearVelocity, normal);

			if ( approach < 0.0F )
			{
				body.linearVelocity -= normal * ((1.0F + restitution) * approach);
			}
		}

		/* ============================================================
		 * 5. WRITE BACK the dynamic bodies.
		 * ============================================================ */
		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			auto & body = bodies[index];

			if ( body.movable == nullptr || !body.dynamic )
			{
				continue;
			}

			body.movable->setLinearVelocity(body.linearVelocity);

			if ( body.movable->isRotationPhysicsEnabled() )
			{
				body.movable->setAngularVelocity(body.angularVelocity);
			}

			bool moved = false;

			/* The solver moved the centre of mass; the node turns about its origin (rotateFromPhysics() below), so
			 * the origin also takes the swing of the offset: Δorigin = Δcom + c − ΔR · c. */
			const auto originDelta = body.deltaPosition + body.centerOffset - (body.deltaRotation * body.centerOffset);

			if ( originDelta.lengthSquared() > 0.0F )
			{
				body.movable->moveFromPhysics(originDelta);
				moved = true;
			}

			float angle = 0.0F;
			Vector< 3, float > axis;

			body.deltaRotation.toAngleAxis(angle, axis);

			if ( std::abs(angle) > std::numeric_limits< float >::epsilon() )
			{
				body.movable->rotateFromPhysics(angle, axis);
				moved = true;
			}

			if ( moved )
			{
				movedEntities.push_back(bodyEntities[index]);
			}
		}

		/* ============================================================
		 * 6. EVENTS and GROUNDED STATE, from the manifolds.
		 * ============================================================ */
		const auto down = m_environmentPhysicalProperties.surfaceGravity().normalized();

		for ( const auto & manifold : manifolds )
		{
			bool pushing = false;
			float approach = 0.0F;

			for ( const auto & point : manifold.points )
			{
				pushing = pushing || point.maxNormalImpulse > 0.0F || point.normalImpulse > 0.0F;
				approach = std::max(approach, -point.relativeVelocity);
			}

			if ( !pushing )
			{
				continue;
			}

			const bool impact = approach > ImpactSpeedThreshold;

			const float alignment = Vector< 3, float >::dotProduct(manifold.normal, down);
			auto & bodyA = bodies[manifold.bodyA];
			auto & bodyB = bodies[manifold.bodyB];

			/* The impact as the former pipeline reported it: mass × approach speed / step (NodeCollision). COLLECTED,
			 * not emitted: this step holds the physics octree lock and a handler may create or remove entities. */
			if ( bodyA.movable != nullptr && bodyA.dynamic && impact )
			{
				impacts.push_back({bodyEntities[manifold.bodyA], approach * bodyA.movable->getBodyPhysicalProperties().mass() / PhysicsStepSeconds});
			}

			if ( bodyB.movable != nullptr && bodyB.dynamic && impact )
			{
				impacts.push_back({bodyEntities[manifold.bodyB], approach * bodyB.movable->getBodyPhysicalProperties().mass() / PhysicsStepSeconds});
			}

			/* A stands on B when the normal (A → B) points down; B on A when it points up. */
			if ( bodyA.movable != nullptr && bodyA.dynamic && alignment > SupportCosine )
			{
				if ( manifold.keyB == GroundKey )
				{
					bodyA.movable->setGrounded(GroundedSource::Ground);
				}
				else
				{
					bodyA.movable->setGrounded(GroundedSource::Entity, bodyB.movable);
				}
			}

			if ( bodyB.movable != nullptr && bodyB.dynamic && alignment < -SupportCosine )
			{
				bodyB.movable->setGrounded(GroundedSource::Entity, bodyA.movable);
			}
		}

		/* ============================================================
		 * 7. THE WORLD BOUNDARIES: hard clip + bounce, after the solver (owner, P2).
		 * ============================================================ */
		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			auto & body = bodies[index];

			if ( body.movable == nullptr || !body.dynamic )
			{
				continue;
			}

			const auto & entity = bodyEntities[index];
			Vector< 3, float > positionCorrection{0.0F, 0.0F, 0.0F};
			Vector< 3, float > dominantNormal{0.0F, 0.0F, 0.0F};
			float maxPenetration = 0.0F;

			this->accumulateBoundaryCorrection(entity, positionCorrection, dominantNormal, maxPenetration);

			if ( maxPenetration <= 0.0F )
			{
				continue;
			}

			/* Only the world box's FLOOR grounds (its normal points down, from the body into the wall). */
			const auto source = dominantNormal[Y] < -SupportCosine ? GroundedSource::Boundary : GroundedSource::None;

			body.movable->moveFromPhysics(positionCorrection);
			applyCollisionResponse(body.movable, dominantNormal, 0.0F, source, nullptr);

			if ( std::ranges::find(movedEntities, entity) == movedEntities.end() )
			{
				movedEntities.push_back(entity);
			}
		}

		/* ============================================================
		 * 8. ISLANDS AND SLEEP (P5, decision 13): the awake dynamic bodies linked by this step's contacts, a union-find
		 * rebuilt every step (Jolt's IslandBuilder, Bullet's btSimulationIslandManager — no code taken). An island whose
		 * bodies have ALL been slow for SleepSteps sleeps as a whole, on any support, keyed by its lowest creation number
		 * (deterministic). The static world and the kinematic bodies do not link islands.
		 * ============================================================ */
		{
			const auto isAwakeDynamic = [&bodies] (uint32_t index) {
				return index != 0 && bodies[index].movable != nullptr && bodies[index].dynamic;
			};

			auto & parents = m_physicsIslandParents;

			parents.resize(bodies.size());
			std::iota(parents.begin(), parents.end(), 0U);

			const auto findRoot = [&parents] (uint32_t index) {
				while ( parents[index] != index )
				{
					parents[index] = parents[parents[index]];
					index = parents[index];
				}

				return index;
			};

			for ( const auto & manifold : manifolds )
			{
				if ( !isAwakeDynamic(manifold.bodyA) || !isAwakeDynamic(manifold.bodyB) )
				{
					continue;
				}

				const auto rootA = findRoot(manifold.bodyA);
				const auto rootB = findRoot(manifold.bodyB);

				if ( rootA != rootB )
				{
					parents[std::max(rootA, rootB)] = std::min(rootA, rootB);
				}
			}

			auto & islandSlow = m_physicsIslandSlow;
			auto & islandKeys = m_physicsIslandKeys;

			islandSlow.assign(bodies.size(), 1);
			islandKeys.assign(bodies.size(), std::numeric_limits< uint64_t >::max());

			for ( uint32_t index = 1; index < bodies.size(); ++index )
			{
				if ( !isAwakeDynamic(index) )
				{
					continue;
				}

				auto * movable = bodies[index].movable;
				const auto root = findRoot(index);

				movable->accountSlowness(movable->linearVelocity().lengthSquared() < SleepLinearSpeed * SleepLinearSpeed && movable->angularVelocity().lengthSquared() < SleepAngularSpeed * SleepAngularSpeed);

				if ( movable->slowSteps() < SleepSteps )
				{
					islandSlow[root] = 0;
				}

				islandKeys[root] = std::min(islandKeys[root], bodyEntities[index]->creationNumber());
			}

			for ( uint32_t index = 1; index < bodies.size(); ++index )
			{
				if ( !isAwakeDynamic(index) )
				{
					continue;
				}

				const auto root = findRoot(index);

				if ( islandSlow[root] == 0 )
				{
					continue;
				}

				bodies[index].movable->stopMovement();
				bodies[index].movable->setSleepIsland(islandKeys[root]);
				bodyEntities[index]->pauseSimulation(true);
			}
		}
	}

	void
	Scene::clipInsideBoundaries (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		const auto position = entity->getWorldCoordinates().position();

		/* No collision model means Point behavior. */
		if ( !entity->hasCollisionModel() )
		{
			if ( position[X] > m_boundary )
			{
				entity->setXPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[X] < -m_boundary )
			{
				entity->setXPosition(-m_boundary, TransformSpace::World);
			}

			if ( position[Y] > m_boundary )
			{
				entity->setYPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[Y] < -m_boundary )
			{
				entity->setYPosition(-m_boundary, TransformSpace::World);
			}

			if ( position[Z] > m_boundary )
			{
				entity->setZPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[Z] < -m_boundary )
			{
				entity->setZPosition(-m_boundary, TransformSpace::World);
			}

			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				if ( position[X] > m_boundary )
				{
					entity->setXPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[X] < -m_boundary )
				{
					entity->setXPosition(-m_boundary, TransformSpace::World);
				}

				if ( position[Y] > m_boundary )
				{
					entity->setYPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[Y] < -m_boundary )
				{
					entity->setYPosition(-m_boundary, TransformSpace::World);
				}

				if ( position[Z] > m_boundary )
				{
					entity->setZPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[Z] < -m_boundary )
				{
					entity->setZPosition(-m_boundary, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto limit = m_boundary - radius;

				if ( position[X] > limit )
				{
					entity->setXPosition(limit, TransformSpace::World);
				}
				else if ( position[X] < -limit )
				{
					entity->setXPosition(-limit, TransformSpace::World);
				}

				if ( position[Y] > limit )
				{
					entity->setYPosition(limit, TransformSpace::World);
				}
				else if ( position[Y] < -limit )
				{
					entity->setYPosition(-limit, TransformSpace::World);
				}

				if ( position[Z] > limit )
				{
					entity->setZPosition(limit, TransformSpace::World);
				}
				else if ( position[Z] < -limit )
				{
					entity->setZPosition(-limit, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Box :
			{
				const auto aabb = model->getAABB(worldCoords);

				if ( aabb.maximum(X) > m_boundary )
				{
					entity->moveX(m_boundary - aabb.maximum(X), TransformSpace::World);
				}
				else if ( aabb.minimum(X) < -m_boundary )
				{
					entity->moveX(-m_boundary - aabb.minimum(X), TransformSpace::World);
				}

				if ( aabb.maximum(Y) > m_boundary )
				{
					entity->moveY(m_boundary - aabb.maximum(Y), TransformSpace::World);
				}
				else if ( aabb.minimum(Y) < -m_boundary )
				{
					entity->moveY(-m_boundary - aabb.minimum(Y), TransformSpace::World);
				}

				if ( aabb.maximum(Z) > m_boundary )
				{
					entity->moveZ(m_boundary - aabb.maximum(Z), TransformSpace::World);
				}
				else if ( aabb.minimum(Z) < -m_boundary )
				{
					entity->moveZ(-m_boundary - aabb.minimum(Z), TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule boundary clipping. */
				break;
		}
	}

	void
	Scene::accumulateBoundaryCorrection (const std::shared_ptr< AbstractEntity > & entity, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration) const noexcept
	{
		/* No collision model means no boundary correction. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();
		const auto position = worldCoords.position();

		/* Helper lambda to accumulate a single boundary collision. */
		auto accumulateCollision = [&positionCorrection, &dominantNormal, &maxPenetration] (const Vector< 3, float > & normal, float penetration) {
			/* Accumulate position correction (move opposite to normal). */
			positionCorrection -= normal * penetration;

			/* Track dominant collision for velocity bounce. */
			if ( penetration > maxPenetration )
			{
				maxPenetration = penetration;
				dominantNormal = normal;
			}
		};

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				if ( position[X] > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, position[X] - m_boundary);
				}
				else if ( position[X] < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - position[X]);
				}

				if ( position[Y] > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, position[Y] - m_boundary);
				}
				else if ( position[Y] < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - position[Y]);
				}

				if ( position[Z] > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, position[Z] - m_boundary);
				}
				else if ( position[Z] < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - position[Z]);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;

				if ( position[X] + radius > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, (position[X] + radius) - m_boundary);
				}
				else if ( position[X] - radius < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - (position[X] - radius));
				}

				if ( position[Y] + radius > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, (position[Y] + radius) - m_boundary);
				}
				else if ( position[Y] - radius < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - (position[Y] - radius));
				}

				if ( position[Z] + radius > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, (position[Z] + radius) - m_boundary);
				}
				else if ( position[Z] - radius < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - (position[Z] - radius));
				}
			}
				break;

			case CollisionModelType::Box :
			{
				const auto aabb = model->getAABB(worldCoords);

				if ( aabb.maximum(X) > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, aabb.maximum(X) - m_boundary);
				}
				else if ( aabb.minimum(X) < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - aabb.minimum(X));
				}

				if ( aabb.maximum(Y) > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, aabb.maximum(Y) - m_boundary);
				}
				else if ( aabb.minimum(Y) < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - aabb.minimum(Y));
				}

				if ( aabb.maximum(Z) > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, aabb.maximum(Z) - m_boundary);
				}
				else if ( aabb.minimum(Z) < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - aabb.minimum(Z));
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule boundary correction. */
				break;
		}
	}

}
