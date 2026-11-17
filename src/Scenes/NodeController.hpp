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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>

/* Local inclusions for inheritances. */
#include "AbstractNodeController.hpp"
#include "Input/KeyboardListenerInterface.hpp"

/* Local inclusions for usages. */
#include "Input/GamepadController.hpp"
#include "Math/CartesianFrame.hpp"
#include "Mutex.hpp"
#include "StaticVector.hpp"
#include "ThreadSafety.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Node;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Moves and turns a scene node step by step, from the numeric keypad or a gamepad (a debug control).
	 * @details The keypad: 7/4 the X axis, 8/5 the Y axis, 9/6 the Z axis — a translation step, or a rotation step
	 * once the decimal key switched the mode; 0 switches the transform space. A key release RECORDS a step; update()
	 * applies the recorded steps on the logic thread, then polls the gamepad (the family's contract:
	 * AbstractNodeController).
	 * @extends EmEn::Scenes::AbstractNodeController The controlled node, and who writes it.
	 * @extends EmEn::Input::KeyboardListenerInterface
	 */
	class EMEN_API NodeController final : public AbstractNodeController, public Input::KeyboardListenerInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NodeController"};

			/** @brief How many keypad steps wait for the logic thread at most; a step asked beyond is dropped (the logic thread is paused or far behind). */
			static constexpr size_t MaxPendingSteps{32};

			/** @brief The size of one step of the node, by the keypad or the gamepad. */
			struct StepSizes final
			{
				float move{6.0F}; /**< A translation step, in metres. */
				float rotationAngle{1.0F}; /**< A rotation step, in degrees. */
			};

			/**
			 * @brief Constructs a node controller.
			 */
			NodeController () noexcept
				: KeyboardListenerInterface{false, false}
			{

			}

			/**
			 * @brief Connects a gamepad controller to this node controller.
			 * @note The device slot is changed between two logic cycles, like every controller's (input doc 03).
			 * @param deviceId The ID of the device. The first is 0.
			 * @param stepSizes The size of a translation and of a rotation step.
			 */
			void connectDevice (int deviceId, const StepSizes & stepSizes) noexcept EMEN_EXCLUDES(m_inputAccess);

			/**
			 * @brief Connects a gamepad controller to this node controller, with the default step sizes (6 m, 1 degree).
			 * @note See the other overload.
			 * @param deviceId The ID of the device. The first is 0.
			 */
			void
			connectDevice (int deviceId) noexcept EMEN_EXCLUDES(m_inputAccess)
			{
				/* NOTE: Not a default argument: a nested type with default member initializers is not complete there. */
				this->connectDevice(deviceId, StepSizes{});
			}

			/**
			 * @brief Disconnects the gamepad device from this node controller.
			 * @note Between two logic cycles (see connectDevice()).
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

		private:

			/** @brief The axis a step acts on: a translation along it, or a rotation around it. */
			enum class Axis : uint8_t
			{
				X,
				Y,
				Z
			};

			/** @brief One step asked by the keypad, waiting for the logic thread. */
			struct Step final
			{
				float amount{0.0F}; /**< Metres for a translation, radians for a rotation. */
				Base::Math::TransformSpace space{Base::Math::TransformSpace::Parent};
				Axis axis{Axis::X};
				bool rotation{false};
			};

			/** @copydoc EmEn::Scenes::AbstractNodeController::onUpdate() */
			void onUpdate (Node & node) noexcept override EMEN_EXCLUDES(m_inputAccess);

			/**
			 * @brief On key release event handler: records a step, or switches the mode or the transform space.
			 * @note MAIN thread (the input manager's event pump). Never touches the node.
			 * @param key The keyboard universal key code. I.e, QWERTY keyboard 'A' key gives the ASCII code '65' on all platform.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @return bool
			 */
			bool onKeyRelease (int key, int scancode, int modifiers) noexcept override EMEN_EXCLUDES(m_inputAccess);

			/**
			 * @brief Records one keypad step for the logic thread.
			 * @note MAIN thread.
			 * @param axis The axis of the key.
			 * @param sign -1 or +1.
			 */
			void recordStep (Axis axis, float sign) noexcept EMEN_EXCLUDES(m_inputAccess);

			/**
			 * @brief Applies one recorded step to the node.
			 * @note LOGIC thread.
			 * @param node The controlled node.
			 * @param step The step.
			 */
			static void applyStep (Node & node, const Step & step) noexcept;

			/**
			 * @brief Moves the node from the gamepad's held buttons (polled): one step per held button and per cycle.
			 * @note LOGIC thread.
			 * @param node The controlled node.
			 * @param stepSizes The size of a translation and of a rotation step.
			 */
			void pollGamepad (Node & node, const StepSizes & stepSizes) const noexcept;

			/**
			 * @brief Switches the transform space of the next keypad steps.
			 * @note MAIN thread.
			 */
			void switchTransformSpace () noexcept;

			/**
			 * @brief Switches the keypad between translation and rotation steps.
			 * @note MAIN thread.
			 */
			void switchControlMode () noexcept;

			/** @brief Guards what the keypad's callbacks record for the logic thread, and the step sizes. */
			mutable Base::Mutex m_inputAccess;
			Base::StaticVector< Step, MaxPendingSteps > m_pendingSteps EMEN_GUARDED_BY(m_inputAccess);
			StepSizes m_stepSizes EMEN_GUARDED_BY(m_inputAccess);
			Input::GamepadController m_controller;
			Base::Math::TransformSpace m_transformSpace{Base::Math::TransformSpace::Parent}; /**< MAIN thread (the keypad's callbacks). */
			bool m_nodeRotationMode{false}; /**< MAIN thread (the keypad's callbacks). */
	};
}
