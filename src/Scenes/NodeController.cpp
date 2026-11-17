/*
 * src/Scenes/NodeController.cpp
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

#include "NodeController.hpp"

/* STL inclusions. */
#include <array>

/* Local inclusions. */
#include "Input/Types.hpp"
#include "Math/Base.hpp"
#include "Mutex.hpp"
#include "Node.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;

	void
	NodeController::connectDevice (int deviceId, const StepSizes & stepSizes) noexcept
	{
		{
			const MutexLock lock{m_inputAccess};

			m_stepSizes = stepSizes;
		}

		m_controller.attachDeviceID(deviceId);
	}

	void
	NodeController::switchTransformSpace () noexcept
	{
		switch ( m_transformSpace )
		{
			case TransformSpace::Local :
				m_transformSpace = TransformSpace::Parent;

				Tracer::info(ClassId, "Transform space : Parent.");
				break;

			case TransformSpace::Parent :
				m_transformSpace = TransformSpace::World;

				Tracer::info(ClassId, "Transform space : World.");
				break;

			case TransformSpace::World :
				m_transformSpace = TransformSpace::Local;

				Tracer::info(ClassId, "Transform space : Local.");
				break;
		}
	}

	void
	NodeController::switchControlMode () noexcept
	{
		if ( m_nodeRotationMode )
		{
			m_transformSpace = TransformSpace::Parent;
			m_nodeRotationMode = false;

			Tracer::info(ClassId, "Translation mode (Parent transform space).");
		}
		else
		{
			m_transformSpace = TransformSpace::Local;
			m_nodeRotationMode = true;

			Tracer::info(ClassId, "Rotation mode (Local transform space).");
		}
	}

	bool
	NodeController::onKeyRelease (int key, int /*scancode*/, int /*modifiers*/) noexcept
	{
		if ( !this->hasNode() )
		{
			return false;
		}

		switch ( key )
		{
			/* Switch between Rotate and move. */
			case Input::KeyPadDecimal :
				this->switchControlMode();
				return true;

			/* Switch between transform space. */
			case Input::KeyPad0 :
				this->switchTransformSpace();
				return true;

			case Input::KeyPad7 :
				this->recordStep(Axis::X, -1.0F);
				return true;

			case Input::KeyPad4 :
				this->recordStep(Axis::X, +1.0F);
				return true;

			case Input::KeyPad8 :
				this->recordStep(Axis::Y, -1.0F);
				return true;

			case Input::KeyPad5 :
				this->recordStep(Axis::Y, +1.0F);
				return true;

			case Input::KeyPad9 :
				this->recordStep(Axis::Z, -1.0F);
				return true;

			case Input::KeyPad6 :
				this->recordStep(Axis::Z, +1.0F);
				return true;

			default:
				return false;
		}
	}

	void
	NodeController::recordStep (Axis axis, float sign) noexcept
	{
		bool dropped = false;

		{
			const MutexLock lock{m_inputAccess};

			if ( m_pendingSteps.full() )
			{
				dropped = true;
			}
			else
			{
				m_pendingSteps.push_back({
					.amount = sign * (m_nodeRotationMode ? Math::Radian(m_stepSizes.rotationAngle) : m_stepSizes.move),
					.space = m_transformSpace,
					.axis = axis,
					.rotation = m_nodeRotationMode
				});
			}
		}

		if ( dropped )
		{
			Tracer::warning(ClassId, "A keypad step is dropped: the steps already asked wait for the logic thread (paused, or far behind).");
		}
	}

	void
	NodeController::applyStep (Node & node, const Step & step) noexcept
	{
		switch ( step.axis )
		{
			case Axis::X :
				if ( step.rotation )
				{
					node.yaw(step.amount, step.space);
				}
				else
				{
					node.moveX(step.amount, step.space);
				}
				break;

			case Axis::Y :
				if ( step.rotation )
				{
					node.pitch(step.amount, step.space);
				}
				else
				{
					node.moveY(step.amount, step.space);
				}
				break;

			case Axis::Z :
				if ( step.rotation )
				{
					node.roll(step.amount, step.space);
				}
				else
				{
					node.moveZ(step.amount, step.space);
				}
				break;
		}
	}

	void
	NodeController::onUpdate (Node & node) noexcept
	{
		/* What the keypad asked since the last cycle, taken under the lock; the node is written without it. */
		StaticVector< Step, MaxPendingSteps > steps;
		StepSizes stepSizes;

		{
			const MutexLock lock{m_inputAccess};

			steps = m_pendingSteps;
			m_pendingSteps.clear();
			stepSizes = m_stepSizes;
		}

		for ( const auto & step : steps )
		{
			applyStep(node, step);
		}

		this->pollGamepad(node, stepSizes);
	}

	void
	NodeController::pollGamepad (Node & node, const StepSizes & stepSizes) const noexcept
	{
		if ( !m_controller.isConnected() )
		{
			return;
		}

		/* What one held button does. A rotation about X is a yaw, about Y a pitch, about Z a roll (applyStep()), as on
		 * the keypad. */
		struct Binding final
		{
			decltype(Input::ButtonDPadUp) button;
			Axis axis;
			float sign;
			TransformSpace space;
			bool rotation;
		};

		using Page = std::array< Binding, 6 >;

		/* X held: the vertical and the lateral translations, the roll. */
		constexpr Page WithButtonX{{
			{.button = Input::ButtonDPadUp, .axis = Axis::Y, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadDown, .axis = Axis::Y, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadLeft, .axis = Axis::X, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadRight, .axis = Axis::X, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonLeftBumper, .axis = Axis::Z, .sign = +1.0F, .space = TransformSpace::Local, .rotation = true},
			{.button = Input::ButtonRightBumper, .axis = Axis::Z, .sign = -1.0F, .space = TransformSpace::Local, .rotation = true}
		}};

		/* Y held: the three translations. */
		constexpr Page WithButtonY{{
			{.button = Input::ButtonDPadUp, .axis = Axis::Z, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadDown, .axis = Axis::Z, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadLeft, .axis = Axis::X, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadRight, .axis = Axis::X, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonLeftBumper, .axis = Axis::Y, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonRightBumper, .axis = Axis::Y, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false}
		}};

		/* Neither: forward and backward, the yaw about the world's vertical, the pitch. */
		constexpr Page Alone{{
			{.button = Input::ButtonDPadUp, .axis = Axis::Z, .sign = -1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadDown, .axis = Axis::Z, .sign = +1.0F, .space = TransformSpace::Local, .rotation = false},
			{.button = Input::ButtonDPadLeft, .axis = Axis::X, .sign = +1.0F, .space = TransformSpace::World, .rotation = true},
			{.button = Input::ButtonDPadRight, .axis = Axis::X, .sign = -1.0F, .space = TransformSpace::World, .rotation = true},
			{.button = Input::ButtonLeftBumper, .axis = Axis::Y, .sign = -1.0F, .space = TransformSpace::Local, .rotation = true},
			{.button = Input::ButtonRightBumper, .axis = Axis::Y, .sign = +1.0F, .space = TransformSpace::Local, .rotation = true}
		}};

		const Page * page = &Alone;

		if ( m_controller.isButtonPressed(Input::ButtonX) )
		{
			page = &WithButtonX;
		}
		else if ( m_controller.isButtonPressed(Input::ButtonY) )
		{
			page = &WithButtonY;
		}

		const auto rotationStep = Math::Radian(stepSizes.rotationAngle);

		for ( const auto & binding : *page )
		{
			if ( m_controller.isButtonPressed(binding.button) )
			{
				applyStep(node, {
					.amount = binding.sign * (binding.rotation ? rotationStep : stepSizes.move),
					.space = binding.space,
					.axis = binding.axis,
					.rotation = binding.rotation
				});
			}
		}
	}
}
