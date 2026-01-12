/*
 * src/Scenes/NodeController.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <memory>

/* Local inclusions for inheritances. */
#include "Input/KeyboardListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Input/GamepadController.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Node.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The NodeController class
	 * @extends EmEn::Input::KeyboardListenerInterface
	 */
	class NodeController final : public Input::KeyboardListenerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NodeController"};

			/**
			 * @brief Constructs a node controller.
			 */
			NodeController () noexcept
				: KeyboardListenerInterface{false, false}
			{

			}

			/**
			 * @brief Sets a node to control.
			 * @param node The scene node.
			 * @return void
			 */
			void
			controlNode (const std::shared_ptr< Node > & node) noexcept
			{
				m_controlledNode = node;
			}

			/**
			 * @brief Releases the node.
			 * @return void
			 */
			void
			releaseNode () noexcept
			{
				m_controlledNode.reset();
			}

			/**
			 * @brief Returns whether the controlled has a node.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasNode () const noexcept
			{
				return m_controlledNode != nullptr;
			}

			/**
			 * @brief Returns the controlled node.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			node () const noexcept
			{
				return m_controlledNode;
			}

			/**
			 * @brief Connects a gamepad controller to this node controller.
			 * @param deviceId The ID of the device. The first is 0.
			 * @param moveStep Set a step value to move the node. Default 6.0.
			 * @param rotationAngleStep Set the rate of speed to rotate the node. Default 1.0.
			 * @return void
			 */
			void
			connectDevice (int deviceId, float moveStep = DefaultMoveStep, float rotationAngleStep = DefaultRotationAngleStep) noexcept
			{
				m_moveStep = moveStep;
				m_rotationAngleStep = rotationAngleStep;

				m_controller.attachDeviceID(deviceId);
			}

			/**
			 * @brief Disconnects the gamepad device from this node controller.
			 * @return void
			 */
			void
			disconnectDevice () noexcept
			{
				m_controller.detachDevice();
			}

			/**
			 * @brief Returns whether a gamepad is connected to this node controller.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasDeviceConnected () const noexcept
			{
				return m_controller.isConnected();
			}

			/**
			 * @brief Checks the controller buttons. This should be called in logic process loop.
			 * @return void
			 */
			void update () const noexcept;

		private:

			/**
			 * @brief On key release event handler.
			 * @param key The keyboard universal key code. I.e, QWERTY keyboard 'A' key gives the ASCII code '65' on all platform.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @return bool
			 */
			bool onKeyRelease (int key, int scancode, int modifiers) noexcept override;

			/**
			 * @brief switchTransformSpace
			 * @return void
			 */
			void switchTransformSpace () noexcept;

			/**
			 * @brief switchControlMode
			 * @return void
			 */
			void switchControlMode () noexcept;

			/* Default values */
			static constexpr auto DefaultMoveStep{6.0F};
			static constexpr auto DefaultRotationAngleStep{1.0F};

			std::shared_ptr< Node > m_controlledNode{};
			Libs::Math::TransformSpace m_transformSpace{Libs::Math::TransformSpace::Parent};
			Input::GamepadController m_controller{};
			float m_moveStep{DefaultMoveStep};
			float m_rotationAngleStep{DefaultRotationAngleStep};
			bool m_nodeRotationMode{false};
	};
}
