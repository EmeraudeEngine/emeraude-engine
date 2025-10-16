/*
 * src/Scenes/NodeController.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#include "NodeController.hpp"

/* STL inclusions. */
#include <cstddef>
#include <memory>

/* Local inclusions. */
#include "Libs/Math/Base.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Input/KeyboardListenerInterface.hpp"
#include "Node.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;

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
				if ( m_nodeRotationMode )
				{
					m_controlledNode->yaw(-Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveX(-m_moveStep, m_transformSpace);
				}
				return true;

			case Input::KeyPad4 :
				if ( m_nodeRotationMode )
				{
					m_controlledNode->yaw(+Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveX(+m_moveStep, m_transformSpace);
				}
				return true;

			case Input::KeyPad8 :
				if ( m_nodeRotationMode )
				{
					m_controlledNode->pitch(-Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveY(-m_moveStep, m_transformSpace);
				}
				return true;

			case Input::KeyPad5 :
				if ( m_nodeRotationMode )
				{
					m_controlledNode->pitch(+Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveY(+m_moveStep, m_transformSpace);
				}
				return true;

			case Input::KeyPad9 :
				if ( m_nodeRotationMode )
				{
					m_controlledNode->roll(-Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveZ(-m_moveStep, m_transformSpace);
				}
				return true;

			case Input::KeyPad6 :
				if ( m_nodeRotationMode )
				{
					m_controlledNode->roll(+Math::Radian(m_rotationAngleStep), m_transformSpace);
				}
				else
				{
					m_controlledNode->moveZ(+m_moveStep, m_transformSpace);
				}
				return true;

			default:
				return false;
		}
	}

	void
	NodeController::update () const noexcept
	{
		if ( !this->hasNode() || !m_controller.isConnected() )
		{
			return;
		}

		if ( m_controller.isButtonPressed(Input::ButtonX) )
		{
			if ( m_controller.isButtonPressed(Input::ButtonDPadUp) )
			{
				m_controlledNode->moveY(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadDown) )
			{
				m_controlledNode->moveY(-m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadLeft) )
			{
				m_controlledNode->moveX(-m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadRight) )
			{
				m_controlledNode->moveX(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonLeftBumper) )
			{
				m_controlledNode->roll(Math::Radian(m_rotationAngleStep), TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonRightBumper) )
			{
				m_controlledNode->roll(-Math::Radian(m_rotationAngleStep), TransformSpace::Local);
			}
		}
		else if ( m_controller.isButtonPressed(Input::ButtonY) )
		{
			if ( m_controller.isButtonPressed(Input::ButtonDPadUp) )
			{
				m_controlledNode->moveZ(-m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadDown) )
			{
				m_controlledNode->moveZ(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadLeft) )
			{
				m_controlledNode->moveX(-m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadRight) )
			{
				m_controlledNode->moveX(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonLeftBumper) )
			{
				m_controlledNode->moveY(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonRightBumper) )
			{
				m_controlledNode->moveY(-m_moveStep, TransformSpace::Local);
			}
		}
		else
		{
			if ( m_controller.isButtonPressed(Input::ButtonDPadUp) )
			{
				m_controlledNode->moveZ(-m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadDown) )
			{
				m_controlledNode->moveZ(m_moveStep, TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadLeft) )
			{
				m_controlledNode->yaw(Math::Radian(m_rotationAngleStep), TransformSpace::World);
			}

			if ( m_controller.isButtonPressed(Input::ButtonDPadRight) )
			{
				m_controlledNode->yaw(-Math::Radian(m_rotationAngleStep), TransformSpace::World);
			}

			if ( m_controller.isButtonPressed(Input::ButtonLeftBumper) )
			{
				m_controlledNode->pitch(-Math::Radian(m_rotationAngleStep), TransformSpace::Local);
			}

			if ( m_controller.isButtonPressed(Input::ButtonRightBumper) )
			{
				m_controlledNode->pitch(Math::Radian(m_rotationAngleStep), TransformSpace::Local);
			}
		}
	}
}
