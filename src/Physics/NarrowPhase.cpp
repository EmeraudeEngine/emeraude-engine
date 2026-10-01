/*
 * src/Physics/NarrowPhase.cpp
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

#include "NarrowPhase.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstdint>

/* Local inclusions. */
#include "Math/Space3D/Contacts/BoxBox.hpp"
#include "Math/Space3D/Contacts/BoxTriangle.hpp"
#include "Math/Space3D/Contacts/CapsuleBox.hpp"
#include "Math/Space3D/Contacts/CapsuleTriangle.hpp"
#include "Math/Space3D/Contacts/RoundShapes.hpp"
#include "Math/Space3D/Contacts/SphereBox.hpp"
#include "Math/Space3D/Contacts/SphereTriangle.hpp"
#include "Math/Space3D/OrientedBox.hpp"
#include "StaticVector.hpp"
#include "BoxCollisionModel.hpp"
#include "CapsuleCollisionModel.hpp"
#include "CollisionModelInterface.hpp"
#include "SphereCollisionModel.hpp"

namespace EmEn::Physics
{
	using namespace Base::Math;
	using namespace Base::Math::Space3D;

	namespace
	{
		/** @brief A collision model as a base primitive in world space. */
		struct WorldShape final
		{
			enum class Kind : uint8_t
			{
				Sphere,
				Box,
				Capsule
			};

			Sphere< float > sphere;
			OrientedBox< float > box;
			Capsule< float > capsule;
			Kind kind{Kind::Sphere};
			/* False for a box not usable yet (a model whose geometry has not loaded: an empty local box). */
			bool valid{true};
		};

		/**
		 * @brief Builds the world primitive of a model, inflated by a margin.
		 */
		[[nodiscard]]
		WorldShape
		toWorldShape (const CollisionModelInterface & model, const CartesianFrame< float > & frame, float margin) noexcept
		{
			WorldShape shape;

			switch ( model.modelType() )
			{
				case CollisionModelType::Point :
					shape.kind = WorldShape::Kind::Sphere;
					shape.sphere = Sphere< float >{margin, frame.position()};
					break;

				case CollisionModelType::Sphere :
				{
					const auto sphere = static_cast< const SphereCollisionModel & >(model).toWorldSphere(frame);

					shape.kind = WorldShape::Kind::Sphere;
					shape.sphere = Sphere< float >{sphere.radius() + margin, sphere.position()};
				}
					break;

				case CollisionModelType::Box :
				{
					/* P3: the box turns with its entity (before 2026-10-02, the world envelope of its 8 corners). */
					const auto & boxModel = static_cast< const BoxCollisionModel & >(model);

					shape.kind = WorldShape::Kind::Box;

					if ( !boxModel.localBox().isValid() )
					{
						shape.valid = false;

						break;
					}

					const auto box = boxModel.toWorldBox(frame);
					const auto & half = box.halfExtents();

					shape.box = OrientedBox< float >{box.center(), box.axes(), Vector< 3, float >{half[X] + margin, half[Y] + margin, half[Z] + margin}};
					shape.valid = shape.box.isValid();
				}
					break;

				case CollisionModelType::Capsule :
				{
					const auto capsule = static_cast< const CapsuleCollisionModel & >(model).toWorldCapsule(frame);

					shape.kind = WorldShape::Kind::Capsule;
					shape.capsule = Capsule< float >{capsule.startPoint(), capsule.endPoint(), capsule.radius() + margin};
				}
					break;
			}

			return shape;
		}

		/** @brief Dispatches a pair of world shapes to the base contact generators. */
		[[nodiscard]]
		bool
		contactsOf (const WorldShape & shapeA, const WorldShape & shapeB, ContactManifold< float > & manifold) noexcept
		{
			using Kind = WorldShape::Kind;

			if ( !shapeA.valid || !shapeB.valid )
			{
				manifold.clear();

				return false;
			}

			switch ( shapeA.kind )
			{
				case Kind::Sphere :
					switch ( shapeB.kind )
					{
						case Kind::Sphere : return computeContactManifold(shapeA.sphere, shapeB.sphere, manifold);
						case Kind::Box : return computeContactManifold(shapeA.sphere, shapeB.box, manifold);
						case Kind::Capsule : return computeContactManifold(shapeA.sphere, shapeB.capsule, manifold);
					}
					break;

				case Kind::Box :
					switch ( shapeB.kind )
					{
						case Kind::Sphere : return computeContactManifold(shapeA.box, shapeB.sphere, manifold);
						case Kind::Box : return computeContactManifold(shapeA.box, shapeB.box, manifold);
						case Kind::Capsule : return computeContactManifold(shapeA.box, shapeB.capsule, manifold);
					}
					break;

				case Kind::Capsule :
					switch ( shapeB.kind )
					{
						case Kind::Sphere : return computeContactManifold(shapeA.capsule, shapeB.sphere, manifold);
						case Kind::Box : return computeContactManifold(shapeA.capsule, shapeB.box, manifold);
						case Kind::Capsule : return computeContactManifold(shapeA.capsule, shapeB.capsule, manifold);
					}
					break;
			}

			manifold.clear();

			return false;
		}

		/** @brief Dispatches a world shape and a triangle to the base contact generators. */
		[[nodiscard]]
		bool
		contactsOfTriangle (const WorldShape & shape, const Triangle< float > & triangle, ContactManifold< float > & manifold) noexcept
		{
			switch ( shape.kind )
			{
				case WorldShape::Kind::Sphere :
					return computeContactManifold(shape.sphere, triangle, manifold);

				case WorldShape::Kind::Box :
					return computeContactManifold(shape.box, triangle, manifold);

				case WorldShape::Kind::Capsule :
					return computeContactManifold(shape.capsule, triangle, manifold);
			}

			manifold.clear();

			return false;
		}

		/** @brief A feature id bit for the half-space contacts of a ground triangle (never produced by the generators). */
		constexpr uint32_t GroundHalfSpaceFeature{0x80000000U};

		/** @brief The centre of a world shape. */
		[[nodiscard]]
		Vector< 3, float >
		centerOf (const WorldShape & shape) noexcept
		{
			switch ( shape.kind )
			{
				case WorldShape::Kind::Sphere :
					return shape.sphere.position();

				case WorldShape::Kind::Box :
					return shape.box.center();

				case WorldShape::Kind::Capsule :
					return (shape.capsule.startPoint() + shape.capsule.endPoint()) * 0.5F;
			}

			return {};
		}

		/** @brief A low point of a shape over a ground plane: its position, height over the plane and stable index. */
		struct LowPoint final
		{
			Vector< 3, float > position;
			float height{0.0F};
			uint32_t index{0};
		};

		/**
		 * @brief The points of a world shape lower than a reach over a plane, the four lowest at most: the bottom of a
		 * sphere, the bottom of each capsule end, the box corners. Their index is stable (the box corner index, the
		 * capsule end), so a solver can carry their impulse from one step to the next.
		 */
		void
		lowPoints (const WorldShape & shape, const Vector< 3, float > & upward, const Vector< 3, float > & planePoint, float reach, Base::StaticVector< LowPoint, 4 > & points) noexcept
		{
			points.clear();

			/* Eight box corners at most. */
			Base::StaticVector< LowPoint, 8 > candidates;

			const auto add = [&] (const Vector< 3, float > & position, uint32_t index) {
				const auto height = Vector< 3, float >::dotProduct(position - planePoint, upward);

				if ( height < reach && !candidates.full() )
				{
					candidates.push_back(LowPoint{.position = position, .height = height, .index = index});
				}
			};

			switch ( shape.kind )
			{
				case WorldShape::Kind::Sphere :
					add(shape.sphere.position() - (upward * shape.sphere.radius()), 0);
					break;

				case WorldShape::Kind::Capsule :
				{
					const auto rim = upward * shape.capsule.radius();

					add(shape.capsule.startPoint() - rim, 0);
					add(shape.capsule.endPoint() - rim, 1);
				}
					break;

				case WorldShape::Kind::Box :
					for ( uint32_t index = 0; index < 8; ++index )
					{
						add(shape.box.corner(index), index);
					}
					break;
			}

			/* The four lowest, ties broken by the index: deterministic. */
			std::ranges::sort(candidates, [] (const LowPoint & lhs, const LowPoint & rhs) {
				return lhs.height != rhs.height ? lhs.height < rhs.height : lhs.index < rhs.index;
			});

			for ( const auto & candidate : candidates )
			{
				if ( points.full() )
				{
					break;
				}

				points.push_back(candidate);
			}
		}

		/** @brief Whether a point projects inside a triangle along its unit normal (edges included). */
		[[nodiscard]]
		bool
		projectsInside (const Vector< 3, float > & point, const Triangle< float > & triangle, const Vector< 3, float > & normal) noexcept
		{
			const auto onInnerSide = [&point, &normal] (const Vector< 3, float > & from, const Vector< 3, float > & to) {
				return Vector< 3, float >::dotProduct(Vector< 3, float >::crossProduct(to - from, point - from), normal) >= 0.0F;
			};

			return
				onInnerSide(triangle.pointA(), triangle.pointB()) &&
				onInnerSide(triangle.pointB(), triangle.pointC()) &&
				onInnerSide(triangle.pointC(), triangle.pointA());
		}
	}

	bool
	NarrowPhase::generate (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const CollisionModelInterface & modelB, const CartesianFrame< float > & frameB, float margin, ContactManifold< float > & manifold) noexcept
	{
		return contactsOf(toWorldShape(modelA, frameA, margin), toWorldShape(modelB, frameB, 0.0F), manifold);
	}

	bool
	NarrowPhase::generate (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Triangle< float > & triangle, float margin, ContactManifold< float > & manifold) noexcept
	{
		const auto shape = toWorldShape(modelA, frameA, margin);

		if ( !shape.valid )
		{
			manifold.clear();

			return false;
		}

		return contactsOfTriangle(shape, triangle, manifold);
	}

	bool
	NarrowPhase::generateGround (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Triangle< float > & triangle, float margin, uint32_t & claimedLowPoints, ContactManifold< float > & manifold) noexcept
	{
		manifold.clear();

		Vector< 3, float > upward;

		if ( !TriangleDetail::unitNormal(triangle, upward) )
		{
			return false;
		}

		/* A height-field triangle faces +Y whatever its winding. */
		if ( upward[Y] < 0.0F )
		{
			upward = -upward;
		}

		const auto shape = toWorldShape(modelA, frameA, margin);

		if ( !shape.valid )
		{
			return false;
		}

		const auto centreHeight = Vector< 3, float >::dotProduct(centerOf(shape) - triangle.pointA(), upward);

		/* 1. The centre is OVER the plane (every body that did not cross the surface): the contact generators, one-sided —
		 * the ground may only push a body up (the normal A → B points down). */
		if ( centreHeight >= 0.0F )
		{
			if ( !contactsOfTriangle(shape, triangle, manifold) )
			{
				return false;
			}

			if ( Vector< 3, float >::dotProduct(manifold.normal(), upward) > 0.0F )
			{
				manifold.clear();

				return false;
			}

			return true;
		}

		/* 2. The centre is UNDER the plane (the body crossed the surface within one step): the ground is solid below, the
		 * low points of A that project inside this triangle go back up along the face normal. */
		Base::StaticVector< LowPoint, 4 > low;
		lowPoints(shape, upward, triangle.pointA(), 0.0F, low);

		manifold.setNormal(-upward);

		for ( const auto & point : low )
		{
			const auto bit = 1U << point.index;

			if ( (claimedLowPoints & bit) != 0U || !projectsInside(point.position, triangle, upward) )
			{
				continue;
			}

			claimedLowPoints |= bit;

			/* The (inflated) point is under the plane by -height: halfway to it, as the generators place a point. */
			static_cast< void >(manifold.addPoint(ContactPoint< float >{point.position - (upward * (point.height * 0.5F)), -point.height, GroundHalfSpaceFeature | point.index}));
		}

		return !manifold.empty();
	}
}
