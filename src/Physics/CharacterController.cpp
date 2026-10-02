/*
 * src/Physics/CharacterController.cpp
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

#include "CharacterController.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

namespace EmEn::Physics
{
	using namespace Base::Math;
	using namespace Base::Math::Space3D;

	namespace
	{
		/** @brief Below this length (m) a motion is nothing. */
		constexpr float CharacterNegligibleMotion{1.0e-6F};

		/** @brief How fast the air control reaches the wanted velocity (1 / s, at air control 1). */
		constexpr float CharacterAirResponse{10.0F};

		/** @brief A surface whose normal points this far down is a ceiling (cosine with upward). */
		constexpr float CharacterCeilingCosine{-0.3F};

		/** @brief How fast a flying character reaches its wanted velocity (1 / s: about 0.1 s to start or stop). */
		constexpr float CharacterFlyResponse{10.0F};

		/** @brief The least horizontal progress (m) that makes a step-up worth it. */
		constexpr float CharacterStepProgress{1.0e-3F};

		/** @brief How far past a step's edge the probe looks for its top (m). */
		constexpr float CharacterStepProbeAhead{0.02F};

		/** @brief How high over the edge the probe starts (m). */
		constexpr float CharacterStepProbeRise{0.05F};

		/** @brief The probe's radius (m). */
		constexpr float CharacterStepProbeRadius{0.005F};

		/** @brief The vector without its part along a unit direction. */
		[[nodiscard]]
		Vector< 3, float >
		withoutAlong (const Vector< 3, float > & vector, const Vector< 3, float > & direction) noexcept
		{
			return vector - (direction * Vector< 3, float >::dotProduct(vector, direction));
		}

		/** @brief The finite check of a vector. */
		[[nodiscard]]
		bool
		isFinite (const Vector< 3, float > & vector) noexcept
		{
			return std::isfinite(vector[X]) && std::isfinite(vector[Y]) && std::isfinite(vector[Z]);
		}
	}

	bool
	CharacterController::setSize (float radius, float height) noexcept
	{
		if ( !std::isfinite(radius) || !std::isfinite(height) || !(radius > 0.0F) || height < radius * 2.0F )
		{
			return false;
		}

		m_radius = radius;
		m_height = height;

		return true;
	}

	bool
	CharacterController::setStepHeight (float height) noexcept
	{
		if ( !std::isfinite(height) || height < 0.0F )
		{
			return false;
		}

		m_stepHeight = height;

		return true;
	}

	bool
	CharacterController::setMaxSlope (float degrees) noexcept
	{
		if ( !std::isfinite(degrees) || degrees < 0.0F || degrees > 89.0F )
		{
			return false;
		}

		m_walkableCosine = std::cos(degrees * (std::numbers::pi_v< float > / 180.0F));

		return true;
	}

	bool
	CharacterController::setSnapDistance (float distance) noexcept
	{
		if ( !std::isfinite(distance) || distance < 0.0F )
		{
			return false;
		}

		m_snapDistance = distance;

		return true;
	}

	bool
	CharacterController::setAirControl (float control) noexcept
	{
		if ( !std::isfinite(control) || control < 0.0F || control > 1.0F )
		{
			return false;
		}

		m_airControl = control;

		return true;
	}

	bool
	CharacterController::setPushForce (float force) noexcept
	{
		if ( !std::isfinite(force) || force < 0.0F )
		{
			return false;
		}

		m_pushForce = force;

		return true;
	}

	bool
	CharacterController::setWantedVelocity (const Vector< 3, float > & velocity) noexcept
	{
		if ( !isFinite(velocity) )
		{
			return false;
		}

		m_wantedVelocity = velocity;

		return true;
	}

	bool
	CharacterController::jump (float launchSpeed) noexcept
	{
		if ( !std::isfinite(launchSpeed) || !(launchSpeed > 0.0F) )
		{
			return false;
		}

		m_jumpSpeed = launchSpeed;
		m_jumpRequested = true;

		return true;
	}

	void
	CharacterController::setFlying (bool state) noexcept
	{
		if ( state == m_flying )
		{
			return;
		}

		/* Airborne, the ground normal is the upward direction: the last step's velocity splits along it. */
		if ( state )
		{
			m_flyVelocity = m_velocity;
		}
		else
		{
			m_verticalSpeed = Vector< 3, float >::dotProduct(m_velocity, m_groundNormal);
			m_horizontalVelocity = withoutAlong(m_velocity, m_groundNormal);
		}

		m_flying = state;
	}

	Capsule< float >
	CharacterController::localCapsule () const noexcept
	{
		return Capsule< float >{Vector< 3, float >{0.0F, m_radius, 0.0F}, Vector< 3, float >{0.0F, m_height - m_radius, 0.0F}, m_radius};
	}

	Capsule< float >
	CharacterController::capsuleAt (const Vector< 3, float > & feet, const Vector< 3, float > & upward) const noexcept
	{
		return Capsule< float >{feet + (upward * m_radius), feet + (upward * (m_height - m_radius)), m_radius};
	}

	Vector< 3, float >
	CharacterController::step (CharacterWorldInterface & world, const Vector< 3, float > & feet, const Vector< 3, float > & upward, float gravity, float deltaTime) noexcept
	{
		m_events = {};

		if ( !isFinite(feet) || !isFinite(upward) || !std::isfinite(gravity) || !(deltaTime > 0.0F) )
		{
			return feet;
		}

		auto position = feet;

		/* 1. Out of what overlaps the capsule (a platform rising into it, a solid placed on it), and back to a skin width
		 * from what it touches: the capsule tested is inflated by the skin. Starting a step closer than that, every sweep
		 * — even along the ground — met the ground at fraction 0 and the character never moved. */
		for ( uint32_t pass = 0; pass < 3; ++pass )
		{
			auto skinned = this->capsuleAt(position, upward);
			skinned.setRadius(m_radius + SkinWidth);

			auto correction = world.penetrationCorrection(skinned);
			const auto lengthSquared = correction.lengthSquared();

			if ( lengthSquared < CharacterNegligibleMotion * CharacterNegligibleMotion )
			{
				break;
			}

			/* Out of a WALKABLE surface the character goes straight up, by the height that clears the same distance from
			 * its plane: out along a slope's normal, then snapped straight down by the ground probe, a character standing
			 * still crept downhill (1.3 cm/s on a terrain slope). */
			const auto rise = Vector< 3, float >::dotProduct(correction, upward);

			if ( rise > 0.0F && rise * rise >= m_walkableCosine * m_walkableCosine * lengthSquared )
			{
				correction = upward * (lengthSquared / rise);
			}

			position += correction;
		}

		if ( m_flying )
		{
			return this->fly(world, position, feet, upward, deltaTime);
		}

		/* 2. The velocities of this step. */
		const bool wasGrounded = m_grounded;
		bool jumped = false;

		if ( m_jumpRequested )
		{
			m_jumpRequested = false;

			if ( m_grounded )
			{
				m_verticalSpeed = m_jumpSpeed;
				m_grounded = false;
				jumped = true;
			}
		}

		if ( m_grounded )
		{
			m_verticalSpeed = 0.0F;
		}
		else
		{
			m_verticalSpeed -= gravity * deltaTime;
		}

		const auto wanted = withoutAlong(m_wantedVelocity, upward);

		if ( m_grounded )
		{
			m_horizontalVelocity = wanted;
		}
		else
		{
			const auto blend = std::min(1.0F, m_airControl * CharacterAirResponse * deltaTime);

			m_horizontalVelocity += (wanted - m_horizontalVelocity) * blend;
		}

		auto motion = m_horizontalVelocity * deltaTime;

		/* On the ground the walk follows the surface: the horizontal speed is kept up and down a walkable slope. */
		if ( m_grounded )
		{
			const auto normalUp = Vector< 3, float >::dotProduct(m_groundNormal, upward);

			if ( normalUp > 0.0F )
			{
				motion -= upward * (Vector< 3, float >::dotProduct(m_groundNormal, motion) / normalUp);
			}

			/* Carried by what it stands on (a moving platform, a dynamic body). */
			motion += m_supportVelocity * deltaTime;
		}

		motion += upward * (m_verticalSpeed * deltaTime);

		const auto fallSpeed = -m_verticalSpeed;

		/* 3. Collide and slide. */
		bool stepped = false;

		position = this->slide(world, position, motion, upward, wasGrounded && !jumped, deltaTime, stepped);

		/* 4. The ground probe: snap back onto a walkable surface (stairs and slopes down), or detect a landing. A step
		 * just climbed keeps the character grounded: its rounded bottom rests on the step's edge, whose normal leans, until
		 * its axis is over the step (the next steps climb on — the stair walk of PhysX's character controller). */
		bool grounded = false;

		if ( stepped )
		{
			grounded = true;
			m_groundNormal = upward;
			m_supportVelocity.reset();
		}
		else if ( !jumped && m_verticalSpeed <= 0.0F )
		{
			const auto probeLength = wasGrounded ? m_snapDistance + SkinWidth : SkinWidth * 2.0F;
			CharacterWorldInterface::Hit hit;

			if ( this->probeGround(world, position, upward, probeLength, hit) )
			{
				const auto travel = std::max(0.0F, (hit.fraction * probeLength) - SkinWidth);

				position -= upward * travel;
				grounded = true;
				m_groundNormal = hit.normal;
				m_supportVelocity = hit.velocity;
				m_supportKey = hit.key;
			}
		}

		if ( grounded )
		{
			if ( !wasGrounded )
			{
				m_events.landed = true;
				m_events.landingSpeed = std::max(0.0F, fallSpeed);
			}

			m_verticalSpeed = 0.0F;
		}
		else
		{
			if ( wasGrounded )
			{
				m_events.leftGround = true;

				/* Leaving a moving support keeps its velocity (a jump off a platform). */
				m_horizontalVelocity += withoutAlong(m_supportVelocity, upward);
			}

			m_groundNormal = upward;
			m_supportVelocity.reset();
			m_supportKey = 0;
		}

		m_grounded = grounded;
		m_velocity = (position - feet) * (1.0F / deltaTime);

		return position;
	}

	Vector< 3, float >
	CharacterController::fly (CharacterWorldInterface & world, const Vector< 3, float > & start, const Vector< 3, float > & feet, const Vector< 3, float > & upward, float deltaTime) noexcept
	{
		/* No jump in flight: a request does not wait for the landing. */
		m_jumpRequested = false;

		const auto blend = std::min(1.0F, CharacterFlyResponse * deltaTime);

		m_flyVelocity += (m_wantedVelocity - m_flyVelocity) * blend;

		/* Collide and slide, without climbing steps: nothing to stand on. */
		bool stepped = false;
		const auto position = this->slide(world, start, m_flyVelocity * deltaTime, upward, false, deltaTime, stepped);

		if ( m_grounded )
		{
			m_events.leftGround = true;
		}

		m_grounded = false;
		m_verticalSpeed = 0.0F;
		m_groundNormal = upward;
		m_supportVelocity.reset();
		m_supportKey = 0;
		m_velocity = (position - feet) * (1.0F / deltaTime);

		return position;
	}

	bool
	CharacterController::probeGround (const CharacterWorldInterface & world, const Vector< 3, float > & position, const Vector< 3, float > & upward, float length, CharacterWorldInterface::Hit & hit) const noexcept
	{
		const auto probe = upward * -length;

		if ( !world.sweep(this->capsuleAt(position, upward), probe, hit) )
		{
			return false;
		}

		if ( Vector< 3, float >::dotProduct(hit.normal, upward) >= m_walkableCosine )
		{
			return true;
		}

		/* The rounded bottom resting on a step's EDGE (its normal leans): supported when the step's top just beyond the
		 * contact, away from the capsule's axis, is walkable. */
		const auto axisPoint = position + (upward * m_radius);
		const auto away = withoutAlong(hit.point - axisPoint, upward);

		if ( away.lengthSquared() > CharacterNegligibleMotion && this->walkableBeyond(world, hit.point, away.normalized(), upward) )
		{
			hit.normal = upward;

			return true;
		}

		/* The full capsule met a slope too steep first (it stands against it): a probe of half the radius looks for the
		 * floor under the character itself. Its fraction is then measured from the same feet. */
		auto narrow = this->capsuleAt(position, upward);
		narrow.setRadius(m_radius * 0.5F);

		const auto lowered = upward * (m_radius * 0.5F);
		narrow = Capsule< float >{narrow.startPoint() - lowered, narrow.endPoint() - lowered, narrow.radius()};

		return world.sweep(narrow, probe, hit) && Vector< 3, float >::dotProduct(hit.normal, upward) >= m_walkableCosine;
	}

	bool
	CharacterController::walkableBeyond (const CharacterWorldInterface & world, const Vector< 3, float > & point, const Vector< 3, float > & direction, const Vector< 3, float > & upward) const noexcept
	{
		const auto probeStart = point + (direction * CharacterStepProbeAhead) + (upward * CharacterStepProbeRise);
		const Capsule< float > probe{probeStart, probeStart, CharacterStepProbeRadius};
		CharacterWorldInterface::Hit top;

		return world.sweep(probe, upward * -(CharacterStepProbeRise * 2.0F), top) && Vector< 3, float >::dotProduct(top.normal, upward) >= m_walkableCosine;
	}

	Vector< 3, float >
	CharacterController::slide (CharacterWorldInterface & world, Vector< 3, float > position, Vector< 3, float > motion, const Vector< 3, float > & upward, bool canStep, float deltaTime, bool & stepped) noexcept
	{
		for ( uint32_t iteration = 0; iteration < MaxSlideIterations; ++iteration )
		{
			const auto length = motion.length();

			if ( length < CharacterNegligibleMotion )
			{
				break;
			}

			CharacterWorldInterface::Hit hit;

			if ( !world.sweep(this->capsuleAt(position, upward), motion, hit) )
			{
				position += motion;

				break;
			}

			/* Stop a skin width before the obstacle. */
			const auto travel = std::max(0.0F, (hit.fraction * length) - SkinWidth);
			const auto advance = motion * (travel / length);

			position += advance;

			auto remaining = motion - advance;
			const auto & normal = hit.normal;
			const auto normalUp = Vector< 3, float >::dotProduct(normal, upward);

			/* A dynamic body in the way is pushed, with a bounded force; the character does not go through it. */
			if ( hit.dynamic && m_pushForce > 0.0F )
			{
				const auto into = withoutAlong(-normal, upward);

				if ( into.lengthSquared() > CharacterNegligibleMotion )
				{
					world.push(hit.bodyIndex, hit.point, into.normalized() * (m_pushForce * deltaTime));
				}
			}

			if ( normalUp >= m_walkableCosine )
			{
				/* A walkable surface (a floor reached falling, a slope walked up): slide along it. */
				if ( Vector< 3, float >::dotProduct(normal, remaining) < 0.0F )
				{
					remaining = withoutAlong(remaining, normal);
				}

				m_verticalSpeed = std::max(m_verticalSpeed, 0.0F);
			}
			else
			{
				/* A low obstacle in the way of a walk: climb it. */
				if ( canStep && m_stepHeight > 0.0F && normalUp > CharacterCeilingCosine )
				{
					Vector< 3, float > reached;

					if ( this->stepUp(world, position, withoutAlong(remaining, upward), upward, reached) )
					{
						position = reached;
						stepped = true;

						break;
					}
				}

				/* A wall or a slope too steep: it only blocks, horizontally — never a ramp to climb. */
				auto blocking = normal;

				if ( normalUp > 0.0F )
				{
					const auto horizontal = withoutAlong(normal, upward);

					if ( horizontal.lengthSquared() > CharacterNegligibleMotion )
					{
						blocking = horizontal.normalized();
					}
				}

				if ( Vector< 3, float >::dotProduct(blocking, remaining) < 0.0F )
				{
					remaining = withoutAlong(remaining, blocking);
				}

				/* A ceiling stops a rise. */
				if ( normalUp < CharacterCeilingCosine && m_verticalSpeed > 0.0F )
				{
					m_verticalSpeed = 0.0F;
					remaining = withoutAlong(remaining, upward);
				}

				m_events.hitWall = m_events.hitWall || normalUp <= CharacterCeilingCosine || normalUp < m_walkableCosine;
			}

			motion = remaining;
		}

		return position;
	}

	bool
	CharacterController::stepUp (const CharacterWorldInterface & world, const Vector< 3, float > & position, const Vector< 3, float > & horizontalMotion, const Vector< 3, float > & upward, Vector< 3, float > & reached) const noexcept
	{
		const auto forwardLength = horizontalMotion.length();

		if ( forwardLength < CharacterNegligibleMotion )
		{
			return false;
		}

		CharacterWorldInterface::Hit hit;

		/* Up, as high as the step height and the headroom allow. */
		auto rise = m_stepHeight;

		if ( world.sweep(this->capsuleAt(position, upward), upward * m_stepHeight, hit) )
		{
			rise = std::max(0.0F, (hit.fraction * m_stepHeight) - SkinWidth);
		}

		if ( rise < SkinWidth )
		{
			return false;
		}

		const auto raised = position + (upward * rise);

		/* Forward, at that height. */
		auto forward = horizontalMotion;

		if ( world.sweep(this->capsuleAt(raised, upward), horizontalMotion, hit) )
		{
			forward = horizontalMotion * (std::max(0.0F, (hit.fraction * forwardLength) - SkinWidth) / forwardLength);
		}

		if ( forward.length() < CharacterStepProgress )
		{
			return false;
		}

		const auto ahead = raised + forward;

		/* Down onto the step: it must be walkable. */
		const auto drop = rise + SkinWidth;

		if ( !world.sweep(this->capsuleAt(ahead, upward), upward * -drop, hit) )
		{
			return false;
		}

		/* The rounded bottom of the capsule usually lands on the step's EDGE, whose normal leans: a thin vertical probe
		 * just past the edge tells whether the step's top is walkable (Jolt's stair walk does the same). */
		if ( Vector< 3, float >::dotProduct(hit.normal, upward) < m_walkableCosine && !this->walkableBeyond(world, hit.point, horizontalMotion * (1.0F / forwardLength), upward) )
		{
			return false;
		}

		reached = ahead - (upward * std::max(0.0F, (hit.fraction * drop) - SkinWidth));

		return true;
	}
}
