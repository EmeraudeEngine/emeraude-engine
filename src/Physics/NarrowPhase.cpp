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
#include <cmath>
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

		/**
		 * @brief Whether a point lies VERTICALLY over or under a triangle (its X/Z footprint, edges included, either
		 * winding): a height field gives every point of the plane to the triangle above or under it.
		 */
		[[nodiscard]]
		bool
		insideFootprint (const Vector< 3, float > & point, const Triangle< float > & triangle) noexcept
		{
			const auto side = [&point] (const Vector< 3, float > & from, const Vector< 3, float > & to) {
				return ((to[X] - from[X]) * (point[Z] - from[Z])) - ((to[Z] - from[Z]) * (point[X] - from[X]));
			};

			const auto sideAB = side(triangle.pointA(), triangle.pointB());
			const auto sideBC = side(triangle.pointB(), triangle.pointC());
			const auto sideCA = side(triangle.pointC(), triangle.pointA());

			return (sideAB >= 0.0F && sideBC >= 0.0F && sideCA >= 0.0F) || (sideAB <= 0.0F && sideBC <= 0.0F && sideCA <= 0.0F);
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
	NarrowPhase::heightOverGround (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Triangle< float > & triangle, float & height, Vector< 3, float > & normal) noexcept
	{
		const auto shape = toWorldShape(modelA, frameA, 0.0F);

		if ( !shape.valid )
		{
			return false;
		}

		const auto centre = centerOf(shape);

		if ( !insideFootprint(centre, triangle) || !TriangleDetail::unitNormal(triangle, normal) || normal[Y] == 0.0F )
		{
			return false;
		}

		/* A height-field triangle faces +Y whatever its winding. */
		if ( normal[Y] < 0.0F )
		{
			normal = -normal;
		}

		/* The surface height under the centre, on the triangle's plane. */
		const auto & corner = triangle.pointA();
		const auto surface = corner[Y] - (((normal[X] * (centre[X] - corner[X])) + (normal[Z] * (centre[Z] - corner[Z]))) / normal[Y]);

		height = centre[Y] - surface;

		return true;
	}

	bool
	NarrowPhase::generateGround (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Triangle< float > & triangle, float margin, ContactManifold< float > & manifold) noexcept
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

		if ( !shape.valid || !contactsOfTriangle(shape, triangle, manifold) )
		{
			return false;
		}

		/* One-sided: the ground may only push a body up (the normal A → B points down). */
		if ( Vector< 3, float >::dotProduct(manifold.normal(), upward) > 0.0F )
		{
			manifold.clear();

			return false;
		}

		return true;
	}

	namespace
	{
		/** @brief The round core of a world shape: a sphere (a box's inscribed one, a point's of radius 0) or a capsule. */
		struct SweptCore final
		{
			Sphere< float > sphere;
			Capsule< float > capsule;
			bool isCapsule{false};
			bool valid{false};
		};

		[[nodiscard]]
		SweptCore
		sweptCoreOf (const CollisionModelInterface & model, const CartesianFrame< float > & frame) noexcept
		{
			SweptCore core;
			const auto shape = toWorldShape(model, frame, 0.0F);

			if ( !shape.valid )
			{
				return core;
			}

			switch ( shape.kind )
			{
				case WorldShape::Kind::Sphere :
					core.sphere = shape.sphere;
					core.valid = true;
					break;

				case WorldShape::Kind::Box :
				{
					const auto & half = shape.box.halfExtents();

					core.sphere = Sphere< float >{std::min({half[X], half[Y], half[Z]}), shape.box.center()};
					core.valid = true;
				}
					break;

				case WorldShape::Kind::Capsule :
					core.capsule = shape.capsule;
					core.isCapsule = true;
					core.valid = core.capsule.isValid();
					break;
			}

			return core;
		}

		/** @brief Sweeps a core against any base target (box, sphere, capsule, triangle). */
		template< typename target_t >
		[[nodiscard]]
		bool
		sweepCoreAgainst (const SweptCore & core, const Vector< 3, float > & motion, const target_t & target, CastHit< float > & hit) noexcept
		{
			if ( core.isCapsule )
			{
				return castCapsule(core.capsule, motion, target, hit);
			}

			/* A point is swept as a ray (a sphere of radius 0 is not a valid sphere). */
			if ( !(core.sphere.radius() > 0.0F) )
			{
				return castRay(core.sphere.position(), motion, target, hit);
			}

			return castSphere(core.sphere, motion, target, hit);
		}
	}

	float
	NarrowPhase::coreRadius (const CollisionModelInterface & model, const CartesianFrame< float > & frame) noexcept
	{
		const auto core = sweptCoreOf(model, frame);

		if ( !core.valid )
		{
			return 0.0F;
		}

		return core.isCapsule ? core.capsule.radius() : core.sphere.radius();
	}

	bool
	NarrowPhase::sweepCore (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Vector< 3, float > & motion, const CollisionModelInterface & modelB, const CartesianFrame< float > & frameB, CastHit< float > & hit) noexcept
	{
		const auto core = sweptCoreOf(modelA, frameA);
		const auto target = toWorldShape(modelB, frameB, 0.0F);

		if ( !core.valid || !target.valid )
		{
			return false;
		}

		switch ( target.kind )
		{
			case WorldShape::Kind::Sphere :
				/* A point obstacle (a zero-radius sphere) cannot stop anything. */
				return target.sphere.radius() > 0.0F && sweepCoreAgainst(core, motion, target.sphere, hit);

			case WorldShape::Kind::Box :
				return sweepCoreAgainst(core, motion, target.box, hit);

			case WorldShape::Kind::Capsule :
				return sweepCoreAgainst(core, motion, target.capsule, hit);
		}

		return false;
	}

	bool
	NarrowPhase::sweepCore (const CollisionModelInterface & modelA, const CartesianFrame< float > & frameA, const Vector< 3, float > & motion, const Triangle< float > & triangle, CastHit< float > & hit) noexcept
	{
		const auto core = sweptCoreOf(modelA, frameA);

		if ( !core.valid )
		{
			return false;
		}

		return sweepCoreAgainst(core, motion, triangle, hit);
	}
}
