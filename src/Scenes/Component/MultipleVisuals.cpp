/*
 * src/Scenes/Component/MultipleVisuals.cpp
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

#include "MultipleVisuals.hpp"

/* Local inclusions. */
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Physics;
	using namespace Graphics;

	MultipleVisuals::MultipleVisuals (const std::string & name, const AbstractEntity & parentEntity, const std::shared_ptr< Renderable::Interface > & renderable, const std::vector< CartesianFrame< float > > & coordinates) noexcept
		: Abstract(name, parentEntity),
		m_renderableInstance(std::make_shared< RenderableInstance::Multiple >(renderable, coordinates, renderable->isSprite() ? RenderableInstance::FacingCamera : RenderableInstance::None)),
		m_coordinates(coordinates)
	{
		this->observe(m_renderableInstance.get());
	}

	void
	MultipleVisuals::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		for ( auto & coordinate : m_coordinates )
		{
			coordinate.position() += worldCoordinates.position();
		}

		if ( !m_renderableInstance->updateLocalData(m_coordinates, 0) )
		{
			m_renderableInstance->setBroken("Something goes wrong when updating instances model matrices !");
		}
	}

	void
	MultipleVisuals::processLogics (const Scene & scene) noexcept
	{
		if ( m_renderableInstance->isBroken() )
		{
			return;
		}

		if ( m_renderableInstance->isAnimated() )
		{
			m_renderableInstance->updateFrameIndex(scene.lifetimeMS() - this->parentEntity().birthTime());
		}

		this->updateAnimations(scene.cycle());

		m_renderableInstance->updateVideoMemory();
	}

	bool
	MultipleVisuals::playAnimation (uint8_t /*animationID*/, const Variant & /*value*/, size_t /*cycle*/) noexcept
	{
		return false;
	}

	bool
	MultipleVisuals::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable == m_renderableInstance.get() )
		{
			switch ( notificationCode )
			{
				case RenderableInstance::Abstract::ReadyToSetupOnGPU :
					this->notify(ComponentContentModified);
					break;

				default:
					break;
			}

			return true;
		}

		/* NOTE: Auto-forget */
		return false;
	}
}
