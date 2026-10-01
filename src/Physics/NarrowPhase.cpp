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

/* Local inclusions. */
#include "Math/Space3D/Contacts/BoxBox.hpp"
#include "Math/Space3D/Contacts/BoxTriangle.hpp"
#include "Math/Space3D/Contacts/CapsuleBox.hpp"
#include "Math/Space3D/Contacts/CapsuleTriangle.hpp"
#include "Math/Space3D/Contacts/RoundShapes.hpp"
#include "Math/Space3D/Contacts/SphereBox.hpp"
#include "Math/Space3D/Contacts/SphereTriangle.hpp"
#include "Math/Space3D/OrientedBox.hpp"
#include "AABBCollisionModel.hpp"
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

				case CollisionModelType::AABB :
				{
					/* P2: the world envelope, axis-aligned (oriented boxes are P3). */
					const auto aabb = static_cast< const AABBCollisionModel & >(model).toWorldAABB(frame);
					const Vector< 3, float > half{(aabb.width() * 0.5F) + margin, (aabb.height() * 0.5F) + margin, (aabb.depth() * 0.5F) + margin};

					shape.kind = WorldShape::Kind::Box;
					shape.box = OrientedBox< float >{aabb.centroid(), {Vector< 3, float >{1.0F, 0.0F, 0.0F}, Vector< 3, float >{0.0F, 1.0F, 0.0F}, Vector< 3, float >{0.0F, 0.0F, 1.0F}}, half};
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
}
