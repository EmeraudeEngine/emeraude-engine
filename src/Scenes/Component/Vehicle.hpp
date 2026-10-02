/*
 * src/Scenes/Component/Vehicle.hpp
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
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Physics/Vehicle.hpp"
#include "StaticVector.hpp"

namespace EmEn::Scenes
{
	class Node;
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief A wheeled vehicle on a DYNAMIC node (its chassis): the wheels, the drive train, the driver's inputs (physics
	 * overhaul bonus phase, decisions 15). The scene's physics step casts its wheels and solves them with the contacts;
	 * this component moves the wheel nodes attached to it (suspension, steering, spin).
	 * @note Its settings through controller().setup(); a wheel node through attachWheelNode() — a child of the chassis,
	 * its mesh rolling about its local X axis when the wheel's forward is −Z.
	 * @extends EmEn::Scenes::Component::Abstract
	 */
	class EMEN_API Vehicle final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Vehicle"};

			/** @brief A piece of an input script: the inputs from a physics cycle on (a deterministic driver). */
			struct ScriptedInput final
			{
				uint64_t fromCycle{0};
				float forward{0.0F};
				float right{0.0F};
				float brake{0.0F};
				float handBrake{0.0F};
			};

			/**
			 * @brief Constructs a vehicle component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity (the chassis).
			 */
			Vehicle (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{
				/* It has no extent of its own. */
				this->setContributesToEntityExtents(false);
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Vehicle (const Vehicle & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Vehicle (Vehicle && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Vehicle &
			 */
			Vehicle & operator= (const Vehicle & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Vehicle &
			 */
			Vehicle & operator= (Vehicle && copy) noexcept = delete;

			/** @brief The out-of-line destructor (the key function: one typeinfo across shared libraries). */
			~Vehicle () override;

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

			/**
			 * @copydoc EmEn::Scenes::Component::Abstract::processLogics()
			 * @note The input script's entry for this cycle, then the wheel nodes from the last physics step.
			 */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @brief Returns the vehicle. */
			[[nodiscard]]
			Physics::VehicleController &
			controller () noexcept
			{
				return m_controller;
			}

			/** @brief Returns the vehicle. */
			[[nodiscard]]
			const Physics::VehicleController &
			controller () const noexcept
			{
				return m_controller;
			}

			/**
			 * @brief Attaches the node a wheel moves (a child of the chassis).
			 * @param wheelIndex The wheel (its index in the settings).
			 * @param node The node, or nullptr to detach it.
			 * @return bool False for a wheel that does not exist.
			 */
			[[nodiscard]]
			bool attachWheelNode (size_t wheelIndex, const std::shared_ptr< Node > & node) noexcept;

			/**
			 * @brief Sets an input script: each entry applies from its physics cycle on (the entries in increasing cycles).
			 * @note An empty script leaves the inputs to setInput().
			 * @param script The entries.
			 * @return bool False (nothing changed) for decreasing cycles or a non-finite value.
			 */
			[[nodiscard]]
			bool setInputScript (std::vector< ScriptedInput > script) noexcept;

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::playAnimation() */
			bool
			playAnimation (uint8_t /*animationID*/, const Base::Variant & /*value*/, size_t /*cycle*/) noexcept override
			{
				return false;
			}

			Physics::VehicleController m_controller;
			Base::StaticVector< std::weak_ptr< Node >, 8 > m_wheelNodes;
			std::vector< ScriptedInput > m_script;
	};
}
