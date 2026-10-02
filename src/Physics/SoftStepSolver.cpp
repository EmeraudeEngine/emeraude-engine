/*
 * src/Physics/SoftStepSolver.cpp
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

#include "SoftStepSolver.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Math/RigidBody.hpp"

namespace EmEn::Physics
{
	using namespace Base::Math;

	namespace
	{
		using Vec3 = Vector< 3, float >;

		/** @brief The effective mass along a direction at a point: 1 / (mA + mB + (rA × d)·IA(rA × d) + (rB × d)·IB(rB × d)). */
		[[nodiscard]]
		float
		effectiveMass (const SoftStepSolver::Body & bodyA, const SoftStepSolver::Body & bodyB, const Vec3 & anchorA, const Vec3 & anchorB, const Vec3 & direction) noexcept
		{
			const Vec3 crossA = Vec3::crossProduct(anchorA, direction);
			const Vec3 crossB = Vec3::crossProduct(anchorB, direction);
			const float inverse = bodyA.inverseMass + bodyB.inverseMass + Vec3::dotProduct(crossA, bodyA.inverseInertia * crossA) + Vec3::dotProduct(crossB, bodyB.inverseInertia * crossB);

			return inverse > 0.0F ? 1.0F / inverse : 0.0F;
		}

		/** @brief The velocity of B relative to A at the anchors. */
		[[nodiscard]]
		Vec3
		relativeVelocityAt (const SoftStepSolver::Body & bodyA, const SoftStepSolver::Body & bodyB, const Vec3 & anchorA, const Vec3 & anchorB) noexcept
		{
			const Vec3 velocityA = bodyA.linearVelocity + Vec3::crossProduct(bodyA.angularVelocity, anchorA);
			const Vec3 velocityB = bodyB.linearVelocity + Vec3::crossProduct(bodyB.angularVelocity, anchorB);

			return velocityB - velocityA;
		}

		/** @brief Applies an impulse P at the anchors: −P to A, +P to B (only a dynamic body changes). */
		void
		applyImpulse (SoftStepSolver::Body & bodyA, SoftStepSolver::Body & bodyB, const Vec3 & anchorA, const Vec3 & anchorB, const Vec3 & impulse) noexcept
		{
			if ( bodyA.dynamic )
			{
				bodyA.linearVelocity -= impulse * bodyA.inverseMass;
				bodyA.angularVelocity -= bodyA.inverseInertia * Vec3::crossProduct(anchorA, impulse);
			}

			if ( bodyB.dynamic )
			{
				bodyB.linearVelocity += impulse * bodyB.inverseMass;
				bodyB.angularVelocity += bodyB.inverseInertia * Vec3::crossProduct(anchorB, impulse);
			}
		}

		/** @brief Two unit tangents completing a unit normal (a stable, deterministic basis). */
		void
		tangentBasis (const Vec3 & normal, Vec3 & tangent1, Vec3 & tangent2) noexcept
		{
			/* A world axis little aligned with the normal seeds the basis (one component is always under 1/√3). */
			Vec3 seed{0.0F, 0.0F, 1.0F};

			if ( std::abs(normal[X]) < std::numbers::inv_sqrt3_v< float > )
			{
				seed = Vec3{1.0F, 0.0F, 0.0F};
			}
			else if ( std::abs(normal[Y]) < std::numbers::inv_sqrt3_v< float > )
			{
				seed = Vec3{0.0F, 1.0F, 0.0F};
			}

			tangent1 = Vec3::crossProduct(normal, seed).normalized();
			tangent2 = Vec3::crossProduct(normal, tangent1);
		}
	}

	void
	SoftStepSolver::prepare (const std::vector< Body > & bodies, std::vector< Manifold > & manifolds) noexcept
	{
		for ( auto & manifold : manifolds )
		{
			const auto & bodyA = bodies[manifold.bodyA];
			const auto & bodyB = bodies[manifold.bodyB];

			tangentBasis(manifold.normal, manifold.tangent1, manifold.tangent2);

			const CachedManifold * cached = nullptr;

			if ( const auto found = m_cache.find({manifold.keyA, manifold.keyB, manifold.keySub}); found != m_cache.end() )
			{
				cached = &found->second;
			}

			for ( auto & point : manifold.points )
			{
				point.anchorA = point.position - bodyA.position;
				point.anchorB = point.position - bodyB.position;
				/* The separation as a function of the motion: s = adjusted + n · ((pB + dB) − (pA + dA)). */
				point.adjustedSeparation = point.separation - Vec3::dotProduct(manifold.normal, point.anchorB - point.anchorA);
				point.normalMass = effectiveMass(bodyA, bodyB, point.anchorA, point.anchorB, manifold.normal);
				point.tangentMass1 = effectiveMass(bodyA, bodyB, point.anchorA, point.anchorB, manifold.tangent1);
				point.tangentMass2 = effectiveMass(bodyA, bodyB, point.anchorA, point.anchorB, manifold.tangent2);
				point.relativeVelocity = Vec3::dotProduct(manifold.normal, relativeVelocityAt(bodyA, bodyB, point.anchorA, point.anchorB));
				point.normalImpulse = 0.0F;
				point.tangentImpulse1 = 0.0F;
				point.tangentImpulse2 = 0.0F;
				point.maxNormalImpulse = 0.0F;

				/* Warm start: the impulses this feature carried at the last step. */
				if ( cached != nullptr )
				{
					for ( const auto & previous : cached->points )
					{
						if ( previous.featureId == point.featureId )
						{
							point.normalImpulse = previous.normalImpulse;
							point.tangentImpulse1 = previous.tangentImpulse1;
							point.tangentImpulse2 = previous.tangentImpulse2;

							break;
						}
					}
				}
			}
		}
	}

	void
	SoftStepSolver::warmStart (std::vector< Body > & bodies, const std::vector< Manifold > & manifolds) noexcept
	{
		for ( const auto & manifold : manifolds )
		{
			auto & bodyA = bodies[manifold.bodyA];
			auto & bodyB = bodies[manifold.bodyB];

			for ( const auto & point : manifold.points )
			{
				const Vec3 impulse = (manifold.normal * point.normalImpulse) + (manifold.tangent1 * point.tangentImpulse1) + (manifold.tangent2 * point.tangentImpulse2);

				applyImpulse(bodyA, bodyB, point.anchorA, point.anchorB, impulse);
			}
		}
	}

	void
	SoftStepSolver::solveContacts (std::vector< Body > & bodies, std::vector< Manifold > & manifolds, float biasRate, float massScale, float impulseScale, float inverseSubStep, bool useBias) noexcept
	{
		/* ⚠️ Both passes walk FORWARD. A symmetric sweep (the relax pass backward) was measured on 2026-10-01: it broke
		 * the coherence of the soft solve and the relax — the bench's stack toppled again (its top box at 17°), a box
		 * dropped flat tipped 10.5° instead of 5°. The 5° left is P2's axis-aligned envelope: nothing turns a tilted body
		 * back while its collision shape does not turn with it (P3). */
		const auto solvePoint = [&bodies, biasRate, massScale, impulseScale, inverseSubStep, useBias] (Manifold & manifold, Point & point) {
			auto & bodyA = bodies[manifold.bodyA];
			auto & bodyB = bodies[manifold.bodyB];

			{
				/* The current separation from the step's motion so far. */
				const Vec3 movedA = bodyA.deltaPosition + bodyA.deltaRotation.rotatedVector(point.anchorA);
				const Vec3 movedB = bodyB.deltaPosition + bodyB.deltaRotation.rotatedVector(point.anchorB);
				const float separation = point.adjustedSeparation + Vec3::dotProduct(manifold.normal, movedB - movedA);

				float bias = 0.0F;
				float pointMassScale = 1.0F;
				float pointImpulseScale = 0.0F;

				if ( separation > 0.0F )
				{
					/* Speculative: allow the gap to close within this sub-step, never more. */
					bias = separation * inverseSubStep;
				}
				else if ( useBias )
				{
					bias = std::max(biasRate * separation, -MaxPushoutVelocity);
					pointMassScale = massScale;
					pointImpulseScale = impulseScale;
				}

				/* The normal constraint. */
				const float normalSpeed = Vec3::dotProduct(manifold.normal, relativeVelocityAt(bodyA, bodyB, point.anchorA, point.anchorB));
				const float normalDelta = (-point.normalMass * pointMassScale * (normalSpeed + bias)) - (pointImpulseScale * point.normalImpulse);
				const float normalImpulse = std::max(point.normalImpulse + normalDelta, 0.0F);
				const float appliedNormal = normalImpulse - point.normalImpulse;

				point.normalImpulse = normalImpulse;
				point.maxNormalImpulse = std::max(point.maxNormalImpulse, appliedNormal);

				applyImpulse(bodyA, bodyB, point.anchorA, point.anchorB, manifold.normal * appliedNormal);

				/* The friction: both tangents together, clamped to the Coulomb disc of radius μ · normal impulse. */
				const Vec3 slip = relativeVelocityAt(bodyA, bodyB, point.anchorA, point.anchorB);
				float tangent1 = point.tangentImpulse1 - (point.tangentMass1 * Vec3::dotProduct(manifold.tangent1, slip));
				float tangent2 = point.tangentImpulse2 - (point.tangentMass2 * Vec3::dotProduct(manifold.tangent2, slip));
				const float maxFriction = manifold.friction * point.normalImpulse;
				const float lengthSquared = (tangent1 * tangent1) + (tangent2 * tangent2);

				if ( lengthSquared > maxFriction * maxFriction )
				{
					const float scale = lengthSquared > 0.0F ? maxFriction / std::sqrt(lengthSquared) : 0.0F;

					tangent1 *= scale;
					tangent2 *= scale;
				}

				const Vec3 frictionImpulse = (manifold.tangent1 * (tangent1 - point.tangentImpulse1)) + (manifold.tangent2 * (tangent2 - point.tangentImpulse2));

				point.tangentImpulse1 = tangent1;
				point.tangentImpulse2 = tangent2;

				applyImpulse(bodyA, bodyB, point.anchorA, point.anchorB, frictionImpulse);
			}
		};

		for ( auto & manifold : manifolds )
		{
			for ( auto & point : manifold.points )
			{
				solvePoint(manifold, point);
			}
		}
	}

	void
	SoftStepSolver::prepareWheels (const std::vector< Body > & bodies, std::vector< Wheel > & wheels) noexcept
	{
		/* Under this rolling speed (m/s) the slip ratio is measured against it (a wheel spinning at a standstill). Jolt's
		 * 1 mm/s: a 0.5 m/s floor made a few mm/s of creep a tiny slip, hence a tiny grip — a braked car crept down a
		 * 6° slope at 0.24 m/s. */
		constexpr float WheelSlipSpeedFloor{1.0e-3F};

		for ( auto & wheel : wheels )
		{
			wheel.suspensionImpulse = 0.0F;
			wheel.longitudinalImpulse = 0.0F;
			wheel.lateralImpulse = 0.0F;
			wheel.brakeImpulse = 0.0F;
			wheel.locked = false;

			if ( !wheel.contact )
			{
				continue;
			}

			const auto & ground = bodies[wheel.ground];
			const auto & chassis = bodies[wheel.chassis];

			wheel.anchorGround = wheel.contactPoint - ground.position;
			wheel.anchorChassis = wheel.contactPoint - chassis.position;
			wheel.adjustedSuspensionLength = wheel.suspensionLength - Vec3::dotProduct(wheel.normal, wheel.anchorChassis - wheel.anchorGround);
			wheel.suspensionMass = effectiveMass(ground, chassis, wheel.anchorGround, wheel.anchorChassis, wheel.normal);
			wheel.lateralMass = effectiveMass(ground, chassis, wheel.anchorGround, wheel.anchorChassis, wheel.side);

			/* The rolling direction couples the bodies and the wheel's spin: 1 / (1 / m + r² / I). */
			const auto rolling = effectiveMass(ground, chassis, wheel.anchorGround, wheel.anchorChassis, wheel.forward);
			const auto inverse = (rolling > 0.0F ? 1.0F / rolling : 0.0F) + (wheel.radius * wheel.radius / wheel.inertia);

			wheel.longitudinalMass = inverse > 0.0F ? 1.0F / inverse : 0.0F;
			wheel.rollingMass = rolling;

			/* The slips, from the step's start (the chassis' point relative to the ground's). */
			const Vec3 velocity = relativeVelocityAt(ground, chassis, wheel.anchorGround, wheel.anchorChassis);
			const auto longitudinal = Vec3::dotProduct(velocity, wheel.forward);
			const auto lateral = Vec3::dotProduct(velocity, wheel.side);

			wheel.slipRatio = ((wheel.angularVelocity * wheel.radius) - longitudinal) / std::max(std::abs(longitudinal), WheelSlipSpeedFloor);
			wheel.slipAngle = std::abs(longitudinal) > 0.0F || std::abs(lateral) > 0.0F ? std::atan2(std::abs(lateral), std::abs(longitudinal)) * (180.0F / std::numbers::pi_v< float >) : 0.0F;
			wheel.longitudinalCoefficient = wheel.longitudinalFriction != nullptr ? std::max(0.0F, wheel.longitudinalFriction->value(std::abs(wheel.slipRatio))) : 0.0F;
			wheel.lateralCoefficient = wheel.lateralFriction != nullptr ? std::max(0.0F, wheel.lateralFriction->value(wheel.slipAngle)) : 0.0F;
		}
	}

	void
	SoftStepSolver::warmStartWheels (std::vector< Body > & bodies, std::vector< Wheel > & wheels) noexcept
	{
		for ( auto & wheel : wheels )
		{
			if ( !wheel.contact )
			{
				continue;
			}

			auto & ground = bodies[wheel.ground];
			auto & chassis = bodies[wheel.chassis];
			const Vec3 impulse = (wheel.normal * wheel.suspensionImpulse) + (wheel.forward * wheel.longitudinalImpulse) + (wheel.side * wheel.lateralImpulse);

			applyImpulse(ground, chassis, wheel.anchorGround, wheel.anchorChassis, impulse);

			/* A locked wheel's rolling impulse went through its brake, not its spin. */
			if ( !wheel.locked )
			{
				wheel.angularVelocity -= wheel.longitudinalImpulse * wheel.radius / wheel.inertia;
			}
		}
	}

	void
	SoftStepSolver::solveWheels (std::vector< Body > & bodies, std::vector< Wheel > & wheels, float subStep, bool integrateSpin) noexcept
	{
		for ( auto & wheel : wheels )
		{
			/* The spin: its torques and damping, then the brake. A brake stronger than what stops the wheel within the
			 * sub-step LOCKS it: its spin 0, its surplus the bound of the tyre's rolling impulse (Jolt's model: the
			 * brake inside the solve; applied outside it, the light wheel spun back up every iteration and the car
			 * crept). Otherwise the brake slows the wheel. */
			if ( integrateSpin )
			{
				wheel.angularVelocity += wheel.driveTorque / wheel.inertia * subStep;
				wheel.angularVelocity /= 1.0F + (wheel.angularDamping * subStep);

				const auto lockTorque = std::abs(wheel.angularVelocity) * wheel.inertia / subStep;

				if ( wheel.brakeTorque > 0.0F && wheel.brakeTorque > lockTorque )
				{
					wheel.angularVelocity = 0.0F;
					wheel.brakeImpulse = (wheel.brakeTorque - lockTorque) * subStep / wheel.radius;
					wheel.locked = true;
				}
				else
				{
					wheel.angularVelocity -= std::copysign(wheel.brakeTorque / wheel.inertia * subStep, wheel.angularVelocity);
					wheel.brakeImpulse = 0.0F;
					wheel.locked = false;
				}
			}

			if ( !wheel.contact )
			{
				continue;
			}

			auto & ground = bodies[wheel.ground];
			auto & chassis = bodies[wheel.chassis];

			/* The suspension: a soft spring towards its rest (its maximum length), pushing only (Box2D's soft step with
			 * the wheel's own frequency and damping ratio — both passes, as Box2D's wheel joint spring). */
			{
				const Vec3 movedGround = ground.deltaPosition + ground.deltaRotation.rotatedVector(wheel.anchorGround);
				const Vec3 movedChassis = chassis.deltaPosition + chassis.deltaRotation.rotatedVector(wheel.anchorChassis);
				const auto length = wheel.adjustedSuspensionLength + Vec3::dotProduct(wheel.normal, movedChassis - movedGround);
				const auto compression = length - wheel.suspensionMaxLength;
				const auto omega = 2.0F * std::numbers::pi_v< float > * wheel.suspensionFrequency;
				const auto a1 = (2.0F * wheel.suspensionDamping) + (subStep * omega);
				const auto a2 = subStep * omega * a1;
				const auto a3 = 1.0F / (1.0F + a2);
				const auto biasRate = a1 > 0.0F ? omega / a1 : 0.0F;
				const auto speed = Vec3::dotProduct(wheel.normal, relativeVelocityAt(ground, chassis, wheel.anchorGround, wheel.anchorChassis));
				const auto delta = (-wheel.suspensionMass * a2 * a3 * (speed + (biasRate * compression))) - (a3 * wheel.suspensionImpulse);
				const auto impulse = std::max(wheel.suspensionImpulse + delta, 0.0F);

				applyImpulse(ground, chassis, wheel.anchorGround, wheel.anchorChassis, wheel.normal * (impulse - wheel.suspensionImpulse));
				wheel.suspensionImpulse = impulse;
			}

			/* The tyre: the contact point's rolling speed to the wheel's surface speed (the impulse spins the wheel), the
			 * side speed to 0; each bounded by its coefficient × the load, both by the friction ellipse. */
			{
				const Vec3 velocity = relativeVelocityAt(ground, chassis, wheel.anchorGround, wheel.anchorChassis);
				const auto rollingSlip = Vec3::dotProduct(velocity, wheel.forward) - (wheel.angularVelocity * wheel.radius);
				const auto sideSlip = Vec3::dotProduct(velocity, wheel.side);
				/* A locked wheel holds the contact still through its brake: the bodies' mass alone, at most the brake's
				 * surplus. A free one rolls: its spin takes part (the impulse spins it). */
				const auto maxLongitudinal = wheel.locked ? std::min(wheel.longitudinalCoefficient * wheel.suspensionImpulse, wheel.brakeImpulse) : wheel.longitudinalCoefficient * wheel.suspensionImpulse;
				const auto maxLateral = wheel.lateralCoefficient * wheel.suspensionImpulse;
				auto longitudinal = std::clamp(wheel.longitudinalImpulse - ((wheel.locked ? wheel.rollingMass : wheel.longitudinalMass) * rollingSlip), -maxLongitudinal, maxLongitudinal);
				auto lateral = std::clamp(wheel.lateralImpulse - (wheel.lateralMass * sideSlip), -maxLateral, maxLateral);

				if ( maxLongitudinal > 0.0F && maxLateral > 0.0F )
				{
					const auto ellipse = ((longitudinal / maxLongitudinal) * (longitudinal / maxLongitudinal)) + ((lateral / maxLateral) * (lateral / maxLateral));

					if ( ellipse > 1.0F )
					{
						const auto scale = 1.0F / std::sqrt(ellipse);

						longitudinal *= scale;
						lateral *= scale;
					}
				}

				const auto appliedLongitudinal = longitudinal - wheel.longitudinalImpulse;

				applyImpulse(ground, chassis, wheel.anchorGround, wheel.anchorChassis, (wheel.forward * appliedLongitudinal) + (wheel.side * (lateral - wheel.lateralImpulse)));

				if ( !wheel.locked )
				{
					wheel.angularVelocity -= appliedLongitudinal * wheel.radius / wheel.inertia;
				}
				wheel.longitudinalImpulse = longitudinal;
				wheel.lateralImpulse = lateral;
			}
		}
	}

	void
	SoftStepSolver::applyRestitution (std::vector< Body > & bodies, std::vector< Manifold > & manifolds) noexcept
	{
		for ( auto & manifold : manifolds )
		{
			if ( manifold.restitution <= 0.0F )
			{
				continue;
			}

			auto & bodyA = bodies[manifold.bodyA];
			auto & bodyB = bodies[manifold.bodyB];

			for ( auto & point : manifold.points )
			{
				/* Only a contact that was approaching fast enough, and that the solve actually pushed, bounces. */
				if ( point.relativeVelocity > -RestitutionThreshold || point.maxNormalImpulse <= 0.0F )
				{
					continue;
				}

				const float normalSpeed = Vec3::dotProduct(manifold.normal, relativeVelocityAt(bodyA, bodyB, point.anchorA, point.anchorB));
				const float delta = -point.normalMass * (normalSpeed + (manifold.restitution * point.relativeVelocity));
				const float normalImpulse = std::max(point.normalImpulse + delta, 0.0F);
				const float applied = normalImpulse - point.normalImpulse;

				point.normalImpulse = normalImpulse;
				point.maxNormalImpulse = std::max(point.maxNormalImpulse, applied);

				applyImpulse(bodyA, bodyB, point.anchorA, point.anchorB, manifold.normal * applied);
			}
		}
	}

	void
	SoftStepSolver::storeImpulses (const std::vector< Manifold > & manifolds) noexcept
	{
		for ( const auto & manifold : manifolds )
		{
			auto & cached = m_cache[{manifold.keyA, manifold.keyB, manifold.keySub}];

			cached.points.clear();
			cached.lastStep = m_stepCount;

			for ( const auto & point : manifold.points )
			{
				cached.points.push_back({.featureId = point.featureId, .normalImpulse = point.normalImpulse, .tangentImpulse1 = point.tangentImpulse1, .tangentImpulse2 = point.tangentImpulse2});
			}
		}

		/* Forget the pairs that did not touch at this step (erasing never changes a result: only lookups are used). */
		std::erase_if(m_cache, [this] (const auto & entry) {
			return entry.second.lastStep != m_stepCount;
		});
	}

	void
	SoftStepSolver::step (std::vector< Body > & bodies, std::vector< Manifold > & manifolds, std::vector< Wheel > & wheels, const Vec3 & gravity, float deltaTime) noexcept
	{
		if ( bodies.empty() || !(deltaTime > 0.0F) )
		{
			return;
		}

		++m_stepCount;

		const float subStep = deltaTime / static_cast< float >(SubStepCount);
		const float inverseSubStep = 1.0F / subStep;

		/* The soft contact (Box2D's b2MakeSoft): a spring of ContactHertz, capped to a quarter of the sub-step rate. */
		const float hertz = std::min(ContactHertz, 0.25F * inverseSubStep);
		const float omega = 2.0F * std::numbers::pi_v< float > * hertz;
		const float a1 = (2.0F * ContactDampingRatio) + (subStep * omega);
		const float a2 = subStep * omega * a1;
		const float a3 = 1.0F / (1.0F + a2);
		const float biasRate = omega / a1;
		const float massScale = a2 * a3;
		const float impulseScale = a3;

		for ( auto & body : bodies )
		{
			body.deltaPosition = Vec3{};
			body.deltaRotation = Quaternion< float >{};
		}

		this->prepare(bodies, manifolds);
		prepareWheels(bodies, wheels);

		for ( uint32_t subStepIndex = 0; subStepIndex < SubStepCount; ++subStepIndex )
		{
			/* 1. The velocities: gravity on the dynamic bodies. */
			for ( auto & body : bodies )
			{
				if ( body.dynamic && body.gravity )
				{
					body.linearVelocity += gravity * subStep;
				}
			}

			/* 2. Warm start, 3. the soft solve. */
			warmStart(bodies, manifolds);
			warmStartWheels(bodies, wheels);
			solveContacts(bodies, manifolds, biasRate, massScale, impulseScale, inverseSubStep, true);
			solveWheels(bodies, wheels, subStep, true);

			/* 4. The positions: the dynamic bodies move, the kinematic ones carry their velocity. */
			for ( auto & body : bodies )
			{
				if ( !body.dynamic && body.movable == nullptr && body.linearVelocity.lengthSquared() <= 0.0F )
				{
					continue;
				}

				const Vec3 translation = body.linearVelocity * subStep;

				body.position += translation;
				body.deltaPosition += translation;

				if ( body.dynamic && body.angularVelocity.lengthSquared() > 0.0F )
				{
					body.orientation = RigidBody::integrateOrientation(body.orientation, body.angularVelocity, subStep);
					body.deltaRotation = RigidBody::integrateOrientation(body.deltaRotation, body.angularVelocity, subStep);
				}
			}

			/* 5. Relax: the rigid pass, no bias, removes the soft solve's velocity overshoot. */
			solveContacts(bodies, manifolds, biasRate, massScale, impulseScale, inverseSubStep, false);
			solveWheels(bodies, wheels, subStep, false);
		}

		for ( uint32_t iteration = 0; iteration < RestitutionIterations; ++iteration )
		{
			applyRestitution(bodies, manifolds);
		}

		this->storeImpulses(manifolds);
	}
}
