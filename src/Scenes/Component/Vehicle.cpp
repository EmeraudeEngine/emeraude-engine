/*
 * src/Scenes/Component/Vehicle.cpp
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

#include "Vehicle.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Scenes/Node.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base::Math;

	Vehicle::~Vehicle () = default;

	bool
	Vehicle::attachWheelNode (size_t wheelIndex, const std::shared_ptr< Node > & node) noexcept
	{
		if ( wheelIndex >= m_controller.settings().wheels.size() )
		{
			return false;
		}

		while ( m_wheelNodes.size() <= wheelIndex )
		{
			m_wheelNodes.emplace_back();
		}

		m_wheelNodes[wheelIndex] = node;

		return true;
	}

	bool
	Vehicle::setInputScript (std::vector< ScriptedInput > script) noexcept
	{
		for ( size_t index = 0; index < script.size(); ++index )
		{
			const auto & entry = script[index];

			if ( !std::isfinite(entry.forward) || !std::isfinite(entry.right) || !std::isfinite(entry.brake) || !std::isfinite(entry.handBrake) ||
				(index > 0 && entry.fromCycle < script[index - 1].fromCycle) )
			{
				return false;
			}
		}

		m_script = std::move(script);

		return true;
	}

	void
	Vehicle::processLogics (const Scene & scene) noexcept
	{
		/* The script's entry for this cycle (the last one started). */
		if ( !m_script.empty() )
		{
			const auto cycle = scene.cycle();
			const ScriptedInput * current = nullptr;

			for ( const auto & entry : m_script )
			{
				if ( entry.fromCycle > cycle )
				{
					break;
				}

				current = &entry;
			}

			if ( current != nullptr )
			{
				static_cast< void >(m_controller.setInput(current->forward, current->right, current->brake, current->handBrake));
			}
		}

		/* The wheel nodes: on the suspension, steered about it, spinning about their axle. */
		const auto & settings = m_controller.settings();
		const auto & wheels = m_controller.wheels();

		for ( size_t index = 0; index < m_wheelNodes.size() && index < wheels.size(); ++index )
		{
			const auto node = m_wheelNodes[index].lock();

			if ( node == nullptr )
			{
				continue;
			}

			const auto & wheel = settings.wheels[index];
			const auto & state = wheels[index];
			const Vector< 3, float > up = -wheel.suspensionDirection;
			const auto axle = Vector< 3, float >::crossProduct(wheel.forward, up).normalized();

			CartesianFrame< float > frame;

			/* Steered about the suspension, then rolling: forward turns the top of the wheel forward, negative about the
			 * axle (right-handed). */
			frame.rotate(state.steerAngle, up, false);
			/* The axle in the wheel's own frame: the frame turned with the steering, the axle with it. */
			frame.rotate(-state.rotationAngle, axle, true);
			/* ⚠️ Placed AFTER the rotations: a parent-space rotate() also turns the position about the parent's origin
			 * (the chassis centre), which swung a steered wheel 0.7 m sideways. */
			frame.setPosition(wheel.attachment + (wheel.suspensionDirection * state.suspensionLength));

			node->setLocalCoordinates(frame);
		}
	}
}
