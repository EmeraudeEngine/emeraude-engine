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
			 * @brief Generates the contacts of a collision model (A) and a GROUND triangle (B): one-sided, and solid below.
			 * @note A height-field ground is the top of a solid, not a thin shell (owner, 2026-10-02; the approach of Jolt's
			 * height fields and Box2D v3's one-sided chains). (1) A's centre OVER the triangle's plane: the contact
			 * generators, a manifold whose normal would push A down dropped. (2) A's centre UNDER the plane (it crossed the
			 * surface within one step — a ball at 43 m/s moves 0.72 m per step): the low points of A (the bottom of a
			 * sphere, of each capsule end, the four lowest box corners) that project inside the triangle are pushed back up
			 * along the face normal by their depth. Before, case (2) got a normal pointing DOWN and the body went through.
			 * @param modelA A reference to model A.
			 * @param frameA A reference to A's world frame.
			 * @param triangle A reference to the world ground triangle (its up side is the side of +Y).
			 * @param margin The speculative margin added to A (m), >= 0.
			 * @param claimedLowPoints A writable bit set of A's low points already given to a triangle (bit = the point's
			 * stable index), zero for each body: a point on an edge shared by two triangles belongs to the first only.
			 * @param manifold A reference to the manifold written (cleared first).
			 * @return bool True when the inflated shape touches the triangle, or is under it.
			 */
			[[nodiscard]]
			static bool generateGround (const CollisionModelInterface & modelA, const Base::Math::CartesianFrame< float > & frameA, const Base::Math::Space3D::Triangle< float > & triangle, float margin, uint32_t & claimedLowPoints, Base::Math::Space3D::ContactManifold< float > & manifold) noexcept;
	};
}
