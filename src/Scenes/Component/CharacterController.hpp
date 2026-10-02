/*
 * src/Scenes/Component/CharacterController.hpp
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
#include <cstring>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Physics/CharacterController.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A KINEMATIC character controller on a Node (physics overhaul P4, owner decisions 9): the node is moved by
	 * collide and slide, not simulated — it walks, climbs steps, follows slopes and moving platforms, jumps, and is a
	 * solid for the dynamic bodies, which it pushes with a bounded force.
	 * @note The entity takes a capsule collision model standing on its origin (`Physics::CharacterController::
	 * localCapsule()`) and becomes kinematic for the physics step, which moves it before its solver
	 * (`Scene::resolveCollisions()` step 0). Its events are notified after the step, outside the physics lock.
	 * @note The game drives it with setWantedVelocity() and jump(); the parameters are those of
	 * `Physics::CharacterController` (controller()).
	 * @extends EmEn::Scenes::Component::Abstract
	 */
	class EMEN_API CharacterController final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CharacterController"};

			/** @brief Observable notification codes (notified after the physics step). */
			enum NotificationCode : std::uint8_t
			{
				/** Landed on a walkable surface; data: the fall speed (float, m/s). */
				Landed,
				/** Left the ground (a jump, a ledge). */
				LeftGround,
				/** Hit a wall, a slope too steep or a ceiling. */
				HitWall,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs a character controller component.
			 * @param componentName A reference to the component name.
			 * @param parentEntity A reference to the parent entity.
			 */
			CharacterController (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{
				/* It shapes the collider itself: never merged into the entity's extents. */
				this->setContributesToEntityExtents(false);
			}

			/**
			 * @brief Destructs the component.
			 * @note ⚠️ Defined OUT OF LINE on purpose: it is the class's KEY FUNCTION, so its vtable and typeinfo live in
			 * the engine library only. With every virtual inline they were emitted, hidden, in the application that
			 * builds the component, and on macOS (libc++ compares type_info by address) the engine's dynamic_cast to it
			 * failed: no character on macOS (2026-10-02). docs/caution-points.md.
			 */
			~CharacterController () override;

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void
			processLogics (const Scene & /*scene*/) noexcept override
			{
				/* The physics step moves it (Scene::resolveCollisions()). */
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/**
			 * @brief Returns the controller (its parameters, inputs and state).
			 * @return Physics::CharacterController &
			 */
			[[nodiscard]]
			Physics::CharacterController &
			controller () noexcept
			{
				return m_controller;
			}

			/**
			 * @brief Returns the controller (its parameters, inputs and state).
			 * @return const Physics::CharacterController &
			 */
			[[nodiscard]]
			const Physics::CharacterController &
			controller () const noexcept
			{
				return m_controller;
			}

			/**
			 * @brief Notifies the events of the last physics step. Called by the scene after its step, outside its lock.
			 * @return void
			 */
			void notifyEvents () noexcept;

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool
			playAnimation (uint8_t /*animationID*/, const Base::Variant & /*value*/, size_t /*cycle*/) noexcept override
			{
				return false;
			}

			Physics::CharacterController m_controller;
	};
}
