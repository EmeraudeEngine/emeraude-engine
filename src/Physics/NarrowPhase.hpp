/*
 * src/Physics/NarrowPhase.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"
#include "Math/Space3D/Casts/ShapeCast.hpp"
#include "Math/Space3D/Contacts/ContactManifold.hpp"
#include "Math/Space3D/Triangle.hpp"

namespace EmEn::Physics
{
	class CollisionModelInterface;

	/**
	 * @brief The narrow phase of the physics step (physics overhaul P2): a collision model in its world frame becomes a
	 * base primitive, and a pair of them a base contact manifold (`Base::Math::Space3D::computeContactManifold()`).
	 * @note The model mapping of P2: an AABB model becomes an AXIS-ALIGNED `OrientedBox` built from its world AABB (a
	 * rotated entity collides as its world envelope until P3's oriented boxes), a sphere a `Sphere`, a capsule a
	 * `Capsule`, a point a zero-radius sphere.
	 * @note SPECULATIVE MARGIN: shape A is inflated by `margin` (radius or half extents), so a pair within `margin` of
	 * touching still yields points; their depth then INCLUDES the margin — the separation is `margin − depth`.
	 * @note The manifold's normal points from A to B.
	 */
	class EMEN_API NarrowPhase final
	{
		public:

			/**
			 * @brief Generates the contacts of two collision models.
			 * @param modelA A reference to model A.
			 * @param frameA A reference to A's world frame.
			 * @param modelB A reference to model B.
			 * @param frameB A reference to B's world frame.
			 * @param margin The speculative margin added to A (m), >= 0.
			 * @param manifold A reference to the manifold written (cleared first).
			 * @return bool True when the inflated shapes overlap.
			 */
			[[nodiscard]]
			static bool generate (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const CollisionModelInterface & modelB, const Base::Math::CartesianFrame< float > & frameB, float margin, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Generates the contacts of a collision model (A) and a triangle (B), e.g. a ground triangle.
			 * @param modelA A reference to model A.
			 * @param frameA A reference to A's world frame.
			 * @param triangle A reference to the world triangle (two-sided).
			 * @param margin The speculative margin added to A (m), >= 0.
			 * @param manifold A reference to the manifold written (cleared first).
			 * @return bool True when the inflated shape overlaps the triangle.
			 */
			[[nodiscard]]
			static bool generate (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Space3D::Triangle< float > & triangle, float margin, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Answers the height of A's centre over a ground triangle, when the centre lies VERTICALLY over or under it.
			 * @note What tells a body that crossed the ground within one step (a negative height on the triangle vertically
			 * under its centre): `Scene::resolveCollisions()` puts it back on the surface before its contacts.
			 * @param modelA A reference to model A.
			 * @param frameA A reference to A's world frame.
			 * @param triangle A reference to the world ground triangle.
			 * @param height A reference to the height written (negative = under the surface).
			 * @param normal A reference to the triangle's unit normal written (its up side, +Y).
			 * @return bool False when the centre is not over or under this triangle (or the triangle is vertical).
			 */
			[[nodiscard]]
			static bool heightOverGround (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Space3D::Triangle< float > & triangle, float & height, Base::Math::Vector< 3, float > & normal) noexcept;

			/**
			 * @brief Generates the contacts of a collision model (A) and a GROUND triangle (B): one-sided.
			 * @note A height-field ground is the top of a solid, not a thin shell (owner, 2026-10-02): a manifold whose normal
			 * would push A DOWN is dropped (a body whose centre crossed a triangle got a normal pointing down and was pushed
			 * through). A body under the ground is put back on it by the scene before its contacts (heightOverGround()).
			 * @param modelA A reference to model A.
			 * @param frameA A reference to A's world frame.
			 * @param triangle A reference to the world ground triangle (its up side is the side of +Y).
			 * @param margin The speculative margin added to A (m), >= 0.
			 * @param manifold A reference to the manifold written (cleared first).
			 * @return bool True when the inflated shape touches the triangle.
			 */
			[[nodiscard]]
			static bool generateGround (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Space3D::Triangle< float > & triangle, float margin, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Returns the radius of the round CORE a fast body is swept with (continuous collision): a sphere's radius,
			 * a capsule's radius, a box's smallest half extent (its inscribed sphere, Bullet's "ccdSweptSphereRadius"), 0 for
			 * a point (a ray).
			 * @param model A reference to the collision model.
			 * @param frame A reference to its world frame.
			 * @return float 0 for an unusable shape too.
			 */
			[[nodiscard]]
			static float coreRadius (const CollisionModelInterface & model, const Base::Math::CartesianFrame< float > & frame) noexcept;

			/**
			 * @brief Sweeps the round core of A (coreRadius()) along a motion against a collision model B.
			 * @note The core of a box is its inscribed sphere: conservative, a box never passes a solid, it stops a little
			 * later than its true faces would.
			 * @param modelA A reference to the moving model.
			 * @param frameA A reference to A's world frame at the motion's start.
			 * @param motion A reference to the motion.
			 * @param modelB A reference to the obstacle.
			 * @param frameB A reference to B's world frame.
			 * @param hit A reference to the first contact, written on a contact.
			 * @return bool True on a contact before the end of the motion.
			 */
			[[nodiscard]]
			static bool sweepCore (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Vector< 3, float > & motion, const CollisionModelInterface & modelB, const Base::Math::CartesianFrame< float > & frameB, Base::Math::Space3D::CastHit< float > & hit) noexcept;

			/**
			 * @brief Sweeps the round core of A (coreRadius()) along a motion against a triangle (a ground triangle).
			 * @param modelA A reference to the moving model.
			 * @param frameA A reference to A's world frame at the motion's start.
			 * @param motion A reference to the motion.
			 * @param triangle A reference to the world triangle.
			 * @param hit A reference to the first contact, written on a contact.
			 * @return bool True on a contact before the end of the motion.
			 */
			[[nodiscard]]
			static bool sweepCore (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Vector< 3, float > & motion, const Base::Math::Space3D::Triangle< float > & triangle, Base::Math::Space3D::CastHit< float > & hit) noexcept;

			/**
			 * @brief Sweeps a world capsule (a kinematic character) along a motion against a collision model.
			 * @param capsule A reference to the world capsule at its start. @pre capsule.isValid().
			 * @param motion A reference to the motion.
			 * @param model A reference to the obstacle.
			 * @param frame A reference to the obstacle's world frame.
			 * @param hit A reference to the first contact, written on a contact.
			 * @return bool True on a contact before the end of the motion.
			 */
			[[nodiscard]]
			static bool sweepCapsule (const Base::Math::Space3D::Capsule< float > & capsule, const Base::Math::Vector< 3, float > & motion, const CollisionModelInterface & model, const Base::Math::CartesianFrame< float > & frame, Base::Math::Space3D::CastHit< float > & hit) noexcept;

			/**
			 * @brief Sweeps a world capsule along a motion against a triangle.
			 * @param capsule A reference to the world capsule at its start. @pre capsule.isValid().
			 * @param motion A reference to the motion.
			 * @param triangle A reference to the world triangle.
			 * @param hit A reference to the first contact, written on a contact.
			 * @return bool True on a contact before the end of the motion.
			 */
			[[nodiscard]]
			static bool sweepCapsule (const Base::Math::Space3D::Capsule< float > & capsule, const Base::Math::Vector< 3, float > & motion, const Base::Math::Space3D::Triangle< float > & triangle, Base::Math::Space3D::CastHit< float > & hit) noexcept;

			/**
			 * @brief Generates the contacts of a world capsule (A) and a collision model (B).
			 * @param capsule A reference to the world capsule.
			 * @param model A reference to model B.
			 * @param frame A reference to B's world frame.
			 * @param manifold A reference to the manifold written (cleared first), normal from A to B.
			 * @return bool True when they overlap.
			 */
			[[nodiscard]]
			static bool capsuleContacts (const Base::Math::Space3D::Capsule< float > & capsule, const CollisionModelInterface & model, const Base::Math::CartesianFrame< float > & frame, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Generates the contacts of a world capsule (A) and a ground triangle (B), one-sided as generateGround().
			 * @param capsule A reference to the world capsule.
			 * @param triangle A reference to the world ground triangle.
			 * @param manifold A reference to the manifold written (cleared first), normal from A to B.
			 * @return bool True when they overlap and the contact pushes the capsule up.
			 */
			[[nodiscard]]
			static bool capsuleGroundContacts (const Base::Math::Space3D::Capsule< float > & capsule, const Base::Math::Space3D::Triangle< float > & triangle, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Generates the contacts of a body (A) and ONE world triangle of a triangle-mesh model (B), P5.
			 * @note One-sided unless two-sided (the front face only pushes a body out of its front); a contact on an
			 * inactive (flat or concave) edge or vertex takes the face normal, so a body crossing two coplanar triangles
			 * does not bump on their shared edge.
			 * @param model A reference to the body's collision model.
			 * @param frame A reference to the body's world frame.
			 * @param worldTriangle A reference to the world triangle (TriangleMeshCollisionModel::forEachWorldTriangle()).
			 * @param faceNormal Its unit world front normal.
			 * @param activeEdges Its active edge flags.
			 * @param twoSided Whether its back face collides too.
			 * @param margin The speculative margin (m).
			 * @param manifold A reference to the manifold written, normal from A to B.
			 * @return bool
			 */
			[[nodiscard]]
			static bool generateMeshTriangle (const CollisionModelInterface & model, const Base::Math::CartesianFrame< float > & frame, const Base::Math::Space3D::Triangle< float > & worldTriangle, const Base::Math::Vector< 3, float > & faceNormal, uint8_t activeEdges, bool twoSided, float margin, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Generates the contacts of a world capsule (A) and one world triangle of a mesh (B), as
			 * generateMeshTriangle() (the character's depenetration).
			 * @param capsule A reference to the world capsule.
			 * @param worldTriangle A reference to the world triangle.
			 * @param faceNormal Its unit world front normal.
			 * @param activeEdges Its active edge flags.
			 * @param twoSided Whether its back face collides too.
			 * @param manifold A reference to the manifold written (cleared first), normal from A to B.
			 * @return bool
			 */
			[[nodiscard]]
			static bool capsuleMeshTriangleContacts (const Base::Math::Space3D::Capsule< float > & capsule, const Base::Math::Space3D::Triangle< float > & worldTriangle, const Base::Math::Vector< 3, float > & faceNormal, uint8_t activeEdges, bool twoSided, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;

			/**
			 * @brief Filters a sweep's hit on a mesh triangle: one-sided unless two-sided, its normal corrected on an
			 * inactive edge or vertex (the face normal).
			 * @param worldTriangle A reference to the world triangle hit.
			 * @param faceNormal Its unit world front normal.
			 * @param activeEdges Its active edge flags.
			 * @param twoSided Whether its back face collides too.
			 * @param hit A reference to the hit (its normal may be replaced).
			 * @return bool False when the hit is on the back of a one-sided triangle (to ignore).
			 */
			[[nodiscard]]
			static bool acceptMeshHit (const Base::Math::Space3D::Triangle< float > & worldTriangle, const Base::Math::Vector< 3, float > & faceNormal, uint8_t activeEdges, bool twoSided, Base::Math::Space3D::CastHit< float > & hit) noexcept;
	};
}
